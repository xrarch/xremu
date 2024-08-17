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

uint32_t XrScacheTags[XR_SC_LINE_COUNT];
uint32_t XrScacheReplacementIndex;
uint8_t XrScacheFlags[XR_SC_LINE_COUNT];
uint8_t XrScacheExclusiveIds[XR_SC_LINE_COUNT];

#if DBG

#define DBGPRINT(...) printf(_VA_ARGS_)

#else

#define DBGPRINT(...)

#endif

static inline uint32_t RoR(uint32_t x, uint32_t n) {
    return (x >> n & 31) | (x << (32-n) & 31);
}

#define NMI_MASK_CYCLES 64

// The canonical invalid TB entry is:
// ASID=4095 VPN=0 V=0
//
// This means ASID 4095 is unusable if access to the zeroth page is required.

#define TB_INVALID_ENTRY 0xFFF0000000000000

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

#define XrReadByte(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 1, 0);
#define XrReadInt(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 2, 0);
#define XrReadLong(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 4, 0);

#define XrWriteByte(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 1, 0);
#define XrWriteInt(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 2, 0);
#define XrWriteLong(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 4, 0);

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

	proc->IcReplacementIndex = 0;
	proc->DcReplacementIndex = 0;

#ifdef PROFCPU
	proc->IcMissCount = 0;
	proc->IcHitCount = 0;

	proc->DcMissCount = 0;
	proc->DcHitCount = 0;

	proc->TimeToNextPrint = 0;
#endif

	proc->WbWriteIndex = 0;
	proc->WbCycles = 0;
	proc->WbFillIndex = 0;

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		proc->WbIndices[i] = XR_CACHE_INDEX_INVALID;
	}

	for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
		proc->DcIndexToWbIndex[i] = XR_WB_INDEX_INVALID;
	}

	proc->StallCycles = 0;

	proc->NmiMaskCounter = NMI_MASK_CYCLES;
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

	DBGPRINT("exc %d\n", exc);
	// proc->Running = false;

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

	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	// Reset the "progress", allowing more polling.

	proc->Progress = XR_POLL_MAX;
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

static inline uint8_t XrNoncachedAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length) {
	proc->StallCycles += XR_UNCACHED_STALL;

	int result;

	XrLockIoMutex(proc, address);

	if (dest) {
		result = EBusRead(address, dest, length);
	} else {
		result = EBusWrite(address, &srcvalue, length);
	}

	XrUnlockIoMutex(address);

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

static inline uint8_t XrIcacheAccess(XrProcessor *proc, uint32_t address, uint32_t *dest) {
	// Access Icache. Quite fast, don't need to take any locks or anything
	// as there is no coherence, plus its always a 32-bit read, so we
	// duplicate some logic here as a fast path.

	uint32_t tag = address & ~(XR_IC_LINE_SIZE - 1);
	uint32_t lineoffset = address & (XR_IC_LINE_SIZE - 1);

	if (tag == proc->IcLastTag) {
		// Matches the lookup hint, nice.

#ifdef PROFCPU
		proc->IcHitCount++;
#endif

		*dest = *(uint32_t*)(&proc->Ic[proc->IcLastOffset + lineoffset]);

		return 1;
	}

	uint32_t setnumber = (address >> XR_IC_LINE_SIZE_LOG) & (XR_IC_SETS - 1);
	uint32_t cacheindex = setnumber << XR_IC_WAY_LOG;

	for (int i = 0; i < XR_IC_WAYS; i++) {
		if (proc->IcFlags[cacheindex + i] && proc->IcTags[cacheindex + i] == tag) {
			// Found it!

#ifdef PROFCPU
			proc->IcHitCount++;
#endif

			uint32_t cacheoff = (cacheindex + i) << XR_IC_LINE_SIZE_LOG;

			*dest = *(uint32_t*)(&proc->Ic[cacheoff + lineoffset]);

			proc->IcLastTag = tag;
			proc->IcLastOffset = cacheoff;

			return 1;
		}
	}

#ifdef PROFCPU
	proc->IcMissCount++;
#endif

	// Unfortunately there was a miss. Incur a penalty.

	proc->StallCycles += XR_MISS_STALL;

	// Replace a random-ish line within the set.

	uint32_t newindex = cacheindex + (proc->IcReplacementIndex & (XR_IC_WAYS - 1));
	proc->IcReplacementIndex += 1;

	uint32_t cacheoff = newindex << XR_IC_LINE_SIZE_LOG;

	XrLockIoMutex(proc, tag);
	int result = EBusRead(tag, &proc->Ic[cacheoff], XR_IC_LINE_SIZE);
	XrUnlockIoMutex(tag);

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

static inline void XrFlushWriteBuffer(XrProcessor *proc) {
	// Flush the write buffer of the given processor.

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		uint32_t index = proc->WbIndices[i];

		if (index != XR_CACHE_INDEX_INVALID) {
			// Found an entry. Try locking the corresponding tag.

			uint32_t tag = proc->DcTags[index];

			XrLockCache(proc, tag);

			// Check if the buffer entry is still valid. It may have been
			// flushed out due to cache invalidation.

			if (proc->WbIndices[i] == XR_CACHE_INDEX_INVALID) {
				// It was flushed.

				XrUnlockCache(proc, tag);

				continue;
			}

			// Write out the entry.

			DBGPRINT("flush wb write %x from cacheindex %d\n", tag, index);

			XrLockIoMutex(proc, tag);
			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE);
			XrUnlockIoMutex(tag);

			// Invalidate it.

			proc->WbIndices[i] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;

			// Unlock the tag.

			XrUnlockCache(proc, tag);
		}
	}
}

static inline uint32_t XrFindInScache(uint32_t tag) {
	// Find a tag in the Scache.

	uint32_t setnumber = (tag >> XR_SC_LINE_SIZE_LOG) & (XR_SC_SETS - 1);
	uint32_t cacheindex = setnumber << XR_SC_WAY_LOG;

	for (int i = 0; i < XR_SC_WAYS; i++) {
		DBGPRINT("scache search %d: flags=%d tag=%x\n", cacheindex+i, XrScacheFlags[cacheindex+i], XrScacheTags[cacheindex+i]);

		if (XrScacheFlags[cacheindex + i] && XrScacheTags[cacheindex + i] == tag) {
			// Found it!

			DBGPRINT("found %x at scache %d\n", tag, cacheindex + i);

			return cacheindex + i;
		}
	}

	return -1;
}

static inline void XrDowngradeLine(XrProcessor *proc, uint32_t tag, uint32_t newstate) {
	// Invalidate a line if found in the given processor's Dcache.

	uint32_t setnumber = (tag >> XR_DC_LINE_SIZE_LOG) & (XR_DC_SETS - 1);
	uint32_t cacheindex = setnumber << XR_DC_WAY_LOG;

	XrLockCache(proc, tag);

	// Find it in the Dcache.

	for (int i = 0; i < XR_DC_WAYS; i++) {
		DBGPRINT("search %d %x %d\n", cacheindex + i, proc->DcTags[cacheindex + i], proc->DcFlags[cacheindex + i]);

		uint32_t index = cacheindex + i;

		if (proc->DcFlags[index] > newstate && proc->DcTags[index] == tag) {
			// Found it! Kill it.

			DBGPRINT("found to inval %x %d %d\n", tag, proc->DcFlags[cacheindex + i], cacheindex + i);

			if (proc->DcFlags[index] == XR_LINE_EXCLUSIVE) {
				// Find it in the writebuffer.

				uint32_t wbindex = proc->DcIndexToWbIndex[index];

				if (wbindex != XR_WB_INDEX_INVALID) {
					// Force a write out.

					DBGPRINT("forced wb write %x from cacheindex %d\n", tag, index);

					XrLockIoMutex(proc, tag);
					EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE);
					XrUnlockIoMutex(tag);

					// Invalidate.

					proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
					proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
				}
			}

			proc->DcFlags[index] = newstate;

			break;
		}
	}

	XrUnlockCache(proc, tag);
}

static inline void XrDowngradeAll(XrProcessor *thisproc, uint32_t tag, uint32_t newstate) {
	// Invalidate a Dcache line in all processors except the one provided.

	for (int i = 0; i < XrProcessorCount; i++) {
		XrProcessor *proc = CpuTable[i];

		if (proc == thisproc) {
			continue;
		}

		XrDowngradeLine(proc, tag, newstate);
	}
}

static inline uint32_t XrFindOrReplaceInScache(XrProcessor *thisproc, uint32_t tag, uint32_t newstate, uint32_t *created) {
	// Find a tag in the Scache, or replace one if not found.

	*created = 0;

	uint32_t setnumber = (tag >> XR_SC_LINE_SIZE_LOG) & (XR_SC_SETS - 1);
	uint32_t cacheindex = setnumber << XR_SC_WAY_LOG;

	for (int i = 0; i < XR_SC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (XrScacheFlags[index] && XrScacheTags[index] == tag) {
			// Found it!

			DBGPRINT("scache hit %x\n", tag);

			return index;
		}
	}

	// Didn't find it so drop locks and select one to replace.

	XrUnlockScache(tag);

	// Grab the global Scache replacement lock.

	XrLockScacheReplacement();

	// In the meantime someone might have filled it so we have to re-check with
	// both the replacement lock and the tag lock.

	XrLockScache(tag);

	for (int i = 0; i < XR_SC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (XrScacheFlags[index] && XrScacheTags[index] == tag) {
			// Found it!

			XrUnlockScacheReplacement();

			DBGPRINT("scache retry hit %x\n", tag);

			return index;
		}
	}

	XrUnlockScache(tag);

	// Still not there, so, we really are gonna replace one.

	cacheindex += XrScacheReplacementIndex & (XR_SC_WAYS - 1);
	XrScacheReplacementIndex += 1;

restart:

	if (XrScacheFlags[cacheindex] == XR_LINE_INVALID) {
		// It's invalid, we can just take it.

	} else {
		// Lock the tag in this selected entry.

		uint32_t oldtag = XrScacheTags[cacheindex];

		DBGPRINT("scache take %x\n", tag);

		XrLockScache(oldtag);

		// Check if state changed. We'll have to restart if it did.

		// Impossible for tag to have changed since that implies a new line was
		// added, which is blocked out by the replacement lock.

#if 0
		if (XrScacheTags[cacheindex] != oldtag) {
			XrUnlockScache(oldtag);

			goto restart;
		}
#endif

		if (XrScacheFlags[cacheindex] == XR_LINE_INVALID) {
			XrUnlockScache(oldtag);

			DBGPRINT("scache retry invalid %x\n", oldtag);

			goto restart;
		}

		if (XrScacheFlags[cacheindex] == XR_LINE_SHARED) {
			// We have to remove it from everyone's Dcache, including mine.

			DBGPRINT("scache steal %x\n", oldtag);

			XrDowngradeAll(0, oldtag, XR_LINE_INVALID);

		} else if (XrScacheFlags[cacheindex] == XR_LINE_EXCLUSIVE) {
			// Remove it from the owner's Dcache.

			DBGPRINT("scache steal from exclusive %x\n", oldtag);

			XrDowngradeLine(CpuTable[XrScacheExclusiveIds[cacheindex]], oldtag, XR_LINE_INVALID);
		}

		// Mark invalid for the time where we have no tags locked.

		XrScacheFlags[cacheindex] = XR_LINE_INVALID;

		XrUnlockScache(oldtag);
	}

	XrLockScache(tag);

	XrScacheFlags[cacheindex] = newstate;
	XrScacheTags[cacheindex] = tag;
	XrScacheExclusiveIds[cacheindex] = thisproc->Id;

	DBGPRINT("scache fill %x\n", tag);

	XrUnlockScacheReplacement();

	*created = 1;

	return cacheindex;
}

static inline uint8_t XrDcacheAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, uint8_t forceexclusive) {
	// Access Dcache. Scary! We have to worry about coherency with the other
	// Dcaches in the system, and this can be a 1, 2, or 4 byte access.
	// If dest == 0, this is a write. Otherwise it's a read.

	uint32_t tag = address & ~(XR_DC_LINE_SIZE - 1);
	uint32_t lineoffset = address & (XR_DC_LINE_SIZE - 1);
	uint32_t setnumber = (tag >> XR_DC_LINE_SIZE_LOG) & (XR_DC_SETS - 1);
	uint32_t cacheindex = setnumber << XR_DC_WAY_LOG;
	uint32_t freewbindex = -1;

restart:

	// Lock the cache tag.

	XrLockCache(proc, tag);

	if (dest == 0) {
		// This is a write; find a write buffer entry.

		for (int i = 0; i < XR_WB_DEPTH; i++) {
			uint32_t index = proc->WbIndices[i];

			// DBGPRINT("%x\n", index);

			if (index == XR_CACHE_INDEX_INVALID) {
				freewbindex = i;

			} else if (proc->DcTags[index] == tag) {
				// Found it. We can quickly merge the write into the cache line
				// and continue.

				// Note that this means we have the cache line exclusive as the
				// writebuffer entry should be purged with the tag locked
				// whenever the line is invalidated or downgraded.

				uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

				CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

				XrUnlockCache(proc, tag);

				// DBGPRINT("write hit %x merge in writebuffer\n", tag);

				return 1;
			}
		}

		// Failed to find it in the write buffer.

		if (freewbindex == -1) {
			// Write buffer is full. Flush and retry.

			XrUnlockCache(proc, tag);

			proc->StallCycles += XR_UNCACHED_STALL * XR_WB_DEPTH;

			XrFlushWriteBuffer(proc);

			DBGPRINT("flush writebuffer full %x\n", tag);

			goto restart;
		}

		// We're going to insert an entry into the writebuffer, so start the
		// writebuffer write countdown.

		if (proc->WbCycles == 0) {
			proc->WbCycles = XR_UNCACHED_STALL;
		}
	}

	// Look up the cache line.

	for (int i = 0; i < XR_DC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (proc->DcFlags[index] && proc->DcTags[index] == tag) {
			// Cache hit.

			uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

#ifdef PROFCPU
			proc->DcHitCount += 1;
#endif

			if (dest != 0) {
				// We're reading. Copy out the data.

				CopyWithLengthZext(dest, &proc->Dc[cacheoff + lineoffset], length);

				XrUnlockCache(proc, tag);

				// DBGPRINT("read hit %x\n", tag);

				return 1;
			}

			if (proc->DcFlags[index] == XR_LINE_EXCLUSIVE) {
				// We're writing. We got a write buffer index earlier so set
				// it as representing this cache line.

				proc->WbIndices[freewbindex] = index;
				proc->DcIndexToWbIndex[index] = freewbindex;

				// Now merge in the data.

				CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

				XrUnlockCache(proc, tag);

				// DBGPRINT("write hit %x already exclusive\n", tag);

				return 1;
			}

			// This is a write and the cache line is shared. We need to remove
			// it from everybody else's cache, and set it to exclusive in ours.

			// Since we're doing a global operation we need to drop our lock and
			// acquire the Scache lock.

			DBGPRINT("write hit %x to exclusive from shared\n", tag);

			XrUnlockCache(proc, tag);

			// If the cache line is shared that actually doesn't violate the SC
			// condition, since that means nobody else wrote to the cache line
			// in the interim since LL.

#if 0
			if (forceexclusive && dest == 0) {
				// We failed the SC condition, since it was only shared, not
				// exclusive.

				return 2;
			}
#endif

			XrLockScache(tag);

			// While locks were dropped the state of our line might have
			// changed (namely, it may have been invalidated).

			if (proc->DcFlags[index] == XR_LINE_INVALID) {
				// It was invalidated. Unlock and start all over.

				DBGPRINT("invalid retry %x\n", tag);

				XrUnlockScache(tag);

				goto restart;
			}

			// Look up the Scache line.

			uint32_t scacheindex = XrFindInScache(tag);

			// If it's valid in our cache, it must be valid in the Scache,
			// since our cache is maintained as a subset of the Scache.

			if (scacheindex == -1) {
				fprintf(stderr, "Not found in scache %08x\n", address);
				abort();
			}

			// Downgrade all matching lines to INVALID.

			if (XrScacheFlags[scacheindex] == XR_LINE_SHARED) {
				// Really have to search everyone.

				DBGPRINT("steal on write upgrade %x\n", tag);

				XrDowngradeAll(proc, tag, XR_LINE_INVALID);

			} else if (XrScacheFlags[scacheindex] == XR_LINE_EXCLUSIVE) {
				// Since it's exclusive it can only be in one Dcache.

				DBGPRINT("steal exclusive on write upgrade %x\n", tag);

				XrDowngradeLine(CpuTable[XrScacheExclusiveIds[scacheindex]], tag, XR_LINE_INVALID);
			}

			// Set the new cache states.

			XrScacheFlags[scacheindex] = XR_LINE_EXCLUSIVE;
			XrScacheExclusiveIds[scacheindex] = proc->Id;
			proc->DcFlags[index] = XR_LINE_EXCLUSIVE;

			// We got a write buffer index earlier so set
			// it as representing this cache line.

			proc->WbIndices[freewbindex] = index;
			proc->DcIndexToWbIndex[index] = freewbindex;

			// Now merge in the data.

			CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

			XrUnlockScache(tag);

			return 1;
		}
	}

	DBGPRINT("miss on %x\n", tag);

	// Cache miss. Unlock our cache.

	XrUnlockCache(proc, tag);

	if (forceexclusive) {
		// We failed the SC condition since it was invalid.

		return 2;
	}

#ifdef PROFCPU
	proc->DcMissCount += 1;
#endif

	// If this is a write, we want the line exclusive; otherwise shared.

	uint32_t newstate = (dest == 0) ? XR_LINE_EXCLUSIVE : XR_LINE_SHARED;

	// If this is a write, we want the line invalid in everyone else; otherwise
	// shared.

	uint32_t otherstate = (dest == 0) ? XR_LINE_INVALID : XR_LINE_SHARED;

	// Lock the Scache.

	XrLockScache(tag);

	// Look up the tag.

	uint32_t created;

	uint32_t scacheindex = XrFindOrReplaceInScache(proc, tag, newstate, &created);

	if (created == 0) {
		// The line wasn't created for us. It already existed in the Scache and
		// we might need to do something about it.

		if (XrScacheFlags[scacheindex] == XR_LINE_EXCLUSIVE) {
			// We have to remove it from this cache who has it exclusive.

			DBGPRINT("remove exclusive %x after miss\n", tag);

			XrDowngradeLine(CpuTable[XrScacheExclusiveIds[scacheindex]], tag, otherstate);

		} else if (dest == 0 && XrScacheFlags[scacheindex] == XR_LINE_SHARED) {
			// We're writing and this line was shared. We have to invalidate it
			// in everybody.

			DBGPRINT("remove shared %x after miss\n", tag);

			XrDowngradeAll(0, tag, XR_LINE_INVALID);
		}

		XrScacheFlags[scacheindex] = newstate;
		XrScacheExclusiveIds[scacheindex] = proc->Id;
	}

	// The line is now in the desired state in the Scache (and in everyone
	// else's Dcache).

	// We have to select a Dcache line to replace now.

	uint32_t index = cacheindex + (proc->DcReplacementIndex & (XR_DC_WAYS - 1));
	proc->DcReplacementIndex += 1;

recheck:

	if (proc->DcFlags[index] == XR_LINE_INVALID) {
		// It's invalid, we can just take it.

	} else {
		// Lock the tag in this selected entry.

		uint32_t oldtag = proc->DcTags[index];

		XrLockCache(proc, oldtag);

		// Check if state changed. We'll have to restart if it did.

		if (proc->DcFlags[index] == XR_LINE_INVALID) {
			XrUnlockCache(proc, oldtag);

			goto recheck;
		}

		DBGPRINT("reclaim %x after miss\n", oldtag);

		if (proc->DcFlags[index] == XR_LINE_EXCLUSIVE) {
			// See if it's in the writebuffer. If so we'll have to force a
			// write.

			// Find it in the writebuffer.

			uint32_t wbindex = proc->DcIndexToWbIndex[index];

			if (wbindex != XR_WB_INDEX_INVALID) {
				// Force a write out.

				DBGPRINT("forced self wb write %x from cacheindex %d\n", tag, index);

				XrLockIoMutex(proc, tag);
				EBusWrite(oldtag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE);
				XrUnlockIoMutex(tag);

				// Invalidate.

				proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
				proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
			}
		}

		// Mark invalid for the time where we have no tags locked.

		proc->DcFlags[index] = XR_LINE_INVALID;

		XrUnlockCache(proc, oldtag);
	}

	XrLockCache(proc, tag);

	// Initialize the cache line.

	DBGPRINT("new cacheline setnumber=%x cacheindex=%x index=%x out of %x\n", setnumber, cacheindex, index, XR_DC_LINE_COUNT);

	proc->DcFlags[index] = newstate;
	proc->DcTags[index] = tag;

	// Incur a stall.

	proc->StallCycles += XR_MISS_STALL;

	// Read in the cache line contents.

	uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

	XrLockIoMutex(proc, tag);
	int result = EBusRead(tag, &proc->Dc[cacheoff], XR_DC_LINE_SIZE);
	XrUnlockIoMutex(tag);

	if (result == EBUSERROR) {
		// We failed. Back out.

		XrScacheFlags[scacheindex] = XR_LINE_INVALID;
		proc->DcFlags[index] = XR_LINE_INVALID;

		XrUnlockCache(proc, tag);
		XrUnlockScache(tag);

		DBGPRINT("bus error on %x\n", tag);

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS);

		return 0;
	}

	XrUnlockScache(tag);

	if (dest == 0) {
		// We're writing. We got a write buffer index earlier so set
		// it as representing this cache line.

		proc->WbIndices[freewbindex] = index;
		proc->DcIndexToWbIndex[index] = freewbindex;

		// Now merge in the data.

		CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

		// DBGPRINT("write %x after miss\n", tag);

		XrUnlockCache(proc, tag);

		return 1;

	}

	// We're reading.

	// DBGPRINT("read %x after miss\n", tag);

	CopyWithLengthZext(dest, &proc->Dc[cacheoff + lineoffset], length);

	XrUnlockCache(proc, tag);

	return 1;
}

static uint8_t XrAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, uint8_t forceexclusive) {
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
	} else if (address >= XR_NONCACHED_PHYS_BASE) {
		cachetype = NONCACHED;
	}

	if (!XrSimulateCaches) {
		cachetype = NONCACHED;
	}

	if (cachetype == NONCACHED) {
		if (forceexclusive && XrSimulateCaches) {
			// Attempt to use LL/SC on noncached memory. Nonsense! Just kill the
			// evildoer with a bus error.

			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS);

			return 0;
		}

		return XrNoncachedAccess(proc, address, dest, srcvalue, length);
	}

	if (proc->IFetch) {
		return XrIcacheAccess(proc, address, dest);
	}

	return XrDcacheAccess(proc, address, dest, srcvalue, length, forceexclusive);
}

static inline void XrWriteWbEntry(XrProcessor *proc) {
	// Write out a write buffer entry starting at the WriteIndex.

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		int wbindex = (proc->WbWriteIndex + i) % XR_WB_DEPTH;

		uint32_t index = proc->WbIndices[wbindex];

		if (index != XR_CACHE_INDEX_INVALID) {
			// Found an entry. Try locking the corresponding tag.

			uint32_t tag = proc->DcTags[index];

			XrLockCache(proc, tag);

			// Check if the buffer entry is still valid. It may have been
			// flushed out due to cache invalidation.

			if (proc->WbIndices[wbindex] == XR_CACHE_INDEX_INVALID) {
				// It was flushed.

				XrUnlockCache(proc, tag);

				continue;
			}

			// Write out the entry.

			DBGPRINT("timed wb write %x from cacheindex %d\n", tag, index);

			XrLockIoMutex(proc, tag);
			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE);
			XrUnlockIoMutex(tag);

			// Invalidate it.

			proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;

			// Unlock the tag.

			XrUnlockCache(proc, tag);

			// Set the write index.

			proc->WbWriteIndex = i + 1;

			// Reset the write cycle counter.

			proc->WbCycles = XR_UNCACHED_STALL;

			// Break out.

			break;
		}
	}
}

static inline uint32_t XrComputeValue(uint32_t val, uint32_t ir) {
	// Compute a value based on the inline shift expressed in the instruction.

	uint32_t shifttype = (ir >> 26) & 3;
	uint32_t shift = (ir >> 21) & 31;

	switch(shifttype) {
		case 0: // LSH
			return val << shift;

		case 1: // RSH
			return val >> shift;

		case 2: // ASH
			return (int32_t) val >> shift;

		case 3: // ROR
			return RoR(val, shift);
	}

	return val;
}

typedef void (*XrInstructionF)(XrProcessor *proc, uint32_t currentpc, uint32_t ir);

static void XrIllegalInstruction(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrBasicException(proc, XR_EXC_INV);
}

static void XrNor(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = ~(proc->Reg[ra] | val);
}

static void XrOr(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = proc->Reg[ra] | val;
}

static void XrXor(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = proc->Reg[ra] ^ val;
}

static void XrAnd(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = proc->Reg[ra] & val;
}

static void XrSltSigned(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	if ((int32_t) proc->Reg[ra] < (int32_t) val)
		proc->Reg[rd] = 1;
	else
		proc->Reg[rd] = 0;
}

static void XrSlt(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	if (proc->Reg[ra] < val)
		proc->Reg[rd] = 1;
	else
		proc->Reg[rd] = 0;
}

static void XrSub(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = proc->Reg[ra] - val;
}

static void XrAdd(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	proc->Reg[rd] = proc->Reg[ra] + val;
}

static void XrRegShifts(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	uint32_t shifttype = (ir >> 26) & 3;

	switch(shifttype) {
		case 0: // LSH
			if (proc->Reg[ra] >= 32) {
				proc->Reg[rd] = 0;
			} else {
				proc->Reg[rd] = proc->Reg[rb] << proc->Reg[ra];
			}

			break;

		case 1: // RSH
			if (proc->Reg[ra] >= 32) {
				proc->Reg[rd] = 0;
			} else {
				proc->Reg[rd] = proc->Reg[rb] >> proc->Reg[ra];
			}

			break;

		case 2: // ASH
			if (proc->Reg[ra] >= 32) {
				if (proc->Reg[rb] & 0x80000000) {
					proc->Reg[rd] = 0xFFFFFFFF;
				} else {
					proc->Reg[rd] = 0;
				}
			} else {
				proc->Reg[rd] = (int32_t) proc->Reg[rb] >> proc->Reg[ra];
			}

			break;

		case 3: // ROR
			proc->Reg[rd] = RoR(proc->Reg[rb], proc->Reg[ra]);
			break;
	}
}

static void XrStoreLongRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrWriteLong(proc, proc->Reg[ra] + val, proc->Reg[rd]);
}

static void XrStoreIntRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrWriteInt(proc, proc->Reg[ra] + val, proc->Reg[rd]);
}

static void XrStoreByteRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrWriteByte(proc, proc->Reg[ra] + val, proc->Reg[rd]);
}

static void XrLoadLongRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrReadLong(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
}

static void XrLoadIntRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrReadInt(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
}

static void XrLoadByteRegOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;
	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	XrReadByte(proc, proc->Reg[ra] + val, &proc->Reg[rd]);
}

static XrInstructionF XrFunctions111001[16] = {
	[0] = &XrNor,
	[1] = &XrOr,
	[2] = &XrXor,
	[3] = &XrAnd,
	[4] = &XrSltSigned,
	[5] = &XrSlt,
	[6] = &XrSub,
	[7] = &XrAdd,
	[8] = &XrRegShifts,
	[9] = &XrStoreLongRegOffset,
	[10] = &XrStoreIntRegOffset,
	[11] = &XrStoreByteRegOffset,
	[12] = &XrIllegalInstruction,
	[13] = &XrLoadLongRegOffset,
	[14] = &XrLoadIntRegOffset,
	[15] = &XrLoadByteRegOffset,
};

static void XrLookup111001(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrFunctions111001[ir >> 28](proc, currentpc, ir);
}

static void XrSys(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrBasicInbetweenException(proc, XR_EXC_SYS);
}

static void XrBrk(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrBasicInbetweenException(proc, XR_EXC_BRK);
}

static void XrWmb(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	if (XrSimulateCaches) {
		// Flush the write buffer.

		XrFlushWriteBuffer(proc);
	}
}

static void XrMb(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrWmb(proc, currentpc, ir);
}

static void XrSC(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	if (!proc->Locked) {
		// Something happened on this processor that caused the
		// lock to go away.

		proc->Reg[rd] = 0;

		return;
	}

	// Store the word in a way that will atomically fail if we
	// no longer have the cache line from LL's load.

	//printf("%d: SC %d\n", proc->Id, proc->Reg[rb]);

	uint8_t status = XrAccess(proc, proc->Reg[ra], 0, proc->Reg[rb], 4, 1);

	if (!status) {
		return;
	}

	proc->Reg[rd] = (status == 2) ? 0 : 1;
}

static void XrLL(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;

	proc->Locked = 1;

	XrReadLong(proc, proc->Reg[ra], &proc->Reg[rd]);
}

static void XrMod(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	if (val == 0) {
		proc->Reg[rd] = 0;
		return;
	}

	proc->Reg[rd] = proc->Reg[ra] % val;
}

static void XrDivSigned(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	if (val == 0) {
		proc->Reg[rd] = 0;
		return;
	}

	proc->Reg[rd] = (int32_t) proc->Reg[ra] / (int32_t) val;
}

static void XrDiv(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	uint32_t val = XrComputeValue(proc->Reg[rb], ir);

	if (val == 0) {
		proc->Reg[rd] = 0;
		return;
	}

	proc->Reg[rd] = proc->Reg[ra] / val;
}

static void XrMul(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

	proc->Reg[rd] = proc->Reg[ra] * XrComputeValue(proc->Reg[rb], ir);
}

static XrInstructionF XrFunctions110001[16] = {
	[0] = &XrSys,
	[1] = &XrBrk,
	[2] = &XrWmb,
	[3] = &XrMb,
	[4] = &XrIllegalInstruction,
	[5] = &XrIllegalInstruction,
	[6] = &XrIllegalInstruction,
	[7] = &XrIllegalInstruction,
	[8] = &XrSC,
	[9] = &XrLL,
	[10] = &XrIllegalInstruction,
	[11] = &XrMod,
	[12] = &XrDivSigned,
	[13] = &XrDiv,
	[14] = &XrIllegalInstruction,
	[15] = &XrMul,
};

static void XrLookup110001(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrFunctions110001[ir >> 28](proc, currentpc, ir);
}

static void XrRfe(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	proc->Locked = 0;

	if (proc->Cr[RS] & RS_TBMISS) {
		proc->Pc = proc->Cr[TBPC];
	} else {
		proc->Pc = proc->Cr[EPC];
	}

	proc->Cr[RS] = (proc->Cr[RS] & 0xF0000000) | ((proc->Cr[RS] >> 8) & 0xFFFF);
}

static void XrHlt(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	proc->Halted = true;
}

static void XrMtcr(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	uint32_t ra = (ir >> 11) & 31;
	uint32_t rb = (ir >> 16) & 31;

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
				// Icache. We can use clever math to only
				// search the cache lines that lines within
				// this page frame could possibly be in.

				uint32_t phys = proc->Reg[ra] & 0xFFFFF000;

				uint32_t lowindex = ((phys >> XR_IC_LINE_SIZE_LOG) & (XR_IC_SETS - 1)) << XR_IC_WAY_LOG;
				uint32_t highindex = (((phys + 4096) >> XR_IC_LINE_SIZE_LOG) & (XR_IC_SETS - 1)) << XR_IC_WAY_LOG;

				for (int i = lowindex; i < highindex; i++) {
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

			XrFlushWriteBuffer(proc);

			if ((proc->Reg[ra] & 3) == 3) {
				// Invalidate the entire Dcache.

				for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
					proc->DcFlags[i] = XR_LINE_INVALID;
				}
			} else if ((proc->Reg[ra] & 3) == 2) {
				// Invalidate a single page frame of the
				// Dcache. We can use clever math to only
				// search the cache lines that lines within
				// this page frame could possibly be in.

				uint32_t phys = proc->Reg[ra] & 0xFFFFF000;

				uint32_t lowindex = ((phys >> XR_DC_LINE_SIZE_LOG) & (XR_DC_SETS - 1)) << XR_DC_WAY_LOG;
				uint32_t highindex = (((phys + 4096) >> XR_DC_LINE_SIZE_LOG) & (XR_DC_SETS - 1)) << XR_DC_WAY_LOG;

				for (int i = lowindex; i < highindex; i++) {
					if ((proc->DcTags[i] & 0xFFFFF000) == phys) {
						proc->DcFlags[i] = XR_LINE_INVALID;
					}
				}
			}

			break;

		case ITBCTRL:
			if ((proc->Reg[ra] & 3) == 3) {
				// Invalidate the entire ITB.

				for (int i = 0; i < XR_ITB_SIZE; i++) {
					proc->Itb[i] = TB_INVALID_ENTRY;
				}
			} else if ((proc->Reg[ra] & 3) == 2) {
				// Invalidate the entire ITB except for
				// global and reserved entries.

				for (int i = 4; i < XR_ITB_SIZE; i++) {
					if ((proc->Itb[i] & PTE_GLOBAL) == 0) {
						proc->Itb[i] = TB_INVALID_ENTRY;
					}
				}
			} else if ((proc->Reg[ra] & 3) == 1) {
				// Invalidate the entire ITB except for
				// reserved entries.

				for (int i = 4; i < XR_ITB_SIZE; i++) {
					proc->Itb[i] = TB_INVALID_ENTRY;
				}
			} else if ((proc->Reg[ra] & 3) == 0) {
				// Invalidate a single page in the ITB.

				uint64_t vpn = (uint64_t)(proc->Reg[ra] >> 12) << 32;

				for (int i = 0; i < XR_ITB_SIZE; i++) {
					if ((proc->Itb[i] & 0x000FFFFF00000000) == vpn) {
						proc->Itb[i] = TB_INVALID_ENTRY;
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
					proc->Dtb[i] = TB_INVALID_ENTRY;
				}
			} else if ((proc->Reg[ra] & 3) == 2) {
				// Invalidate the entire DTB except for
				// global and reserved entries.

				for (int i = 4; i < XR_DTB_SIZE; i++) {
					if ((proc->Dtb[i] & PTE_GLOBAL) == 0) {
						proc->Dtb[i] = TB_INVALID_ENTRY;
					}
				}
			} else if ((proc->Reg[ra] & 3) == 1) {
				// Invalidate the entire DTB except for
				// reserved entries.

				for (int i = 4; i < XR_DTB_SIZE; i++) {
					proc->Dtb[i] = TB_INVALID_ENTRY;
				}
			} else if ((proc->Reg[ra] & 3) == 0) {
				// Invalidate a single page in the DTB.

				uint64_t vpn = (uint64_t)(proc->Reg[ra] >> 12) << 32;

				for (int i = 0; i < XR_DTB_SIZE; i++) {
					if ((proc->Dtb[i] & 0x000FFFFF00000000) == vpn) {
						proc->Dtb[i] = TB_INVALID_ENTRY;
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
}

static void XrMfcr(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	// Reset the NMI mask counter.

	uint32_t rd = (ir >> 6) & 31;
	uint32_t rb = (ir >> 16) & 31;

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	proc->Reg[rd] = proc->Cr[rb];
}

static XrInstructionF XrFunctions101001[16] = {
	[0] = &XrIllegalInstruction,
	[1] = &XrIllegalInstruction,
	[2] = &XrIllegalInstruction,
	[3] = &XrIllegalInstruction,
	[4] = &XrIllegalInstruction,
	[5] = &XrIllegalInstruction,
	[6] = &XrIllegalInstruction,
	[7] = &XrIllegalInstruction,
	[8] = &XrIllegalInstruction,
	[9] = &XrIllegalInstruction,
	[10] = &XrIllegalInstruction,
	[11] = &XrRfe,
	[12] = &XrHlt,
	[13] = &XrIllegalInstruction,
	[14] = &XrMtcr,
	[15] = &XrMfcr,
};

static void XrLookup101001(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	if (proc->Cr[RS] & RS_USER) {
		// Current mode is usermode, so cause a privilege violation
		// exception.

		XrBasicException(proc, XR_EXC_PRV);
	}

	XrFunctions101001[ir >> 28](proc, currentpc, ir);
}

static void XrBpo(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if (proc->Reg[rd] & 1) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBpe(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if ((proc->Reg[rd] & 1) == 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBge(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if ((int32_t) proc->Reg[rd] >= 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBle(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if ((int32_t) proc->Reg[rd] <= 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBgt(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if ((int32_t) proc->Reg[rd] > 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBlt(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if ((int32_t) proc->Reg[rd] < 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBne(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if (proc->Reg[rd] != 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrBeq(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;

	if (proc->Reg[rd] == 0) {
		proc->Pc = currentpc + SignExt23((ir >> 11) << 2);
	}
}

static void XrLui(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] | (imm << 16);
}

static void XrOri(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] | imm;
}

static void XrXori(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] ^ imm;
}

static void XrAndi(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] & imm;
}

static void XrSltiSigned(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	if ((int32_t) proc->Reg[ra] < (int32_t) SignExt16(imm)) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}
}

static void XrSlti(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	if (proc->Reg[ra] < imm) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}
}

static void XrSubi(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] - imm;
}

static void XrAddi(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Reg[ra] + imm;
}

static void XrStoreLongImmOffsetImm(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteLong(proc, proc->Reg[rd] + (imm << 2), SignExt5(ra));
}

static void XrStoreIntImmOffsetImm(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteInt(proc, proc->Reg[rd] + (imm << 1), SignExt5(ra));
}

static void XrStoreByteImmOffsetImm(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteByte(proc, proc->Reg[rd] + imm, SignExt5(ra));
}

static void XrStoreLongImmOffsetReg(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteLong(proc, proc->Reg[rd] + (imm << 2), proc->Reg[ra]);
}

static void XrStoreIntImmOffsetReg(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteInt(proc, proc->Reg[rd] + (imm << 1), proc->Reg[ra]);
}

static void XrStoreByteImmOffsetReg(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrWriteByte(proc, proc->Reg[rd] + imm, proc->Reg[ra]);
}

static void XrLoadLongImmOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	XrReadLong(proc, proc->Reg[ra] + (imm << 2), &proc->Reg[rd]);
}

static void XrLoadIntImmOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;
	
	XrReadInt(proc, proc->Reg[ra] + (imm << 1), &proc->Reg[rd]);
}

static void XrLoadByteImmOffset(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;
	
	XrReadByte(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);
}

static void XrJalr(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	uint32_t rd = (ir >> 6) & 31;
	uint32_t ra = (ir >> 11) & 31;
	uint32_t imm = ir >> 16;

	proc->Reg[rd] = proc->Pc;
	proc->Pc = proc->Reg[ra] + SignExt18(imm << 2);
}

static XrInstructionF XrLowSix[64] = {
	[0] = &XrIllegalInstruction,
	[1] = &XrIllegalInstruction,
	[2] = &XrIllegalInstruction,
	[3] = &XrIllegalInstruction,
	[4] = &XrLui,
	[5] = &XrBpo,
	[6] = &XrIllegalInstruction,
	[7] = &XrIllegalInstruction,
	[8] = &XrIllegalInstruction,
	[9] = &XrIllegalInstruction,
	[10] = &XrStoreLongImmOffsetImm,
	[11] = &XrIllegalInstruction,
	[12] = &XrOri,
	[13] = &XrBpe,
	[14] = &XrIllegalInstruction,
	[15] = &XrIllegalInstruction,
	[16] = &XrIllegalInstruction,
	[17] = &XrIllegalInstruction,
	[18] = &XrStoreIntImmOffsetImm,
	[19] = &XrIllegalInstruction,
	[20] = &XrXori,
	[21] = &XrBge,
	[22] = &XrIllegalInstruction,
	[23] = &XrIllegalInstruction,
	[24] = &XrIllegalInstruction,
	[25] = &XrIllegalInstruction,
	[26] = &XrStoreByteImmOffsetImm,
	[27] = &XrIllegalInstruction,
	[28] = &XrAndi,
	[29] = &XrBle,
	[30] = &XrIllegalInstruction,
	[31] = &XrIllegalInstruction,
	[32] = &XrIllegalInstruction,
	[33] = &XrIllegalInstruction,
	[34] = &XrIllegalInstruction,
	[35] = &XrIllegalInstruction,
	[36] = &XrSltiSigned,
	[37] = &XrBgt,
	[38] = &XrIllegalInstruction,
	[39] = &XrIllegalInstruction,
	[40] = &XrIllegalInstruction,
	[41] = &XrLookup101001,
	[42] = &XrStoreLongImmOffsetReg,
	[43] = &XrLoadLongImmOffset,
	[44] = &XrSlti,
	[45] = &XrBlt,
	[46] = &XrIllegalInstruction,
	[47] = &XrIllegalInstruction,
	[48] = &XrIllegalInstruction,
	[49] = &XrLookup110001,
	[50] = &XrStoreIntImmOffsetReg,
	[51] = &XrLoadIntImmOffset,
	[52] = &XrSubi,
	[53] = &XrBne,
	[54] = &XrIllegalInstruction,
	[55] = &XrIllegalInstruction,
	[56] = &XrJalr,
	[57] = &XrLookup111001,
	[58] = &XrStoreByteImmOffsetReg,
	[59] = &XrLoadByteImmOffset,
	[60] = &XrAddi,
	[61] = &XrBeq,
	[62] = &XrIllegalInstruction,
	[63] = &XrIllegalInstruction,
};

static void XrJal(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	proc->Reg[LR] = proc->Pc;
	proc->Pc = (currentpc & 0x80000000) | ((ir >> 3) << 2);
}

static void XrJ(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	proc->Pc = (currentpc & 0x80000000) | ((ir >> 3) << 2);
}

static void XrLookupMajor(XrProcessor *proc, uint32_t currentpc, uint32_t ir) {
	XrLowSix[ir & 63](proc, currentpc, ir);
}

static XrInstructionF XrLowThree[8] = {
	[0] = &XrLookupMajor,
	[1] = &XrLookupMajor,
	[2] = &XrLookupMajor,
	[3] = &XrLookupMajor,
	[4] = &XrLookupMajor,
	[5] = &XrLookupMajor,
	[6] = &XrJ,
	[7] = &XrJal,
};

void XrExecute(XrProcessor *proc, uint32_t cycles, uint32_t dt) {
#ifdef PROFCPU
	if (XrPrintCache) {
		proc->TimeToNextPrint -= dt;

		if (proc->TimeToNextPrint <= 0) {
			// It's time to print some cache statistics.

			int itotal = proc->IcHitCount + proc->IcMissCount;
			int dtotal = proc->DcHitCount + proc->DcMissCount;

			fprintf(stderr, "%d: icache misses: %d (%.2f%% miss rate)\n", proc->Id, proc->IcMissCount, (double)proc->IcMissCount/(double)itotal*100.0);
			fprintf(stderr, "%d: dcache misses: %d (%.2f%% miss rate)\n", proc->Id, proc->DcMissCount, (double)proc->DcMissCount/(double)dtotal*100.0);

			proc->IcMissCount = 0;
			proc->IcHitCount = 0;

			proc->DcMissCount = 0;
			proc->DcHitCount = 0;

			proc->TimeToNextPrint = 2000;

			/*
			for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
				if (proc->DcFlags[i]) {
					printf("%d: %d = %x %x\n", proc->Id, i, proc->DcTags[i], proc->DcFlags[i]);
				}
			}
			*/
		}
	}
#endif

	Lsic *lsic = &LsicTable[proc->Id];

	if (proc->UserBreak && !proc->NmiMaskCounter) {
		// There's a pending user-initiated NMI, so do that.

		XrBasicInbetweenException(proc, XR_EXC_NMI);
		proc->UserBreak = 0;
		proc->Halted = 0;
	}

	if (proc->Halted) {
		// We're halted.

		proc->NmiMaskCounter = 0;

		if (!lsic->InterruptPending || (proc->Cr[RS] & RS_INT) == 0) {
			// Interrupts are disabled or there is no interrupt pending. Just
			// return.

			// N.B. There's an assumption here that the host platform will make
			// writes by other host cores to the interrupt pending flag visible
			// to us in a timely manner, without needing any locking.

			return;
		}

		// Interrupts are enabled and there is an interrupt pending.
		// Un-halt the processor.

		proc->Halted = 0;
	}

	if (proc->Progress <= 0) {
		// This processor did a poll-y looking thing too many times this
		// tick. Skip the rest of the tick so as not to eat up too much of
		// the host's CPU.

		return;
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

	for (; cyclesdone < cycles && !proc->Halted && proc->Running; cyclesdone++) {
		if (proc->NmiMaskCounter) {
			// Decrement the NMI mask cycle counter.

			proc->NmiMaskCounter -= 1;

			if (proc->UserBreak && !proc->NmiMaskCounter) {
				// There's a pending user-initiated NMI, so do that.

				XrBasicInbetweenException(proc, XR_EXC_NMI);
				proc->UserBreak = 0;
				proc->Halted = 0;
			}
		}

#ifdef PROFCPU
		proc->CycleCounter++;
#endif

		// Make sure the zero register is always zero, except during TLB misses,
		// where it may be used as a scratch register.

		if ((proc->Cr[RS] & RS_TBMISS) == 0) {
			proc->Reg[0] = 0;
		}

		if (XrSimulateCaches) {
			if (proc->StallCycles && XrSimulateCacheStalls) {
				// There's a simulated cache stall of some number of cycles, so
				// decrement the remaining stall and loop.

				proc->StallCycles--;

				continue;
			}

			if (proc->WbCycles) {
				proc->WbCycles -= 1;

				if (proc->WbCycles == 0) {
					// Time to write out a write buffer entry.

					XrWriteWbEntry(proc);
				}
			}
		}

		if (lsic->InterruptPending && (proc->Cr[RS] & RS_INT)) {
			// Interrupts are enabled and there's an interrupt pending, so cause
			// an interrupt exception.

			// N.B. There's an assumption here that the host platform will make
			// writes by other host cores to the interrupt pending flag visible
			// to us in a timely manner, without needing any locking.

			XrBasicInbetweenException(proc, XR_EXC_INT);
		}

		currentpc = proc->Pc;
		proc->Pc += 4;

		proc->IFetch = 1;
		status = XrReadLong(proc, currentpc, &ir);
		proc->IFetch = 0;

		if (!status) {
			// The read failed and an exception was caused, so loop and let the
			// exception handler execute.

			continue;
		}

		// Fetch was successful, decode the instruction word and execute the
		// instruction.

		XrLowThree[ir & 7](proc, currentpc, ir);
	}
}