//
// Cached interpreter core for the fictional XR/17032 microprocessor.
//
// Cool optimizations left to do:
//
// 1. The basic block cache is virtual and so needs to be purged when ITB
//    entries are explicitly flushed. I managed to tag them with ASIDs so they
//    dont need to be flushed upon address space switch, but it would be nice to
//    be able to only invalidate the basic blocks that resided within the
//    virtual page that was flushed. I have some ideas for doing this
//    efficiently, such as a secondary hash table keyed by bits of the VPN in
//    which basic blocks are all inserted.
//
// 2. Currently most of the branch instructions are capable of caching a pointer
//    to the next basic block for both the true and false paths, which avoids a
//    hash table lookup, but JALR (calling through a register) currently cannot.
//    I'd like to generalize the true/false paths into an array of cached paths
//    in the basic block header and allow JALR-terminated basic blocks to keep a
//    small history table of recent jump destinations.
//
//2a. As a sub-case of the above, JALR to R31 (the link register) should be able
//    to make use of a small stack cache that is "pushed" by JAL x and "popped"
//    by JALR ZERO, R31, 0.
//
// 3. Keep a small cache of 2-4 recent DTB lookups in the basic block header
//    which is consulted before the DTLB by instructions therein. There's an
//    issue with invalidating these when DTLB entries are flushed that im not
//    completely sure how to deal with. One way is by caching an index within
//    the DTB and having it be a validating look up (it makes sure the present
//    DTB entry matches the translation done earlier) but this eliminates the
//    potential benefit of increasing the functional size of the DTB.
//
// 4. Allow basic blocks to, most of the time, directly tail-call one another
//    rather than always returning to the outer dispatch loop on basic block
//    boundaries. I was thinking I could do this with a simple incrementing
//    counter where it'll do a direct call if (counter & 7) != 0 and return to
//    the dispatch loop otherwise. In fact, the dispatch loop could be
//    completely eliminated, with its functions replaced by another tail-called
//    routine (such as checking for interrupts on basic block boundaries).
//
// 5. I do an unnecessary copy from the Icache while doing instruction decoding
//    that could be replaced with directly examining the instruction data within
//    the Icache. It's also probably unnecessary to support noncached
//    instruction fetch and I can eliminate some branches if I just don't.
//
// 6. Various implementational improvements of older, overly-generalized cache
//    simulation and virtual memory translation machinery that can be more
//    specialized and optimized in the new cached interpreter world.
//

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#include "xr.h"
#include "lsic.h"
#include "ebus.h"

uint8_t XrPrintCache = 0;

uint32_t XrScacheTags[XR_SC_LINE_COUNT];
uint32_t XrScacheReplacementIndex;
uint8_t XrScacheFlags[XR_SC_LINE_COUNT];
uint8_t XrScacheExclusiveIds[XR_SC_LINE_COUNT];

#if DBG

#define DBGPRINT(...) printf(__VA_ARGS__)

#else

#define DBGPRINT(...)

#endif

static inline uint32_t RoR(uint32_t x, uint32_t n) {
	n &= 31;
	return (x >> n) | (x << (32-n));
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

#define XrReadByte(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 1, 0);
#define XrReadInt(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 2, 0);
#define XrReadLong(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 4, 0);

#define XrWriteByte(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 1, 0);
#define XrWriteInt(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 2, 0);
#define XrWriteLong(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 4, 0);

static inline void XrInvalidateIblockPointers(XrIblock *iblock) {
	for (int i = 0; i < XR_IBLOCK_CACHEDBY_MAX; i++) {
		if (iblock->CachedBy[i]) {
			*iblock->CachedBy[i] = 0;
		}
	}
}

static inline void XrCreateCachedPointerToBlock(XrIblock *iblock, XrIblock **ptr) {
	int index = (iblock->CachedByFifoIndex++) & (XR_IBLOCK_CACHEDBY_MAX - 1);

	if (iblock->CachedBy[index]) {
		*iblock->CachedBy[index] = 0;
	}

	*ptr = iblock;
	iblock->CachedBy[index] = ptr;
}

static inline void XrDeactivateIblock(XrIblock *iblock) {
	// Remove from the LRU list.

	RemoveEntryList(&iblock->LruEntry);

	// Remove from the hash table.

	RemoveEntryList(&iblock->HashEntry);
}

static inline void XrFreeIblock(XrProcessor *proc, XrIblock *iblock) {
	// Deactivate the Iblock.

	XrDeactivateIblock(iblock);

	// Insert in the free list.

	iblock->HashEntry.Next = (void *)proc->IblockFreeList;
	proc->IblockFreeList = (void *)iblock;
}

static void XrPopulateIblockList(XrProcessor *proc) {
	// The Iblock free list is empty. We need to repopulate by striking down
	// some active ones from the tail of the LRU list.
	//
	// We know that there are at least XR_IBLOCK_RECLAIM Iblocks on the LRU list
	// because all active Iblocks are on the LRU list, all Iblocks are currently
	// active, and there are more than XR_IBLOCK_RECLAIM total Iblocks.

	// Get a pointer to the Iblock on the tail of the LRU list.

	ListEntry *listentry = proc->IblockLruList.Prev;

	for (int i = 0; i < XR_IBLOCK_RECLAIM; i++) {
		XrIblock *iblock = ContainerOf(listentry, XrIblock, LruEntry);

		// Invalidate the pointers to this Iblock.

		XrInvalidateIblockPointers(iblock);

		// Free the Iblock. Note that this doesn't modify the LRU list links so
		// we don't need to stash them.

		XrFreeIblock(proc, iblock);

		// Advance to the previous Iblock.

		listentry = listentry->Prev;
	}
}

static void XrInvalidateIblockCache(XrProcessor *proc) {
	// Invaliate the entire Iblock cache for the processor.

	ListEntry *listentry = proc->IblockLruList.Next;

	while (listentry != &proc->IblockLruList) {
		XrIblock *iblock = ContainerOf(listentry, XrIblock, LruEntry);

		// No need to invalidate the Iblock's pointers. We're destroying all
		// active Iblocks.

		// Free the Iblock. Note that this doesn't modify the LRU list links so
		// we don't need to stash them.

		XrFreeIblock(proc, iblock);

		// Advance to the next Iblock.

		listentry = listentry->Next;
	}
}

static inline XrIblock *XrAllocateIblock(XrProcessor *proc) {
	XrIblock *iblock = proc->IblockFreeList;

	if (iblock == 0) {
		// Populate the Iblock free list.

		XrPopulateIblockList(proc);

		iblock = proc->IblockFreeList;
	}

	// Pop the head Iblock from the free list and return it.

	proc->IblockFreeList = (void *)iblock->HashEntry.Next;

	return iblock;
}

static inline XrIblock *XrLookupIblock(XrProcessor *proc, uint32_t pc, uint32_t asid) {
	// Look up a cached Iblock starting at the given program counter.

	uint32_t hash = XR_IBLOCK_HASH(pc);

	ListEntry *listentry = proc->IblockHashBuckets[hash].Next;

	while (listentry != &proc->IblockHashBuckets[hash]) {
		XrIblock *iblock = ContainerOf(listentry, XrIblock, HashEntry);

		if (iblock->Pc == pc && iblock->Asid == asid) {
			// Found it.

			// Move the Iblock to the front of the hash bucket for faster
			// lookup later.

			// Note the if (move) conditional should get optimized out
			// when this routine is inlined.

			RemoveEntryList(listentry);

			InsertAtHeadList(&proc->IblockHashBuckets[hash], listentry);

			return iblock;
		}

		listentry = listentry->Next;
	}

	return 0;
}

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
	proc->PauseCalls = 0;

	proc->NmiMaskCounter = NMI_MASK_CYCLES;
	proc->LastTbMissWasWrite = 0;
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

static inline void XrBasicException(XrProcessor *proc, uint32_t exc, uint32_t pc) {
	// "Basic" exceptions that behave the same way every time.

	proc->Cr[EPC] = pc;

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
		proc->Cr[TBPC] = proc->Pc;
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
		proc->Cr[TBPC] = proc->Pc;
		proc->Cr[RS] |= RS_TBMISS;
	}

	XrVectorException(proc, XR_EXC_DTB);

	return 0;
}

static inline int XrTranslate(XrProcessor *proc, uint32_t virtual, uint32_t *phys, int *flags, bool writing, bool ifetch) {
	uint64_t tbe;
	uint32_t vpn = virtual >> 12;

	if (ifetch) {
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
			proc->Cr[EPC] = proc->Pc;
			XrPushMode(proc);
		}

		XrSetEcause(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);
		XrVectorException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);

		return 0;
	}

	if ((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER)) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (writing && !(tbe & PTE_WRITABLE)) {
		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (ifetch) {
		proc->ItbLastVpn = vpn;
		proc->ItbLastResult = tbe;
	} else {
		proc->DtbLastVpn = vpn;
		proc->DtbLastResult = tbe;
	}

	*flags = tbe & 31;
	*phys = ((tbe & 0x1FFFFE0) << 7) + (virtual & 0xFFF);

	//DBGPRINT("virt=%x phys=%x\n", virt, *phys);

	return 1;
}

static uint32_t XrAccessMasks[5] = {
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static inline int XrNoncachedAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length) {
	proc->StallCycles += XR_UNCACHED_STALL;

	int result;

	if (dest) {
		result = EBusRead(address, dest, length, proc);
	} else {
		result = EBusWrite(address, &srcvalue, length, proc);
	}

	if (result == EBUSERROR) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

		return 0;
	}

	return 1;
}

static inline int XrIcacheAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t length) {
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

		CopyWithLength(dest, &proc->Ic[proc->IcLastOffset + lineoffset], length);

		return 1;
	}

	uint32_t setnumber = XR_IC_SET_NUMBER(address);
	uint32_t cacheindex = setnumber << XR_IC_WAY_LOG;

	for (int i = 0; i < XR_IC_WAYS; i++) {
		if (proc->IcFlags[cacheindex + i] && proc->IcTags[cacheindex + i] == tag) {
			// Found it!

#ifdef PROFCPU
			proc->IcHitCount++;
#endif

			uint32_t cacheoff = (cacheindex + i) << XR_IC_LINE_SIZE_LOG;

			CopyWithLength(dest, &proc->Ic[cacheoff + lineoffset], length);

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

	int result = EBusRead(tag, &proc->Ic[cacheoff], XR_IC_LINE_SIZE, proc);

	if (result == EBUSERROR) {
		proc->IcFlags[newindex] = XR_LINE_INVALID;

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

		return 0;
	}

	proc->IcFlags[newindex] = XR_LINE_SHARED;
	proc->IcTags[newindex] = tag;

	proc->IcLastTag = tag;
	proc->IcLastOffset = cacheoff;

	CopyWithLength(dest, &proc->Ic[cacheoff + lineoffset], length);

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

			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

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

	uint32_t setnumber = XR_SC_SET_NUMBER(tag);
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

	uint32_t setnumber = XR_DC_SET_NUMBER(tag);
	uint32_t cacheindex = setnumber << XR_DC_WAY_LOG;

	XrLockCache(proc, tag);

	// Find it in the Dcache.

	for (int i = 0; i < XR_DC_WAYS; i++) {
		DBGPRINT("search %d %x %d\n", cacheindex + i, proc->DcTags[cacheindex + i], proc->DcFlags[cacheindex + i]);

		uint32_t index = cacheindex + i;

		if (proc->DcFlags[index] > newstate && proc->DcTags[index] == tag) {
			// Found it! Kill it.

			DBGPRINT("found to inval %x %d %d\n", tag, proc->DcFlags[cacheindex + i], cacheindex + i);

			// Find it in the writebuffer.

			uint32_t wbindex = proc->DcIndexToWbIndex[index];

			if (wbindex != XR_WB_INDEX_INVALID) {
				// Force a write out.

				DBGPRINT("forced wb write %x from cacheindex %d\n", tag, index);

				EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

				// Invalidate.

				proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
				proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
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

	uint32_t setnumber = XR_SC_SET_NUMBER(tag);
	uint32_t cacheindex = setnumber << XR_SC_WAY_LOG;

	for (int i = 0; i < XR_SC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (XrScacheFlags[index] && XrScacheTags[index] == tag) {
			// Found it!

			DBGPRINT("scache hit %x\n", tag);

			return index;
		}
	}

	// Didn't find it so select one to replace.

	cacheindex += XrScacheReplacementIndex & (XR_SC_WAYS - 1);
	XrScacheReplacementIndex += 1;

	if (XrScacheFlags[cacheindex] == XR_LINE_INVALID) {
		// It's invalid, we can just take it.

	} else {
		// Lock the tag in this selected entry.

		uint32_t oldtag = XrScacheTags[cacheindex];

		DBGPRINT("scache take %x\n", oldtag);

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
	}

	XrScacheFlags[cacheindex] = newstate;
	XrScacheTags[cacheindex] = tag;
	XrScacheExclusiveIds[cacheindex] = thisproc->Id;

	DBGPRINT("scache fill %x\n", tag);

	*created = 1;

	return cacheindex;
}

static int XrDcacheAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, int forceexclusive) {
	// Access Dcache. Scary! We have to worry about coherency with the other
	// Dcaches in the system, and this can be a 1, 2, or 4 byte access.
	// If dest == 0, this is a write. Otherwise it's a read.

	uint32_t tag = address & ~(XR_DC_LINE_SIZE - 1);
	uint32_t lineoffset = address & (XR_DC_LINE_SIZE - 1);
	uint32_t setnumber = XR_DC_SET_NUMBER(address);
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
				fprintf(stderr, "Not found in scache %08x %d %d\n", address, index, proc->DcFlags[index]);
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

	XrLockCache(proc, tag);

	// We have to select a Dcache line to replace now.

	uint32_t index = cacheindex + (proc->DcReplacementIndex & (XR_DC_WAYS - 1));
	proc->DcReplacementIndex += 1;

	if (proc->DcFlags[index] != XR_LINE_INVALID) {
		// Lock the tag in this selected entry.

		uint32_t oldtag = proc->DcTags[index];

		DBGPRINT("reclaim %x after miss\n", oldtag);

		// See if it's in the writebuffer. If so we'll have to force a
		// write.

		uint32_t wbindex = proc->DcIndexToWbIndex[index];

		if (wbindex != XR_WB_INDEX_INVALID) {
			// Force a write out.

			DBGPRINT("forced self wb write %x from cacheindex %d\n", tag, index);

			EBusWrite(oldtag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

			// Invalidate.

			proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
		}

		// Mark invalid for the time where we have no tags locked.

		proc->DcFlags[index] = XR_LINE_INVALID;
	}

	// Initialize the cache line.

	DBGPRINT("new cacheline setnumber=%x cacheindex=%x index=%x out of %x\n", setnumber, cacheindex, index, XR_DC_LINE_COUNT);

	proc->DcFlags[index] = newstate;
	proc->DcTags[index] = tag;

	// Incur a stall.

	proc->StallCycles += XR_MISS_STALL;

	// Read in the cache line contents.

	uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

	int result = EBusRead(tag, &proc->Dc[cacheoff], XR_DC_LINE_SIZE, proc);

	if (result == EBUSERROR) {
		// We failed. Back out.

		XrScacheFlags[scacheindex] = XR_LINE_INVALID;
		proc->DcFlags[index] = XR_LINE_INVALID;

		XrUnlockCache(proc, tag);
		XrUnlockScache(tag);

		DBGPRINT("bus error on %x\n", tag);

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

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

static inline int XrAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, int forceexclusive) {
	if (address & (length - 1)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	int flags = 0;

	if (proc->Cr[RS] & RS_MMU) {
		if (!XrTranslate(proc, address, &address, &flags, dest ? 0 : 1, 0)) {
			return 0;
		}
	} else if (address >= XR_NONCACHED_PHYS_BASE) {
		flags |= PTE_NONCACHED;
	}

	if (!XR_SIMULATE_CACHES) {
		flags |= PTE_NONCACHED;
	}

	if (flags & PTE_NONCACHED) {
		if (forceexclusive && XR_SIMULATE_CACHES) {
			// Attempt to use LL/SC on noncached memory. Nonsense! Just kill the
			// evildoer with a bus error.

			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS, proc->Pc);

			return 0;
		}

		int status = XrNoncachedAccess(proc, address, dest, srcvalue, length);

		if (!status) {
			return 0;
		}

		if (dest) {
			*dest &= XrAccessMasks[length];
		}

		return 1;
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

			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

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

static XrIblock *XrDecodeInstructions(XrProcessor *proc, uint32_t pc);

#define XR_NEXT_NO_PC() inst++; XR_TAIL return inst->Func(proc, block, inst);
#define XR_NEXT() proc->Pc += 4; XR_NEXT_NO_PC();

#define XR_REG_RD() proc->Reg[inst->Imm8_1]
#define XR_REG_RA() proc->Reg[inst->Imm8_2]
#define XR_REG_RB() proc->Reg[inst->Imm32_1]
#define XR_INST_SHAMT() inst->Imm8_3
#define XR_SHIFTED_VAL() inst->ShiftFunc(XR_REG_RB(), XR_INST_SHAMT())
#define XR_MAINTAIN_ZERO() if ((proc->Cr[RS] & RS_TBMISS) == 0) proc->Reg[0] = 0;

XR_PRESERVE_NONE
static XrIblock *XrExecuteIllegalInstruction(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 1\n");

	XrBasicException(proc, XR_EXC_INV, proc->Pc);

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteNor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 2\n");

	XR_REG_RD() = ~(XR_REG_RA() | XR_SHIFTED_VAL());

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteOr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 3\n");

	XR_REG_RD() = XR_REG_RA() | XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteXor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 4\n");

	XR_REG_RD() = XR_REG_RA() ^ XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteAnd(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 5\n");

	XR_REG_RD() = XR_REG_RA() & XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSltSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 6\n");

	if ((int32_t) XR_REG_RA() < (int32_t) XR_SHIFTED_VAL()) {
		XR_REG_RD() = 1;
	} else {
		XR_REG_RD() = 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 7\n");

	if (XR_REG_RA() < XR_SHIFTED_VAL()) {
		XR_REG_RD() = 1;
	} else {
		XR_REG_RD() = 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSub(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 8\n");

	XR_REG_RD() = XR_REG_RA() - XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteAdd(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {

	DBGPRINT("exec 9\n");

	XR_REG_RD() = XR_REG_RA() + XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 10\n");

	XR_REG_RD() = XR_REG_RB() << XR_REG_RA();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteRsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 11\n");

	XR_REG_RD() = XR_REG_RB() >> XR_REG_RA();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteAsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 12\n");

	XR_REG_RD() = (int32_t) XR_REG_RB() >> XR_REG_RA();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteRor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 13\n");

	XR_REG_RD() = RoR(XR_REG_RB(), XR_REG_RA());

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreLongRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 14\n");

	int status = XrWriteLong(proc, XR_REG_RA() + XR_SHIFTED_VAL(), XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreIntRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 15\n");

	int status = XrWriteInt(proc, XR_REG_RA() + XR_SHIFTED_VAL(), XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreByteRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 16\n");

	int status = XrWriteByte(proc, XR_REG_RA() + XR_SHIFTED_VAL(), XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadLongRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 17\n");

	int status = XrReadLong(proc, XR_REG_RA() + XR_SHIFTED_VAL(), &XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadIntRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 18\n");

	int status = XrReadInt(proc, XR_REG_RA() + XR_SHIFTED_VAL(), &XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadByteRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 19\n");

	int status = XrReadByte(proc, XR_REG_RA() + XR_SHIFTED_VAL(), &XR_REG_RD());

	if (!status) {
		// An exception occurred, so perform an early exit from the basic block.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSys(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 20\n");

	XrBasicException(proc, XR_EXC_SYS, proc->Pc + 4);

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBrk(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 21\n");

	XrBasicException(proc, XR_EXC_BRK, proc->Pc + 4);

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteWmb(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 22\n");

	if (XR_SIMULATE_CACHES) {
		// Flush the write buffer.

		XrFlushWriteBuffer(proc);
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecutePause(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 23\n");

	proc->PauseCalls++;

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSC(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 24\n");

	if (!proc->Locked) {
		// Something happened on this processor that caused the
		// lock to go away.

		XR_REG_RD() = 0;
	} else {
		// Store the word in a way that will atomically fail if we
		// no longer have the cache line from LL's load.

		//DBGPRINT("%d: SC %d\n", proc->Id, proc->Reg[rb]);

		uint8_t status = XrAccess(proc, XR_REG_RA(), 0, XR_REG_RB(), 4, 1);

		if (!status) {
			return 0;
		}

		XR_REG_RD() = (status == 2) ? 0 : 1;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLL(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 25\n");

	int status = XrReadLong(proc, XR_REG_RA(), &XR_REG_RD());

	if (!status) {
		return 0;
	}

	proc->Locked = 1;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteMod(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 26\n");

	uint32_t val = XR_SHIFTED_VAL();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = XR_REG_RA() % val;

		XR_MAINTAIN_ZERO();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteDivSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 27\n");

	uint32_t val = XR_SHIFTED_VAL();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = (int32_t) XR_REG_RA() / (int32_t) val;

		XR_MAINTAIN_ZERO();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteDiv(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 28\n");

	uint32_t val = XR_SHIFTED_VAL();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = XR_REG_RA() / val;

		XR_MAINTAIN_ZERO();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteMul(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 29\n");

	XR_REG_RD() = XR_REG_RA() * XR_SHIFTED_VAL();

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteRfe(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 30\n");

	proc->Locked = 0;

	uint32_t oldrs = proc->Cr[RS];

	if (oldrs & RS_USER) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		return 0;
	}

	if (proc->Cr[RS] & RS_TBMISS) {
		proc->Pc = proc->Cr[TBPC];
	} else {
		proc->Pc = proc->Cr[EPC];
	}

	proc->Cr[RS] = (proc->Cr[RS] & 0xF0000000) | ((proc->Cr[RS] >> 8) & 0xFFFF);

	if ((proc->Cr[RS] & RS_TBMISS) == 0) {
		// Make sure the zero register is still zero after the TB miss handler.
		proc->Reg[0] = 0;
	}

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteHlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 31\n");

	if (proc->Cr[RS] & RS_USER) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		return 0;
	}

	proc->Halted = true;

	proc->Pc += 4;

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteMtcr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 32\n");

	if (proc->Cr[RS] & RS_USER) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		return 0;
	}

	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	uint32_t ra = inst->Imm8_1;
	uint32_t rb = inst->Imm8_2;

	switch(rb) {
		case ICACHECTRL:
			if (!XR_SIMULATE_CACHES)
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

			// Dump the whole Iblock cache.

			XrInvalidateIblockCache(proc);

			proc->Pc += 4;

			return 0;

		case DCACHECTRL:
			if (!XR_SIMULATE_CACHES)
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

				//DBGPRINT("invl %x\n", Reg[ra] >> 12);

				//Running = false;
			}

			// Reset the lookup hint.

			proc->ItbLastVpn = -1;

			// Dump the whole Iblock cache.

			XrInvalidateIblockCache(proc);

			proc->Pc += 4;

			return 0;

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

				//DBGPRINT("invl %x\n", Reg[ra] >> 12);

				//Running = false;
			}

			// Reset the lookup hint.

			proc->DtbLastVpn = -1;

			break;

		case ITBPTE:
			// Write an entry to the ITB at ITBINDEX, and
			// increment it.

			proc->Itb[proc->Cr[ITBINDEX]] = ((uint64_t)(proc->Cr[ITBTAG]) << 32) | proc->Reg[ra];

			//DBGPRINT("ITB[%d] = %llx\n", ControlReg[ITBINDEX], ITlb[ControlReg[ITBINDEX]]);

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

			//DBGPRINT("DTB[%d] = %llx\n", ControlReg[DTBINDEX], DTlb[ControlReg[DTBINDEX]]);

			proc->Cr[DTBINDEX] += 1;

			if (proc->Cr[DTBINDEX] == XR_DTB_SIZE) {
				// Roll over to index four.

				proc->Cr[DTBINDEX] = 4;
			}

			break;

		case ITBINDEX:
			proc->Cr[ITBINDEX] = proc->Reg[ra] & (XR_ITB_SIZE - 1);
			break;

		case DTBINDEX:
			proc->Cr[DTBINDEX] = proc->Reg[ra] & (XR_DTB_SIZE - 1);
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

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteMfcr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 33\n");

	if (proc->Cr[RS] & RS_USER) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		return 0;
	}

	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	proc->Reg[inst->Imm8_1] = proc->Cr[inst->Imm8_2];

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBpo(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 34\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] & 1) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBpe(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 35\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((proc->Reg[rd] & 1) == 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBge(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 36\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] >= 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBle(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 37\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] <= 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBgt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 38\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] > 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 39\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] < 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBne(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 40\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] != 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteBeq(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 41\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] == 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->TruePath;
	} else {
		proc->Pc += 4;
		referrent = &block->FalsePath;
	}

	iblock = *referrent;

	if (iblock) {
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, referrent);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteB(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 42\n");

	proc->Pc += inst->Imm32_1;

	XrIblock *iblock = block->TruePath;

	if (iblock) {
		DBGPRINT("true path b %p\n", iblock);
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, &block->TruePath);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteOri(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 43\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] | imm;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteXori(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 44\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] ^ imm;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteAndi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 45\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] & imm;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSltiSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 46\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	if ((int32_t) proc->Reg[ra] < (int32_t) imm) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSlti(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 47\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	if (proc->Reg[ra] < imm) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteSubi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 48\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] - imm;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteAddi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 49\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] + imm;

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreLongImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 50\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteLong(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreIntImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 51\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteInt(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreByteImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 52\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteByte(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreLongImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 53\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteLong(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreIntImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 54\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteInt(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteStoreByteImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 55\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteByte(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (!status) {
		return 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadLongImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 56\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadLong(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (!status) {
		// An exception occurred.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadIntImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 57\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadInt(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (!status) {
		// An exception occurred.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteLoadByteImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 58\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadByte(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (!status) {
		// An exception occurred.

		return 0;
	}

	XR_MAINTAIN_ZERO();

	XR_NEXT();
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteJalr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 59 @ %x\n", proc->Pc);

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;

	proc->Reg[rd] = proc->Pc + 4;
	proc->Pc = proc->Reg[ra] + inst->Imm32_1;

	DBGPRINT("jalr destination rd=%x reg[%d]=%x + %x = %x\n", rd, ra, proc->Reg[ra], inst->Imm32_1, proc->Pc);

	XR_MAINTAIN_ZERO();

	return 0;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteJal(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 60\n");

	proc->Reg[LR] = proc->Pc + 4;
	proc->Pc = inst->Imm32_1;

	XrIblock *iblock = block->TruePath;

	if (iblock) {
		DBGPRINT("true path jal %p\n", iblock);
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, &block->TruePath);

	return iblock;
}

XR_PRESERVE_NONE
static XrIblock *XrExecuteJ(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 61\n");

	proc->Pc = inst->Imm32_1;

	XrIblock *iblock = block->TruePath;

	if (iblock) {
		DBGPRINT("true path j %p\n", iblock);
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, &block->TruePath);

	return iblock;
}

XR_PRESERVE_NONE
static uint32_t XrShiftLsh(uint32_t a, uint32_t b) {
	return a << b;
}

XR_PRESERVE_NONE
static uint32_t XrShiftRsh(uint32_t a, uint32_t b) {
	return a >> b;
}

XR_PRESERVE_NONE
static uint32_t XrShiftAsh(uint32_t a, uint32_t b) {
	return (int32_t) a >> b;
}

XR_PRESERVE_NONE
static uint32_t XrShiftRor(uint32_t a, uint32_t b) {
	return RoR(a, b);
}

static XrInstImplF XrRegShiftFunctionTable[4] = {
	[0] = &XrExecuteLsh,
	[1] = &XrExecuteRsh,
	[2] = &XrExecuteAsh,
	[3] = &XrExecuteRor
};

static XrInstShiftF XrShiftFunctionTable[4] = {
	[0] = &XrShiftLsh,
	[1] = &XrShiftRsh,
	[2] = &XrShiftAsh,
	[3] = &XrShiftRor
};

typedef int (*XrDecodeInstructionF)(XrCachedInst *inst, uint32_t ir, uint32_t pc);

static int XrDecodeIllegalInstruction(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteIllegalInstruction;

	return 1;
}

static int XrDecodeRfe(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteRfe;

	return 1;
}

static int XrDecodeHlt(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteHlt;

	return 1;
}

static int XrDecodeMtcr(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMtcr;
	inst->Imm8_1 = (ir >> 11) & 31;
	inst->Imm8_2 = (ir >> 16) & 31;

	return 0;
}

static int XrDecodeMfcr(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMfcr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 16) & 31;

	return 0;
}

static XrDecodeInstructionF XrDecodeFunctions101001[16] = {
	[0] = &XrDecodeIllegalInstruction,
	[1] = &XrDecodeIllegalInstruction,
	[2] = &XrDecodeIllegalInstruction,
	[3] = &XrDecodeIllegalInstruction,
	[4] = &XrDecodeIllegalInstruction,
	[5] = &XrDecodeIllegalInstruction,
	[6] = &XrDecodeIllegalInstruction,
	[7] = &XrDecodeIllegalInstruction,
	[8] = &XrDecodeIllegalInstruction,
	[9] = &XrDecodeIllegalInstruction,
	[10] = &XrDecodeIllegalInstruction,
	[11] = &XrDecodeRfe,
	[12] = &XrDecodeHlt,
	[13] = &XrDecodeIllegalInstruction,
	[14] = &XrDecodeMtcr,
	[15] = &XrDecodeMfcr,
};

static int XrDecodeSys(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSys;

	return 1;
}

static int XrDecodeBrk(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBrk;

	return 1;
}

static int XrDecodeWmb(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteWmb;

	return 0;
}

static int XrDecodeMb(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteWmb;

	return 0;
}

static int XrDecodePause(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecutePause;

	return 0;
}

static int XrDecodeSC(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSC;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	return 0;
}

static int XrDecodeLL(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLL;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;

	return 0;
}

static int XrDecodeMod(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMod;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeDivSigned(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteDivSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeDiv(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteDiv;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeMul(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMul;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static XrDecodeInstructionF XrDecodeFunctions110001[16] = {
	[0] = &XrDecodeSys,
	[1] = &XrDecodeBrk,
	[2] = &XrDecodeWmb,
	[3] = &XrDecodeMb,
	[4] = &XrDecodeIllegalInstruction,
	[5] = &XrDecodeIllegalInstruction,
	[6] = &XrDecodePause,
	[7] = &XrDecodeIllegalInstruction,
	[8] = &XrDecodeSC,
	[9] = &XrDecodeLL,
	[10] = &XrDecodeIllegalInstruction,
	[11] = &XrDecodeMod,
	[12] = &XrDecodeDivSigned,
	[13] = &XrDecodeDiv,
	[14] = &XrDecodeIllegalInstruction,
	[15] = &XrDecodeMul,
};

static int XrDecodeNor(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteNor;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeOr(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteOr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeXor(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteXor;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeAnd(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAnd;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeSltSigned(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSltSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeSlt(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSlt;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeSub(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSub;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeAdd(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAdd;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeRegShifts(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = XrRegShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	return 0;
}

static int XrDecodeStoreLongRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreLongRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeStoreIntRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreIntRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeStoreByteRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreByteRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeLoadLongRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadLongRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeLoadIntRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadIntRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static int XrDecodeLoadByteRegOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadByteRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) & 31;

	inst->ShiftFunc = XrShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_3 = (ir >> 21) & 31;

	return 0;
}

static XrDecodeInstructionF XrDecodeFunctions111001[16] = {
	[0] = &XrDecodeNor,
	[1] = &XrDecodeOr,
	[2] = &XrDecodeXor,
	[3] = &XrDecodeAnd,
	[4] = &XrDecodeSltSigned,
	[5] = &XrDecodeSlt,
	[6] = &XrDecodeSub,
	[7] = &XrDecodeAdd,
	[8] = &XrDecodeRegShifts,
	[9] = &XrDecodeStoreLongRegOffset,
	[10] = &XrDecodeStoreIntRegOffset,
	[11] = &XrDecodeStoreByteRegOffset,
	[12] = &XrDecodeIllegalInstruction,
	[13] = &XrDecodeLoadLongRegOffset,
	[14] = &XrDecodeLoadIntRegOffset,
	[15] = &XrDecodeLoadByteRegOffset,
};

static int XrDecodeLui(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteOri;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 16;

	return 0;
}

static int XrDecodeBpo(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBpo;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecodeStoreLongImmOffsetImm(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreLongImmOffsetImm;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 2;

	return 0;
}

static int XrDecodeOri(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteOri;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBpe(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBpe;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecodeStoreIntImmOffsetImm(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreIntImmOffsetImm;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 1;

	return 0;
}

static int XrDecodeXori(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteXori;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBge(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBge;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecodeStoreByteImmOffsetImm(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreByteImmOffsetImm;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeAndi(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAndi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBle(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBle;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecodeSltiSigned(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSltiSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = SignExt16(ir >> 16);

	return 0;
}

static int XrDecodeBgt(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBgt;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecode101001(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions101001[ir >> 28](inst, ir, pc);
}

static int XrDecodeStoreLongImmOffsetReg(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreLongImmOffsetReg;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 2;

	return 0;
}

static int XrDecodeLoadLongImmOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadLongImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 2;

	return 0;
}

static int XrDecodeSlti(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSlti;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBlt(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBlt;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecode110001(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions110001[ir >> 28](inst, ir, pc);
}

static int XrDecodeStoreIntImmOffsetReg(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreIntImmOffsetReg;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 1;

	return 0;
}

static int XrDecodeLoadIntImmOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadIntImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 1;

	return 0;
}

static int XrDecodeSubi(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSubi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBne(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBne;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 1;
}

static int XrDecodeJalr(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJalr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = SignExt18((ir >> 16) << 2);

	return 1;
}

static int XrDecode111001(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions111001[ir >> 28](inst, ir, pc);
}

static int XrDecodeStoreByteImmOffsetReg(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreByteImmOffsetReg;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeLoadByteImmOffset(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadByteImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeAddi(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAddi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return 0;
}

static int XrDecodeBeq(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	if (((ir >> 6) & 31) == 0) {
		// This is a BEQ, ZERO, XXX instruction.
		// This is the canonical unconditional branch, generated by the B
		// pseudo-instruction. We can optimize this a bit.

		inst->Func = &XrExecuteB;

		return 1;
	}

	inst->Func = &XrExecuteBeq;
	inst->Imm8_1 = (ir >> 6) & 31;

	return 1;
}

static XrDecodeInstructionF XrDecodeLowSix[64] = {
	[0] = &XrDecodeIllegalInstruction,
	[1] = &XrDecodeIllegalInstruction,
	[2] = &XrDecodeIllegalInstruction,
	[3] = &XrDecodeIllegalInstruction,
	[4] = &XrDecodeLui,
	[5] = &XrDecodeBpo,
	[6] = &XrDecodeIllegalInstruction,
	[7] = &XrDecodeIllegalInstruction,
	[8] = &XrDecodeIllegalInstruction,
	[9] = &XrDecodeIllegalInstruction,
	[10] = &XrDecodeStoreLongImmOffsetImm,
	[11] = &XrDecodeIllegalInstruction,
	[12] = &XrDecodeOri,
	[13] = &XrDecodeBpe,
	[14] = &XrDecodeIllegalInstruction,
	[15] = &XrDecodeIllegalInstruction,
	[16] = &XrDecodeIllegalInstruction,
	[17] = &XrDecodeIllegalInstruction,
	[18] = &XrDecodeStoreIntImmOffsetImm,
	[19] = &XrDecodeIllegalInstruction,
	[20] = &XrDecodeXori,
	[21] = &XrDecodeBge,
	[22] = &XrDecodeIllegalInstruction,
	[23] = &XrDecodeIllegalInstruction,
	[24] = &XrDecodeIllegalInstruction,
	[25] = &XrDecodeIllegalInstruction,
	[26] = &XrDecodeStoreByteImmOffsetImm,
	[27] = &XrDecodeIllegalInstruction,
	[28] = &XrDecodeAndi,
	[29] = &XrDecodeBle,
	[30] = &XrDecodeIllegalInstruction,
	[31] = &XrDecodeIllegalInstruction,
	[32] = &XrDecodeIllegalInstruction,
	[33] = &XrDecodeIllegalInstruction,
	[34] = &XrDecodeIllegalInstruction,
	[35] = &XrDecodeIllegalInstruction,
	[36] = &XrDecodeSltiSigned,
	[37] = &XrDecodeBgt,
	[38] = &XrDecodeIllegalInstruction,
	[39] = &XrDecodeIllegalInstruction,
	[40] = &XrDecodeIllegalInstruction,
	[41] = &XrDecode101001,
	[42] = &XrDecodeStoreLongImmOffsetReg,
	[43] = &XrDecodeLoadLongImmOffset,
	[44] = &XrDecodeSlti,
	[45] = &XrDecodeBlt,
	[46] = &XrDecodeIllegalInstruction,
	[47] = &XrDecodeIllegalInstruction,
	[48] = &XrDecodeIllegalInstruction,
	[49] = &XrDecode110001,
	[50] = &XrDecodeStoreIntImmOffsetReg,
	[51] = &XrDecodeLoadIntImmOffset,
	[52] = &XrDecodeSubi,
	[53] = &XrDecodeBne,
	[54] = &XrDecodeIllegalInstruction,
	[55] = &XrDecodeIllegalInstruction,
	[56] = &XrDecodeJalr,
	[57] = &XrDecode111001,
	[58] = &XrDecodeStoreByteImmOffsetReg,
	[59] = &XrDecodeLoadByteImmOffset,
	[60] = &XrDecodeAddi,
	[61] = &XrDecodeBeq,
	[62] = &XrDecodeIllegalInstruction,
	[63] = &XrDecodeIllegalInstruction,
};

static int XrDecodeJal(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJal;
	inst->Imm32_1 = (pc & 0x80000000) | ((ir >> 3) << 2);

	return 1;
}

static int XrDecodeJ(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJ;
	inst->Imm32_1 = (pc & 0x80000000) | ((ir >> 3) << 2);

	return 1;
}

static int XrDecodeMajor(XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeLowSix[ir & 63](inst, ir, pc);
}

static XrDecodeInstructionF XrDecodeLowThree[8] = {
	[0] = &XrDecodeMajor,
	[1] = &XrDecodeMajor,
	[2] = &XrDecodeMajor,
	[3] = &XrDecodeMajor,
	[4] = &XrDecodeMajor,
	[5] = &XrDecodeMajor,
	[6] = &XrDecodeJ,
	[7] = &XrDecodeJal,
};

XR_PRESERVE_NONE
XrIblock *XrSpecialLinkageInstruction(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 0\n");

	// This instruction is placed at the end of a basic block that didn't
	// terminate in a natural way (with a branch or illegal instruction).
	// It just directs the outer loop to look up the next block.

	XrIblock *iblock = block->TruePath;

	if (XrLikely(iblock != 0)) {
		// We already cached the pointer to the next Iblock, so return it
		// directly.

		DBGPRINT("true path linkage %p\n", iblock);
		return iblock;
	}

	iblock = XrDecodeInstructions(proc, proc->Pc);

	if (!iblock) {
		// Ifetch mishap.

		return 0;
	}

	XrCreateCachedPointerToBlock(iblock, &block->TruePath);

	return iblock;
}

static XrIblock *XrDecodeInstructions(XrProcessor *proc, uint32_t pc) {
	// Decode some instructions starting at the given virtual PC.
	// Return NULLPTR if we fail to fetch the first instruction. This implies
	// that an exception occurred, such as an ITB miss, page fault, or bus
	// error.

	uint32_t asid;

	int mmuon = proc->Cr[RS] & RS_MMU;

	if (mmuon) {
		asid = proc->Cr[ITBTAG] & 0xFFF00000;
	} else {
		asid = 0xFFFFFFFF;
	}

	XrIblock *iblock = XrLookupIblock(proc, pc, asid);

	if (XrLikely(iblock != 0)) {
		// Already cached.

		if (XrUnlikely((iblock->PteFlags & PTE_KERNEL) && (proc->Cr[RS] & RS_USER))) {
			proc->Cr[EBADADDR] = pc;
			XrBasicException(proc, XR_EXC_PGF, proc->Pc);

			return 0;
		}

		return iblock;
	}

	if (XrUnlikely((pc & 3) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = pc;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	uint32_t ir[XR_IBLOCK_INSTS];

	// Round down to the last Icache line boundary so that we can fetch one line
	// at a time.

	uint32_t fetchpc = pc & ~(XR_IC_LINE_SIZE - 1);

	int instindex = (pc - fetchpc) >> 2;

	int instcount = XR_IBLOCK_INSTS;

	// Don't allow fetches to cross page boundaries.

	if (((fetchpc + XR_IBLOCK_INSTS_BYTES) & 0xFFFFF000) != (pc & 0xFFFFF000)) {
		instcount = (((pc + 0xFFF) & 0xFFFFF000) - fetchpc) >> 2;
	}

	// Translate the program counter.

	int flags = 0;

	if (XrLikely(proc->Cr[RS] & RS_MMU)) {
		if (!XrTranslate(proc, fetchpc, &fetchpc, &flags, 0, 1)) {
			return 0;
		}
	} else if (XrUnlikely(fetchpc >= XR_NONCACHED_PHYS_BASE)) {
		flags |= PTE_NONCACHED;
	}

	if (!XR_SIMULATE_CACHES) {
		flags |= PTE_NONCACHED;
	}

	// Fetch instructions one line at a time.

	if (XrUnlikely(flags & PTE_NONCACHED)) {
		for (int offset = 0;
			offset < instcount;
			offset += XR_IC_INST_PER_LINE, fetchpc += XR_IC_LINE_SIZE) {

			int status = XrNoncachedAccess(proc, fetchpc, &ir[offset], 0, XR_IC_LINE_SIZE);

			if (XrUnlikely(!status)) {
				return 0;
			}
		}
	} else {
		for (int offset = 0;
			offset < instcount;
			offset += XR_IC_INST_PER_LINE, fetchpc += XR_IC_LINE_SIZE) {

			int status = XrIcacheAccess(proc, fetchpc, &ir[offset], XR_IC_LINE_SIZE);

			if (XrUnlikely(!status)) {
				return 0;
			}
		}
	}

	// Allocate an Iblock.

	iblock = XrAllocateIblock(proc);

	iblock->Pc = pc;
	iblock->Asid = asid;
	iblock->Cycles = 0;
	iblock->TruePath = 0;
	iblock->FalsePath = 0;
	iblock->CachedByFifoIndex = 0;
	iblock->PteFlags = flags;

	for (int i = 0; i < XR_IBLOCK_CACHEDBY_MAX; i++) {
		iblock->CachedBy[i] = 0;
	}

	InsertAtHeadList(&proc->IblockHashBuckets[XR_IBLOCK_HASH(pc)], &iblock->HashEntry);
	InsertAtHeadList(&proc->IblockLruList, &iblock->LruEntry);

	// Decode instructions starting at the offset of the program counter within
	// the fetched chunk, until we reach either a branch, an illegal
	// instruction, or the end of the chunk.

	DBGPRINT("decode %x %x %x %x\n", instindex, pc, fetchpc, ir[instindex]);

	XrCachedInst *inst = &iblock->Insts[0];

	for (;
		instindex < instcount;
		instindex++, inst++, pc += 4) {

		iblock->Cycles++;

		// The decode routine returns 1 if the instruction terminates the basic
		// block.

		if (XrDecodeLowThree[ir[instindex] & 7](inst, ir[instindex], pc)) {
			inst++;
			break;
		}
	}

	// In case we went right up to the end of the basic block's maximum extent
	// without running into a control flow instruction, we want this special
	// linkage instruction at the end which just directs the interpreter to the
	// next basic block. There's an extra instruction slot in the basic block to
	// make sure there's room for this.

	inst->Func = &XrSpecialLinkageInstruction;

	return iblock;
}

int XrExecuteFast(XrProcessor *proc, uint32_t cycles, uint32_t dt) {
#ifdef PROFCPU
	if (XrPrintCache) {
		proc->TimeToNextPrint -= dt;

		if (proc->TimeToNextPrint <= 0) {
			// It's time to print some cache statistics.

			int itotal = proc->IcHitCount + proc->IcMissCount;
			int dtotal = proc->DcHitCount + proc->DcMissCount;

			fDBGPRINT(stderr, "%d: icache misses: %d (%.2f%% miss rate)\n", proc->Id, proc->IcMissCount, (double)proc->IcMissCount/(double)itotal*100.0);
			fDBGPRINT(stderr, "%d: dcache misses: %d (%.2f%% miss rate)\n", proc->Id, proc->DcMissCount, (double)proc->DcMissCount/(double)dtotal*100.0);

			proc->IcMissCount = 0;
			proc->IcHitCount = 0;

			proc->DcMissCount = 0;
			proc->DcHitCount = 0;

			proc->TimeToNextPrint = 2000;

			/*
			for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
				if (proc->DcFlags[i]) {
					DBGPRINT("%d: %d = %x %x\n", proc->Id, i, proc->DcTags[i], proc->DcFlags[i]);
				}
			}
			*/
		}
	}
#endif

	Lsic *lsic = &LsicTable[proc->Id];

	if (proc->UserBreak && !proc->NmiMaskCounter) {
		// There's a pending user-initiated NMI, so do that.

		XrBasicException(proc, XR_EXC_NMI, proc->Pc);
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

			return 0;
		}

		// Interrupts are enabled and there is an interrupt pending.
		// Un-halt the processor.

		proc->Halted = 0;
	}

	if (proc->Progress <= 0) {
		// This processor did a poll-y looking thing too many times this
		// tick. Skip the rest of the tick so as not to eat up too much of
		// the host's CPU.

		return 0;
	}

	proc->PauseCalls = 0;

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

	XrIblock *iblock = 0;

	for (; cyclesdone < cycles && !proc->Halted && proc->Running && proc->PauseCalls < XR_PAUSE_MAX;) {
		if (proc->NmiMaskCounter) {
			// Decrement the NMI mask cycle counter.

			proc->NmiMaskCounter -= 1;

			if (proc->UserBreak && !proc->NmiMaskCounter) {
				// There's a pending user-initiated NMI, so do that.

				XrBasicException(proc, XR_EXC_NMI, proc->Pc);
				proc->UserBreak = 0;
				proc->Halted = 0;

				// Make sure we re-lookup the Iblock.

				iblock = 0;
			}
		}

		if (XR_SIMULATE_CACHE_STALLS && proc->StallCycles) {
			// There's a simulated cache stall of some number of cycles, so
			// decrement the remaining stall and loop.

			proc->StallCycles--;

			continue;
		}

		if (lsic->InterruptPending && (proc->Cr[RS] & RS_INT)) {
			// Interrupts are enabled and there's an interrupt pending, so cause
			// an interrupt exception.

			// N.B. There's an assumption here that the host platform will make
			// writes by other host cores to the interrupt pending flag visible
			// to us in a timely manner, without needing any barriers.

			XrBasicException(proc, XR_EXC_INT, proc->Pc);

			// Reset the poll counter so as not to penalize the processor for
			// handling the interrupt.

			proc->Progress = XR_POLL_MAX;

			// Make sure we re-lookup the Iblock.

			iblock = 0;
		}

		if (!iblock) {
			iblock = XrDecodeInstructions(proc, proc->Pc);

			if (!iblock) {
				// An exception occurred while performing instruction fetch.
				// Loop and let the exception handler execute.

				cyclesdone += XR_IBLOCK_INSTS;

				continue;
			}

			DBGPRINT("acquired %p\n", iblock);
		}

		DBGPRINT("running %x %p\n", proc->Pc, iblock);

		uint32_t blockcycles = iblock->Cycles;

		cyclesdone += blockcycles;

#ifdef PROFCPU
		proc->CycleCounter += blockcycles;
#endif

		// Move the basic block to the front of the LRU list.

		RemoveEntryList(&iblock->LruEntry);

		InsertAtHeadList(&proc->IblockLruList, &iblock->LruEntry);

		// Execute the basic block.
		// Each instruction execute function directly tail-calls the next until
		// the final one returns the next Iblock to us.

		iblock = iblock->Insts[0].Func(proc, iblock, &iblock->Insts[0]);

		DBGPRINT("next %p\n", iblock);

		if (XR_SIMULATE_CACHES && proc->WbCycles) {
			if (proc->WbCycles <= blockcycles) {
				// Time to write out a write buffer entry.
				proc->WbCycles = 0;
				XrWriteWbEntry(proc);
			} else {
				proc->WbCycles -= blockcycles;
			}
		}
	}

	return cyclesdone;
}