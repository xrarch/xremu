#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xr.h"
#include "lsic.h"
#include "ebus.h"

#define RS_USER   1
#define RS_INT    2
#define RS_MMU    4
#define RS_TBMISS 8
#define RS_LEGACY 128

#define RS_ECAUSE_SHIFT 28
#define RS_ECAUSE_MASK  15

#define PTE_VALID     1
#define PTE_WRITABLE  2
#define PTE_KERNEL    4
#define PTE_NONCACHED 8
#define PTE_GLOBAL    16

#define SignExt23(n) (((int32_t)(n << 9)) >> 9)
#define SignExt18(n) (((int32_t)(n << 14)) >> 14)
#define SignExt5(n)  (((int32_t)(n << 27)) >> 27)
#define SignExt16(n) (((int32_t)(n << 16)) >> 16)

#define LR 31

#define RS 0
#define WHAMI 1
#define EB 5
#define EPC 6
#define EBADADDR 7
#define TBMISSADDR 9
#define TBPC 10

#define ITBPTE 16
#define ITBTAG 17
#define ITBINDEX 18
#define ITBCTRL 19
#define ICACHECTRL 20
#define ITBADDR 21

#define DTBPTE 24
#define DTBTAG 25
#define DTBINDEX 26
#define DTBCTRL 27
#define DCACHECTRL 28
#define DTBADDR 29

#define NONCACHED 0
#define CACHED 1

void XrReset(XrProcessor *proc) {
	// Set the program counter to point to the reset vector.

	proc->Pc = 0xFFFE1000;

	// Initialize the control registers that have reset-defined values.

	proc->Cr[RS] = 0;
	proc->Cr[EB] = 0;
	proc->Cr[ICACHECTRL] = (XR_IC_LINE_COUNT_LOG << 16) | (XR_IC_WAY_LOG << 8) | (XR_IC_LINE_SIZE_LOG);
	proc->Cr[DCACHECTRL] = (XR_DC_LINE_COUNT_LOG << 16) | (XR_DC_WAY_LOG << 8) | (XR_DC_LINE_SIZE_LOG);
	proc->Cr[WHAMI] = proc->Id;

	// Initialize emulator support stuff.

	proc->ItbLastVpn = -1;
	proc->DtbLastVpn = -1;

	proc->WbIndex = 0;
	proc->WbSize = 0;
	proc->WbCyclesTilNextWrite = 0;

	proc->LastTbMissWasWrite = 0;
	proc->IFetch = 0;
	proc->UserBreak = 0;
	proc->Halted = 0;
	proc->Running = 1;
}

static inline void XrPushMode(XrProcessor *proc) {
	// "Push" the mode stack bits in RS.

	proc->Cr[RS] = (proc->Cr[RS] & 0xFF0000FF) | ((proc->Cr[RS] & 0xFFFF) << 8);
}

static inline void XrSetEcause(XrProcessor *proc, uint32_t exc) {
	// Set the ECAUSE code in RS.

	proc->Cr[RS] = (proc->Cr[RS] & 0x0FFFFFFF) | (exc << 28);
}

static inline void XrVectorException(XrProcessor *proc, uint32_t exc) {
	// This implements stuff that is common to all exceptions.
	// Note that it does NOT push the mode stack, save PC into EPC, or set the
	// exception code in RS.

	if (proc->Cr[EB] == 0) {
		// Reset the processor.

		XrReset(proc);

		return;
	}

	// Build new mode bits.
	// Enter kernel mode and disable interrupts.

	uint32_t newmode = proc->Cr[RS] & 0xFC;

	if (proc->Cr[RS] & RS_LEGACY) {
		// Legacy exceptions are enabled, so disable virtual addressing. This is
		// NOT part of the "official" xr17032 architecture and is a hack to
		// continue running AISIX in emulation.

		newmode &= ~RS_MMU;
	}

	// Redirect PC to the exception vector.

	proc->Pc = proc->Cr[EB] | (exc << 8);

	// Set the mode bits in RS.

	proc->Cr[RS] = (proc->Cr[RS] & 0xFFFFFF00) | newmode;
}

static inline uint8_t XrLookupItb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[ITBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_ITB_SIZE; i++) {
		uint64_t tmp = proc->Itb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return 1;
		}
	}

	// ITB miss!

	proc->Cr[ITBTAG] = matching;
	proc->Cr[ITBADDR] = (proc->Cr[ITBADDR] & 0xFFC00000) | (vpn << 2);
	proc->LastTbMissWasWrite = 0;

	if ((proc->Cr[RS] & RS_TBMISS) == 0) {
		XrPushMode(proc);
		proc->Cr[TBMISSADDR] = virtual;
		proc->Cr[TBPC] = proc->Pc - 4;
		proc->Cr[RS] |= RS_TBMISS;
	}

	XrVectorException(proc, XR_EXC_ITB);

	return 0;
}

static inline uint8_t XrLookupDtb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe, uint8_t writing) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_DTB_SIZE; i++) {
		uint64_t tmp = proc->Dtb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return 1;
		}
	}

	// DTB miss!

	proc->Cr[DTBTAG] = matching;
	proc->Cr[DTBADDR] = (proc->Cr[DTBADDR] & 0xFFC00000) | (vpn << 2);
	proc->LastTbMissWasWrite = writing;

	if ((proc->Cr[RS] & RS_TBMISS) == 0) {
		XrPushMode(proc);
		proc->Cr[TBMISSADDR] = virtual;
		proc->Cr[TBPC] = proc->Pc - 4;
		proc->Cr[RS] |= RS_TBMISS;
	}

	XrVectorException(proc, XR_EXC_DTB);

	return 0;
}

static inline uint8_t XrTranslate(XrProcessor *proc, uint32_t virtual, uint32_t *phys, int *cachetype, bool writing) {
	uint64_t tbe;
	uint32_t vpn = virtual >> 12;

	if (proc->IFetch) {
		if (proc->ItbLastVpn == vpn) {
			// This matches the last lookup, avoid searching the whole ITB.

			tbe = proc->ItbLastResult;
		} else {
			if (!XrLookupItb(proc, virtual, &tbe))
				return 0;
		}
	} else {
		if (proc->DtbLastVpn == vpn) {
			// This matches the last lookup, avoid searching the whole DTB.

			tbe = proc->DtbLastResult;
		} else {
			if (!XrLookupDtb(proc, virtual, &tbe, writing))
				return 0;
		}
	}

	if ((tbe & PTE_VALID) == 0) {
		// Not valid! Page fault time.

		if (proc->Cr[RS] & RS_TBMISS) {
			// This page fault happened while handling a TB miss, which means
			// it was a fault on a page table. Clear the TBMISS flag from RS and
			// report the original missed address as the faulting address. Also,
			// set EPC to point to the instruction that caused the original TB
			// miss, so that the faulting PC is reported correctly.

			proc->Cr[EBADADDR] = proc->Cr[TBMISSADDR];
			proc->Cr[EPC] = proc->Cr[TBPC];
			proc->Cr[RS] &= ~RS_TBMISS;
			writing = proc->LastTbMissWasWrite;
		} else {
			proc->Cr[EBADADDR] = virtual;
			proc->Cr[EPC] = proc->Pc - 4;
			XrPushMode(proc);
		}

		XrSetEcause(proc, writing ? XR_EXC_PGF : XR_EXC_PGW);
		XrVectorException(proc, writing ? XR_EXC_PGF : XR_EXC_PGW);

		return 0;
	}

	if ((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER)) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		proc->Cr[EPC] = proc->Pc - 4;

		XrPushMode(proc);
		XrSetEcause(proc, writing ? XR_EXC_PGF : XR_EXC_PGW);
		XrVectorException(proc, writing ? XR_EXC_PGF : XR_EXC_PGW);

		return 0;
	}

	if (writing && !(tbe & PTE_WRITABLE)) {
		proc->Cr[EBADADDR] = virtual;
		proc->Cr[EPC] = proc->Pc - 4;

		XrPushMode(proc);
		XrSetEcause(proc, XR_EXC_PGW);
		XrVectorException(proc, XR_EXC_PGW);

		return 0;
	}

	uint32_t physaddr = (tbe & 0x1FFFFE0) << 7;
	int cached = (tbe & PTE_NONCACHED) ? NONCACHED : CACHED;

	if (proc->IFetch) {
		proc->ItbLastVpn = vpn;
		proc->ItbLastResult = tbe;
	} else {
		proc->DtbLastVpn = vpn;
		proc->DtbLastResult = tbe;
	}

	*cachetype = cached;
	*phys = physaddr + (virtual & 0xFFF);

	//printf("virt=%x phys=%x\n", virt, *phys);

	return 1;
}