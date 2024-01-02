#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xr.h"
#include "lsic.h"
#include "ebus.h"

uint8_t XrSimulateCaches = 1;
uint8_t XrSimulateCacheStalls = 0;
uint8_t XrPrintCache = 0;

static inline uint32_t RoR(uint32_t x, uint32_t n) {
    return (x >> n & 31) | (x << (32-n) & 31);
}

#define XR_LINE_INVALID 0
#define XR_LINE_SHARED 1
#define XR_LINE_EXCLUSIVE 2

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

#define XrReadByte(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 1);
#define XrReadInt(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 2);
#define XrReadLong(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 4);

#define XrWriteByte(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 1);
#define XrWriteInt(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 2);
#define XrWriteLong(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 4);

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

	proc->IcLastTag = -1;
	proc->DcLastTag = -1;

	proc->IcReplacementIndex = 0;
	proc->DcReplacementIndex = 0;

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

static inline void XrBasicException(XrProcessor *proc, uint32_t exc) {
	// "Basic" exceptions that behave the same way every time.

	proc->Cr[EPC] = proc->Pc - 4;

	XrPushMode(proc);
	XrSetEcause(proc, exc);
	XrVectorException(proc, exc);
}

static inline void XrBasicInbetweenException(XrProcessor *proc, uint32_t exc) {
	// "Basic" exceptions that behave the same way every time.

	proc->Cr[EPC] = proc->Pc;

	XrPushMode(proc);
	XrSetEcause(proc, exc);
	XrVectorException(proc, exc);
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
		} else if (!XrLookupItb(proc, virtual, &tbe)) {
			return 0;
		}
	} else {
		if (proc->DtbLastVpn == vpn) {
			// This matches the last lookup, avoid searching the whole DTB.

			tbe = proc->DtbLastResult;
		} else if (!XrLookupDtb(proc, virtual, &tbe, writing)) {
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

		XrSetEcause(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);
		XrVectorException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);

		return 0;
	}

	if ((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER)) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);

		return 0;
	}

	if (writing && !(tbe & PTE_WRITABLE)) {
		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);

		return 0;
	}

	if (proc->IFetch) {
		proc->ItbLastVpn = vpn;
		proc->ItbLastResult = tbe;
	} else {
		proc->DtbLastVpn = vpn;
		proc->DtbLastResult = tbe;
	}

	*cachetype = (tbe & PTE_NONCACHED) ? NONCACHED : CACHED;
	*phys = ((tbe & 0x1FFFFE0) << 7) + (virtual & 0xFFF);

	//printf("virt=%x phys=%x\n", virt, *phys);

	return 1;
}

static uint32_t XrAccessMasks[5] = {
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static uint8_t XrAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length) {
	if (address & (length - 1)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA);

		return 0;
	}

	int cachetype = CACHED;

	if (proc->Cr[RS] & RS_MMU) {
		if (!XrTranslate(proc, address, &address, &cachetype, dest ? 0 : 1)) {
			return 0;
		}
	} else if (address >= XR_NONCACHED_PHYS_BASE || !XrSimulateCaches) {
		cachetype = NONCACHED;
	}

	if (!proc->IFetch) { // TEMP
		cachetype = NONCACHED;
	}

	if (cachetype == NONCACHED) {
		proc->StallCycles += XR_UNCACHED_STALL;

		int result;

		XrLockIoMutex(proc);

		if (dest) {
			result = EBusRead(address, dest, length);
		} else {
			result = EBusWrite(address, &srcvalue, length);
		}

		XrUnlockIoMutex();

		if (result == EBUSERROR) {
			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS);

			return 0;
		}

		if (dest) {
			*dest &= XrAccessMasks[length];
		}

		return 1;
	}

	if (proc->IFetch) {
		// Access Icache. Quite fast, don't need to take any locks or anything
		// as there is no coherence, plus its always a 32-bit read, so we
		// duplicate some logic here as a fast path.

		uint32_t tag = address & ~(XR_IC_LINE_SIZE - 1);
		uint32_t lineoffset = address & (XR_IC_LINE_SIZE - 1);

		if (tag == proc->IcLastTag) {
			// Matches the lookup hint, nice.

			*dest = *(uint32_t*)(&proc->Ic[proc->IcLastOffset + lineoffset]);

			return 1;
		}

		uint32_t setnumber = address & (XR_IC_SET_LOG - 1);
		uint32_t cacheindex = setnumber << XR_IC_WAY_LOG;

		for (int i = 0; i < XR_IC_WAYS; i++) {
			if (proc->IcFlags[cacheindex + i] && proc->IcTags[cacheindex + i] == tag) {
				// Found it!

				uint32_t cacheoff = (cacheindex + i) << XR_IC_LINE_SIZE_LOG;

				*dest = *(uint32_t*)(&proc->Ic[cacheoff + lineoffset]);

				proc->IcLastTag = tag;
				proc->IcLastOffset = cacheoff;

				return 1;
			}
		}

		// Unfortunately there was a miss. Incur a penalty.

		proc->StallCycles += XR_MISS_STALL;

		// Replace a random line within the set.

		uint32_t newindex = cacheindex + (proc->IcReplacementIndex & (XR_IC_WAYS - 1));
		proc->IcReplacementIndex += 1;

		uint32_t cacheoff = newindex << XR_IC_LINE_SIZE_LOG;

		XrLockIoMutex(proc);
		int result = EBusRead(tag, &proc->Ic[cacheoff], XR_IC_LINE_SIZE);
		XrUnlockIoMutex();

		if (result == EBUSERROR) {
			proc->IcFlags[newindex] = XR_LINE_INVALID;

			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS);

			return 0;
		}

		proc->IcFlags[newindex] = XR_LINE_SHARED;
		proc->IcTags[newindex] = tag;

		proc->IcLastTag = tag;
		proc->IcLastOffset = cacheoff;

		*dest = *(uint32_t*)(&proc->Ic[cacheoff + lineoffset]);

		return 1;
	}

	return 1;
}

uint32_t XrExecute(XrProcessor *proc, uint32_t cycles, uint32_t dt) {
	if (!proc->Running) {
		return cycles;
	}

	Lsic *lsic = &LsicTable[proc->Id];

	if (proc->UserBreak) {
		// There's a pending user-initiated NMI, so do that.

		XrBasicInbetweenException(proc, XR_EXC_NMI);
		proc->UserBreak = 0;
		proc->Halted = 0;
	}

	if (proc->Halted) {
		MemoryBarrier;

		if ((proc->Cr[RS] & RS_INT) && lsic->InterruptPending) {
			proc->Halted = 0;
		} else {
			return cycles;
		}
	}

	uint32_t cyclesdone = 0;

	uint32_t currentpc;
	uint32_t ir;
	uint32_t maj;
	uint32_t majoropcode;
	uint32_t funct;
	uint32_t shift;
	uint32_t shifttype;
	uint32_t val;
	uint32_t rd;
	uint32_t ra;
	uint32_t rb;
	uint32_t imm;

	int status;

	for (; cyclesdone < cycles; cyclesdone++) {
		if (proc->Progress <= 0) {
			// This processor did a poll-y looking thing too many times this
			// tick. Skip the rest of the tick so as not to eat up too much of
			// the host's CPU.

			return cycles;
		}

		if (XrSimulateCacheStalls && proc->StallCycles) {
			// There's a simulated cache stall of some number of cycles, so
			// decrement the remaining stall and loop.

			proc->StallCycles--;

			continue;
		}

		// Make sure the zero register is always zero, except during TLB misses,
		// where it may be used as a scratch register.

		if ((proc->Cr[RS] & RS_TBMISS) == 0) {
			proc->Reg[0] = 0;
		}

		if (XrSimulateCaches) {
			// TODO writebuffer sim
		}

		if (((cyclesdone & 63) == 0) && (proc->Cr[RS] & RS_INT)) {
			// Interrupts are enabled, so sample the pending interrupt flag.
			// We only do this once every 64 cycles.

			MemoryBarrier;

			if (lsic->InterruptPending) {
				// There's an interrupt pending, so cause an interrupt
				// exception.

				XrBasicInbetweenException(proc, XR_EXC_INT);
			}
		}

		currentpc = proc->Pc;
		proc->Pc += 4;

		proc->IFetch = 1;
		status = XrReadLong(proc, currentpc, &ir);
		proc->IFetch = 0;

		if (!status) {
			continue;
		}

		// Fetch was successful, decode the instruction word and execute the
		// instruction.

		maj = ir & 7;
		majoropcode = ir & 63;

		if (maj == 7) { // JAL
			proc->Reg[LR] = proc->Pc;
			proc->Pc = (currentpc & 0x80000000) | ((ir >> 3) << 2);
		} else if (maj == 6) { // J
			proc->Pc = (currentpc & 0x80000000) | ((ir >> 3) << 2);
		} else if (majoropcode == 57) { // reg instructions 111001
			funct = ir >> 28;

			shifttype = (ir >> 26) & 3;
			shift = (ir >> 21) & 31;

			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			rb = (ir >> 16) & 31;

			if (shift) {
				switch(shifttype) {
					case 0: // LSH
						val = proc->Reg[rb] << shift;
						break;

					case 1: // RSH
						val = proc->Reg[rb] >> shift;
						break;

					case 2: // ASH
						val = (int32_t) proc->Reg[rb] >> shift;
						break;

					case 3: // ROR
						val = RoR(proc->Reg[rb], shift);
						break;
				}
			} else {
				val = proc->Reg[rb];
			}

			switch(funct) {
				case 0: // NOR
					proc->Reg[rd] = ~(proc->Reg[ra] | val);
					break;

				case 1: // OR
					proc->Reg[rd] = proc->Reg[ra] | val;
					break;

				case 2: // XOR
					proc->Reg[rd] = proc->Reg[ra] ^ val;
					break;

				case 3: // AND
					proc->Reg[rd] = proc->Reg[ra] & val;
					break;

				case 4: // SLT SIGNED
					if ((int32_t) proc->Reg[ra] < (int32_t) val)
						proc->Reg[rd] = 1;
					else
						proc->Reg[rd] = 0;
					break;

				case 5: // SLT
					if (proc->Reg[ra] < val)
						proc->Reg[rd] = 1;
					else
						proc->Reg[rd] = 0;

					break;

				case 6: // SUB
					proc->Reg[rd] = proc->Reg[ra] - val;
					break;

				case 7: // ADD
					proc->Reg[rd] = proc->Reg[ra] + val;
					break;

				case 8: // *SH
					switch(shifttype) {
						case 0: // LSH
							proc->Reg[rd] = proc->Reg[rb] << proc->Reg[ra];
							break;

						case 1: // RSH
							proc->Reg[rd] = proc->Reg[rb] >> proc->Reg[ra];
							break;

						case 2: // ASH
							proc->Reg[rd] = (int32_t) proc->Reg[rb] >> proc->Reg[ra];
							break;

						case 3: // ROR
							proc->Reg[rd] = RoR(proc->Reg[rb], proc->Reg[ra]);
							break;
					}
					break;

				case 9: // MOV LONG, RD
					XrWriteLong(proc, proc->Reg[ra] + val, proc->Reg[rd]);
					break;

				case 10: // MOV INT, RD
					XrWriteInt(proc, proc->Reg[ra] + val, proc->Reg[rd]);
					break;

				case 11: // MOV BYTE, RD
					XrWriteByte(proc, proc->Reg[ra] + val, proc->Reg[rd]);
					break;

				case 12: // invalid
					XrBasicException(proc, XR_EXC_INV);
					break;

				case 13: // MOV RD, LONG
					XrReadLong(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
					break;

				case 14: // MOV RD, INT
					XrReadInt(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
					break;

				case 15: // MOV RD, BYTE
					XrReadByte(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
					break;

				default: // unreachable
					abort();
			}
		} else if (majoropcode == 49) { // reg instructions 110001
			funct = ir >> 28;

			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			rb = (ir >> 16) & 31;

			switch(funct) {
				case 0: // SYS
					XrBasicInbetweenException(proc, XR_EXC_SYS);
					break;

				case 1: // BRK
					XrBasicException(proc, XR_EXC_BRK);
					break;

				case 2: // WMB
				case 3: // MB
					if (XrSimulateCaches) {
						// TODO flush writebuffer
					}

					break;

				case 8: // SC
					// TODO SC multiprocessor semantics

					if (proc->Locked) {
						XrWriteLong(proc, proc->Reg[ra], proc->Reg[rb]);
					}

					proc->Reg[rd] = proc->Locked;

					break;

				case 9: // LL
					// TODO LL multiprocessor semantics

					proc->Locked = 1;

					XrReadLong(proc, proc->Reg[ra], &proc->Reg[rd]);

					break;

				case 11: // MOD
					if (proc->Reg[rb] == 0) {
						proc->Reg[rd] = 0;
						break;
					}

					proc->Reg[rd] = proc->Reg[ra] % proc->Reg[rb];
					break;

				case 12: // DIV SIGNED
					if (proc->Reg[rb] == 0) {
						proc->Reg[rd] = 0;
						break;
					}

					proc->Reg[rd] = (int32_t) proc->Reg[ra] / (int32_t) proc->Reg[rb];
					break;

				case 13: // DIV
					if (proc->Reg[rb] == 0) {
						proc->Reg[rd] = 0;
						break;
					}

					proc->Reg[rd] = proc->Reg[ra] / proc->Reg[rb];
					break;

				case 15: // MUL
					proc->Reg[rd] = proc->Reg[ra] * proc->Reg[rb];
					break;

				default:
					XrBasicException(proc, XR_EXC_INV);
					break;
			}
		} else if (majoropcode == 41) { // privileged instructions 101001
			if (proc->Cr[RS] & RS_USER) {
				// Current mode is usermode, so cause a privilege violation
				// exception.

				XrBasicException(proc, XR_EXC_PRV);
			} else {
				funct = ir >> 28;

				rd = (ir >> 6) & 31;
				ra = (ir >> 11) & 31;
				rb = (ir >> 16) & 31;

				uint32_t asid;
				uint32_t vpn;
				uint32_t index;
				uint64_t tlbe;
				uint32_t pde;
				uint32_t tbhi;

				switch(funct) {
					case 11: // RFE
						proc->Locked = 0;

						if (proc->Cr[RS] & RS_TBMISS) {
							proc->Pc = proc->Cr[TBPC];
						} else {
							proc->Pc = proc->Cr[EPC];
						}

						proc->Cr[RS] = (proc->Cr[RS] & 0xF0000000) | ((proc->Cr[RS] >> 8) & 0xFFFF);
						//printf("rfe rs=%x\n", ControlReg[RS]);

						break;

					case 12: // HLT
						proc->Halted = true;
						break;

					case 14: // MTCR
						switch(rb) {
							case ICACHECTRL:
								if (!XrSimulateCaches)
									break;

								if ((proc->Reg[ra] & 3) == 3) {
									// Invalidate the entire Icache.

									for (int i = 0; i < XR_IC_LINE_COUNT; i++) {
										proc->IcFlags[i] = XR_LINE_INVALID;
									}
								} else if ((proc->Reg[ra] & 3) == 2) {
									// Invalidate a single page frame of the
									// Icache.

									uint32_t phys = proc->Reg[ra] & 0xFFFFF000;

									for (int i = 0; i < XR_IC_LINE_COUNT; i++) {
										if ((proc->IcTags[i] & 0xFFFFF000) == phys) {
											proc->IcFlags[i] = XR_LINE_INVALID;
										}
									}
								}

								// Reset the lookup hint.

								proc->IcLastTag = -1;

								break;

							case DCACHECTRL:
								if (!XrSimulateCaches)
									break;

								XrLockCache(proc);

								if ((proc->Reg[ra] & 3) == 3) {
									// Invalidate the entire Dcache.

									for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
										proc->DcFlags[i] = XR_LINE_INVALID;
									}
								} else if ((proc->Reg[ra] & 3) == 2) {
									// Invalidate a single page frame of the
									// Dcache.

									uint32_t phys = proc->Reg[ra] & 0xFFFFF000;

									for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
										if ((proc->DcTags[i] & 0xFFFFF000) == phys) {
											proc->DcFlags[i] = XR_LINE_INVALID;
										}
									}
								}

								// Reset the lookup hint.

								proc->DcLastTag = -1;

								XrUnlockCache(proc);

								break;

							case ITBCTRL:
								if ((proc->Reg[ra] & 3) == 3) {
									// Invalidate the entire ITB.

									for (int i = 0; i < XR_ITB_SIZE; i++) {
										proc->Itb[i] = 0;
									}
								} else if ((proc->Reg[ra] & 3) == 2) {
									// Invalidate the entire ITB except for
									// global entries.

									for (int i = 0; i < XR_ITB_SIZE; i++) {
										if ((proc->Itb[i] & PTE_GLOBAL) == 0) {
											proc->Itb[i] = 0;
										}
									}
								} else if ((proc->Reg[ra] & 3) == 0) {
									// Invalidate a single page in the ITB.

									uint64_t vpn = (uint64_t)(proc->Reg[ra] >> 12) << 32;

									for (int i = 0; i < XR_ITB_SIZE; i++) {
										if ((proc->Itb[i] & 0xFFFFF00000000) == vpn) {
											proc->Itb[i] = 0;
										}
									}

									//printf("invl %x\n", Reg[ra] >> 12);

									//Running = false;
								}

								// Reset the lookup hint.

								proc->ItbLastVpn = -1;

								break;

							case DTBCTRL:
								if ((proc->Reg[ra] & 3) == 3) {
									// Invalidate the entire DTB.

									for (int i = 0; i < XR_DTB_SIZE; i++) {
										proc->Dtb[i] = 0;
									}
								} else if ((proc->Reg[ra] & 3) == 2) {
									// Invalidate the entire DTB except for
									// global entries.

									for (int i = 0; i < XR_DTB_SIZE; i++) {
										if ((proc->Dtb[i] & PTE_GLOBAL) == 0) {
											proc->Dtb[i] = 0;
										}
									}
								} else if ((proc->Reg[ra] & 3) == 0) {
									// Invalidate a single page in the DTB.

									uint64_t vpn = (uint64_t)(proc->Reg[ra] >> 12) << 32;

									for (int i = 0; i < XR_DTB_SIZE; i++) {
										if ((proc->Dtb[i] & 0xFFFFF00000000) == vpn) {
											proc->Dtb[i] = 0;
										}
									}

									//printf("invl %x\n", Reg[ra] >> 12);

									//Running = false;
								}

								// Reset the lookup hint.

								proc->DtbLastVpn = -1;

								break;

							case ITBPTE:
								// Write an entry to the ITB at ITBINDEX, and
								// increment it.

								proc->Itb[proc->Cr[ITBINDEX]] = ((uint64_t)(proc->Cr[ITBTAG]) << 32) | proc->Reg[ra];

								//printf("ITB[%d] = %llx\n", ControlReg[ITBINDEX], ITlb[ControlReg[ITBINDEX]]);

								proc->Cr[ITBINDEX] += 1;

								if (proc->Cr[ITBINDEX] == XR_ITB_SIZE) {
									// Roll over to index four.

									proc->Cr[ITBINDEX] = 4;
								}

								break;

							case DTBPTE:
								// Write an entry to the DTB at DTBINDEX, and
								// increment it.

								proc->Dtb[proc->Cr[DTBINDEX]] = ((uint64_t)(proc->Cr[DTBTAG]) << 32) | proc->Reg[ra];

								//printf("DTB[%d] = %llx\n", ControlReg[DTBINDEX], DTlb[ControlReg[DTBINDEX]]);

								proc->Cr[DTBINDEX] += 1;

								if (proc->Cr[DTBINDEX] == XR_DTB_SIZE) {
									// Roll over to index four.

									proc->Cr[DTBINDEX] = 4;
								}

								break;

							case ITBINDEX:
								proc->Cr[ITBINDEX] = proc->Reg[ra] & (XR_ITB_SIZE - 1);
								//printf("ITBX = %x\n", ControlReg[ITBINDEX]);
								break;

							case DTBINDEX:
								proc->Cr[DTBINDEX] = proc->Reg[ra] & (XR_DTB_SIZE - 1);
								//printf("DTBX = %x\n", ControlReg[DTBINDEX]);
								break;

							case DTBTAG:
								proc->Cr[DTBTAG] = proc->Reg[ra];

								// Reset the lookup hint.

								proc->DtbLastVpn = -1;

								break;

							case ITBTAG:
								proc->Cr[ITBTAG] = proc->Reg[ra];

								// Reset the lookup hint.

								proc->ItbLastVpn = -1;
								
								break;

							default:
								proc->Cr[rb] = proc->Reg[ra];
								break;
						}

					case 15: // MFCR
						proc->Reg[rd] = proc->Cr[rb];
						break;

					default:
						XrBasicException(proc, XR_EXC_INV);
						break;
				}
			}
		} else { // major opcodes
			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			imm = ir >> 16;

			switch(majoropcode) {
				// branches
				
				case 61: // BEQ
					if (proc->Reg[rd] == 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 53: // BNE
					if (proc->Reg[rd] != 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 45: // BLT
					if ((int32_t) proc->Reg[rd] < 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 37: // BGT
					if ((int32_t) proc->Reg[rd] > 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 29: // BLE
					if ((int32_t) proc->Reg[rd] <= 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 21: // BGE
					if ((int32_t) proc->Reg[rd] >= 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 13: // BPE
					if ((proc->Reg[rd] & 1) == 0) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				case 5: // BPO
					if (proc->Reg[rd] & 1) {
						proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
					}

					break;

				// ALU

				case 60: // ADDI
					proc->Reg[rd] = proc->Reg[ra] + imm;

					break;

				case 52: // SUBI
					proc->Reg[rd] = proc->Reg[ra] - imm;

					break;

				case 44: // SLTI
					if (proc->Reg[ra] < imm) {
						proc->Reg[rd] = 1;
					} else {
						proc->Reg[rd] = 0;
					}

					break;

				case 36: // SLTI signed
					if ((int32_t) proc->Reg[ra] < (int32_t) SignExt16(imm)) {
						proc->Reg[rd] = 1;
					} else {
						proc->Reg[rd] = 0;
					}

					break;

				case 28: // ANDI
					proc->Reg[rd] = proc->Reg[ra] & imm;

					break;

				case 20: // XORI
					proc->Reg[rd] = proc->Reg[ra] ^ imm;

					break;

				case 12: // ORI
					proc->Reg[rd] = proc->Reg[ra] | imm;

					break;

				case 4: // LUI
					proc->Reg[rd] = proc->Reg[ra] | (imm << 16);

					break;

				// LOAD with immediate offset

				case 59: // MOV RD, BYTE
					XrReadByte(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

					break;

				case 51: // MOV RD, INT
					XrReadInt(proc, proc->Reg[ra] + (imm << 1), &proc->Reg[rd]);

					break;

				case 43: // MOV RD, LONG
					XrReadLong(proc, proc->Reg[ra] + (imm << 2), &proc->Reg[rd]);

					break;

				// STORE with immediate offset

				case 58: // MOV BYTE RD+IMM, RA
					XrWriteByte(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

					break;

				case 50: // MOV INT RD+IMM, RA
					XrWriteInt(proc, proc->Reg[rd] + (imm << 1), proc->Reg[ra]);

					break;

				case 42: // MOV LONG RD+IMM, RA
					XrWriteLong(proc, proc->Reg[rd] + (imm << 2), proc->Reg[ra]);

					break;

				case 26: // MOV BYTE RD+IMM, IMM5
					XrWriteByte(proc, proc->Reg[rd] + imm, SignExt5(ra));

					break;

				case 18: // MOV INT RD+IMM, IMM5
					XrWriteInt(proc, proc->Reg[rd] + (imm << 1), SignExt5(ra));

					break;

				case 10: // MOV LONG RD+IMM, IMM5
					XrWriteLong(proc, proc->Reg[rd] + (imm << 2), SignExt5(ra));

					break;

				case 56: // JALR
					proc->Reg[rd] = proc->Pc;
					proc->Pc = proc->Reg[ra] + SignExt18(imm << 2);

					break;

				default:
					XrBasicException(proc, XR_EXC_INV);
					break;
			}
		}

		if (proc->Halted || (!proc->Running))
			return cyclesdone;
	}

	return cyclesdone;
}