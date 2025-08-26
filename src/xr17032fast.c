//
// Cached interpreter core for the fictional XR/17032 microprocessor.
//
// Cool optimizations left to do:
//
//    DONE
// 1. The basic block cache is virtual and so needs to be purged when ITB
//    entries are explicitly flushed. I managed to tag them with ASIDs so they
//    dont need to be flushed upon address space switch, but it would be nice to
//    be able to only invalidate the basic blocks that resided within the
//    virtual page that was flushed. I have some ideas for doing this
//    efficiently, such as a secondary hash table keyed by bits of the VPN in
//    which basic blocks are all inserted.
//
//
//    DONE
// 2. Currently most of the branch instructions are capable of caching a pointer
//    to the next basic block for both the true and false paths, which avoids a
//    hash table lookup, but JALR (calling through a register) currently cannot.
//    I'd like to generalize the true/false paths into an array of cached paths
//    in the basic block header and allow JALR-terminated basic blocks to keep a
//    small history table of recent jump destinations.
//
//
//    DONE, THEN REVERTED: Caused a performance regression.
//2a. As a sub-case of the above, JALR to R31 (the link register) should be able
//    to make use of a small stack cache that is "pushed" by JAL x and "popped"
//    by JALR ZERO, R31, 0.
//
//
// 3. Keep a small cache of 2-4 recent DTB lookups in the basic block header
//    which is consulted before the DTLB by instructions therein. There's an
//    issue with invalidating these when DTLB entries are flushed that im not
//    completely sure how to deal with. One way is by caching an index within
//    the DTB and having it be a validating look up (it makes sure the present
//    DTB entry matches the translation done earlier) but this eliminates the
//    potential benefit of increasing the functional size of the DTB.
//
//
//    DONE
// 4. Allow basic blocks to, most of the time, directly tail-call one another
//    rather than always returning to the outer dispatch loop on basic block
//    boundaries. I was thinking I could do this with a simple incrementing
//    counter where it'll do a direct call if (counter & 7) != 0 and return to
//    the dispatch loop otherwise. In fact, the dispatch loop could be
//    completely eliminated, with its functions replaced by another tail-called
//    routine (such as checking for interrupts on basic block boundaries).
//
//
//    DONE, THEN PARTLY REVERTED: Putting the decode loop inside the Ifetch loop
//                                caused some inscrutable performance regression
// 5. I do an unnecessary copy from the Icache while doing instruction decoding
//    that could be replaced with directly examining the instruction data within
//    the Icache. It's also probably unnecessary to support noncached
//    instruction fetch and I can eliminate some branches if I just don't.
//
//    ????
//    This raises a more interesting notion where the entire memory access model
//    of the emulator should maybe be replaced with a phys addr -> host addr
//    translation scheme, potentially even with its own cache, so I can easily
//    get direct pointers to host memory to do things like instruction decode
//    rapidly. This can always be done for cacheable memory but there still
//    needs to be an active and action-oriented rather than passive and
//    translation-oriented interface for noncached mappings of things like
//    device registers.
//
//
// 6. Various implementational improvements of older, overly-generalized cache
//    simulation and virtual memory translation machinery that can be more
//    specialized and optimized in the new cached interpreter world.
//
//    DONE
//    The cache mutexes currently take up an undue amount of time even when
//    uncontended because of dynamic calls from xremu -> SDL -> pthreads.
//    It is likely worth it to replace this with our own attempt at an inline
//    lock acquisition with a single atomic operation, and then call SDL
//    directly only if it fails.
//
//
//    DONE
// 7. Optimize the zero register. Maybe keep destination registers the same
//    during decode, but replace any source register specified as zero, to be a
//    virtual 33rd register with index 32 that always contains zero. Except for
//    blocks that are decoded with the RS_TBMISS bit set, which must directly
//    use the zero register (it is banked and usable as scratch during TLB miss
//    handling).
//
//
// 8. Decode with a small peephole window rather than a single instruction at a
//    time, and collapse common idioms such as
//
//        SUB RD, RA, RB
//        BEQ RD, OFFSET
//
//    which compares two registers RA and RB and branches if they're equal, to
//
//        BEQ.sub RD, RA, RB, OFFSET
//
//    a direct comparison and jump in a single virtual instruction. Note that it
//    still performs the subtraction into RD because that is a visible side
//    effect of the original sequence and we aren't fancy enough to tell ahead
//    of time whether the result of the subtraction will actually be needed.
//
//
//    DONE
// 9. The inline shifts for register instructions should not be a call through a
//    function pointer as they are currently, but should instead be either a
//    chain of if statements (perhaps doing a binary search) or a virtual
//    instruction inserted into the basic block, which performs the shift into a
//    fake register which is then used as the source by the register
//    instruction. Both methods should be tried and benchmarked. The main
//    benefit of the virtual instruction is that in the common case of a shift
//    amount of 0, we can completely elide the shift. The if statements on the
//    other hand have a constant overhead even when shamt == 0.
//
//
//10. The proc->Pc += 4 that regularly appears can be replaced by an on-demand
//    calculation of the current program counter as 
//
//        block->Pc + (inst - &block->Insts[0]) * 4
//
//    and other appropriate calculations.

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
#include "rtc.h"

int XrProcessorCount = 1;

long XrProcessorFrequency = 20000000;

XrProcessor *XrProcessorTable[XR_PROC_MAX];

#if XR_SIMULATE_CACHES && !SINGLE_THREAD_MP

XrMutex XrScacheMutexes[XR_CACHE_MUTEXES];

#endif

#define XR_STEP_MS 17 // 60Hz rounded up

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
// This means ASID 4095 is unusable.

#define TB_INVALID_ENTRY 0xFFF0000000000000
#define TB_INVALID_MATCHING 0xFFFFFFFF00000000

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

static inline void XrInvalidateIblockPointers(XrIblock *iblock) {
	for (int i = 0; i < XR_IBLOCK_CACHEDBY_MAX; i++) {
		if (iblock->CachedBy[i] && *iblock->CachedBy[i] == iblock) {
			*iblock->CachedBy[i] = 0;
		}
	}
}

static inline void XrCreateCachedPointerToBlock(XrIblock *iblock, XrIblock **ptr) {
	int index = (iblock->CachedByFifoIndex++) & (XR_IBLOCK_CACHEDBY_MAX - 1);

	if (iblock->CachedBy[index] && *iblock->CachedBy[index] == iblock) {
		*iblock->CachedBy[index] = 0;
	}

	*ptr = iblock;
	iblock->CachedBy[index] = ptr;
}

static inline XrJalrPredictionTable *XrAllocatePtable(XrProcessor *proc) {
	// There are as many Ptables as Iblocks, so since the caller got an Iblock,
	// we don't need to check if there are free Ptables.

	XrJalrPredictionTable *ptable = proc->PtableFreeList;
	proc->PtableFreeList = (void *)ptable->Iblocks[0];

	for (int i = 0; i < XR_JALR_PREDICTION_TABLE_ENTRIES; i++) {
		ptable->Iblocks[i] = 0;
	}

	return ptable;
}

static inline void XrFreePtable(XrProcessor *proc, XrJalrPredictionTable *ptable) {
	// Insert in the free list.

	ptable->Iblocks[0] = (void *)proc->PtableFreeList;
	proc->PtableFreeList = ptable;
}

static inline XrVirtualPage *XrAllocateVpage(XrProcessor *proc) {
	// There are as many Vpages as Iblocks, so since the caller got an Iblock,
	// we don't need to check if there are free Vpages.

	XrVirtualPage *vpage = proc->VpageFreeList;
	proc->VpageFreeList = (void *)vpage->VpnHashEntry.Next;

	return vpage;
}

static inline void XrFreeVpage(XrProcessor *proc, XrVirtualPage *vpage) {
	// Insert in the free list.

	vpage->VpnHashEntry.Next = (void *)proc->VpageFreeList;
	proc->VpageFreeList = vpage;
}

static inline void XrFreeIblock(XrProcessor *proc, XrIblock *iblock) {
	// Remove from the LRU list.

	RemoveEntryList(&iblock->LruEntry);

	// Remove from the hash table.

	RemoveEntryList(&iblock->HashEntry);

	// Remove from the Vpage list.

	RemoveEntryList(&iblock->VpageEntry);

	// Decrement the Vpage's reference count and delete it if this was the final
	// Iblock within the virtual page.

	if (--iblock->Vpage->References == 0) {
		RemoveEntryList(&iblock->Vpage->VpnHashEntry);

		XrFreeVpage(proc, iblock->Vpage);
	}

	// Free Ptable.

	if (iblock->HasPtable) {
		XrFreePtable(proc, (void *)iblock->CachedPaths[0]);
	}

	// Insert in the free list.

	iblock->HashEntry.Next = (void *)proc->IblockFreeList;
	proc->IblockFreeList = (void *)iblock;
}

static void XrPopulateIblockList(XrProcessor *proc, XrIblock *hazard) {
	// The Iblock free list is empty. We need to repopulate by striking down
	// some active ones from the tail of the LRU list. The hazard pointer
	// parameter is provided to specify an Iblock which may not be reclaimed.
	// This is usually an Iblock currently in use by a caller whose invalidation
	// at this time would cause problems.
	//
	// We know that there are at least XR_IBLOCK_RECLAIM Iblocks on the LRU list
	// because all active Iblocks are on the LRU list, all Iblocks are currently
	// active, and there are more than XR_IBLOCK_RECLAIM total Iblocks.

	// Get a pointer to the Iblock on the tail of the LRU list.

	ListEntry *listentry = proc->IblockLruList.Prev;

	for (int i = 0; i < XR_IBLOCK_RECLAIM; i++) {
		XrIblock *iblock = ContainerOf(listentry, XrIblock, LruEntry);

		if (hazard != iblock) {
			// Invalidate the pointers to this Iblock.

			XrInvalidateIblockPointers(iblock);

			// Free the Iblock. Note that this doesn't modify the LRU list links
			// so we don't need to stash them.

			XrFreeIblock(proc, iblock);
		}

		// Advance to the previous Iblock.

		listentry = listentry->Prev;
	}
}

static void XrInvalidateIblockCache(XrProcessor *proc) {
	// Invalidate the entire Iblock cache for the processor.

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

static inline void XrInvalidateVpage(XrProcessor *proc, XrVirtualPage *vpage) {
	// Invalidate all of the Iblocks that reside within the VPN represented by
	// the given Vpage.

	ListEntry *listentry = vpage->IblockVpnList.Next;

	while (listentry != &vpage->IblockVpnList) {
		XrIblock *iblock = ContainerOf(listentry, XrIblock, VpageEntry);

		// Invalidate the pointers to this Iblock.

		XrInvalidateIblockPointers(iblock);

		// Free the Iblock. Note that this doesn't modify the VPN list links so
		// we don't need to stash them.

		XrFreeIblock(proc, iblock);

		listentry = listentry->Next;
	}
}

static void XrInvalidateIblockCacheByVpn(XrProcessor *proc, uint32_t vpn) {
	// Invalidate the Iblocks that match the given VPN.

	ListEntry *listentry = proc->VpageHashBuckets[XR_VPN_BUCKET_INDEX(vpn)].Next;

	while (listentry != &proc->VpageHashBuckets[XR_VPN_BUCKET_INDEX(vpn)]) {
		XrVirtualPage *vpage = ContainerOf(listentry, XrVirtualPage, VpnHashEntry);

		if (vpage->Vpn == vpn) {
			// Invalidate the Iblocks within this virtual page.

			XrInvalidateVpage(proc, vpage);

			return;
		}

		// Advance to the next virtual page.

		listentry = listentry->Next;
	}
}

static inline void XrInsertIblockInVpage(XrProcessor* proc, XrIblock *iblock, uint32_t pc) {
	// Insert the Iblock in a Vpage or create a new one if this is the first
	// one in that virtual page.

	XrVirtualPage *vpage;

	int searches = 0;

	uint32_t vpn = pc & ~0xFFF;
	uint32_t hash = XR_VPN_BUCKET_INDEX(vpn);

	ListEntry *listentry = proc->VpageHashBuckets[hash].Next;

	while (listentry != &proc->VpageHashBuckets[hash]) {
		vpage = ContainerOf(listentry, XrVirtualPage, VpnHashEntry);

		if (vpage->Vpn == vpn) {
			// Found it.

			vpage->References++;

			// Insert the Iblock in the Vpage's list.

			iblock->Vpage = vpage;
			InsertAtHeadList(&vpage->IblockVpnList, &iblock->VpageEntry);

			return;
		}

		searches++;
		listentry = listentry->Next;
	}

	// Failed to find a Vpage, so allocate a new one.

	vpage = XrAllocateVpage(proc);

	vpage->Vpn = vpn;
	vpage->References = 1;

	// No need to initialize the Iblock list head, it was done at init time.
	// InitializeList(&vpage->IblockVpnList);

	InsertAtHeadList(&proc->VpageHashBuckets[hash], &vpage->VpnHashEntry);

	// Insert the Iblock in the Vpage's list.

	iblock->Vpage = vpage;
	InsertAtHeadList(&vpage->IblockVpnList, &iblock->VpageEntry);
}

static inline XrIblock *XrAllocateIblock(XrProcessor *proc, XrIblock *hazard) {
	XrIblock *iblock = proc->IblockFreeList;

	if (XrUnlikely(iblock == 0)) {
		// Populate the Iblock free list.

		XrPopulateIblockList(proc, hazard);

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

	proc->Reg[XR_FAKE_ZERO_REGISTER] = 0;

	// Initialize emulator support stuff.

	proc->ItbLastVpn = -1;
	proc->DtbLastVpn = -1;

#if XR_SIMULATE_CACHES
	proc->IcReplacementIndex = 0;
	proc->DcReplacementIndex = 0;

	proc->WbWriteIndex = 0;
	proc->WbCycles = 0;
	proc->WbFillIndex = 0;

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		proc->WbIndices[i] = XR_CACHE_INDEX_INVALID;
	}

	for (int i = 0; i < XR_DC_LINE_COUNT; i++) {
		proc->DcIndexToWbIndex[i] = XR_WB_INDEX_INVALID;
	}
#endif

#ifdef PROFCPU
	proc->IcMissCount = 0;
	proc->IcHitCount = 0;

	proc->DcMissCount = 0;
	proc->DcHitCount = 0;

	proc->TimeToNextPrint = 0;
#endif

#if XR_SIMULATE_CACHE_STALLS
	proc->StallCycles = 0;
#endif
	proc->PauseCalls = 0;

	proc->NmiMaskCounter = NMI_MASK_CYCLES;
	proc->LastTbMissWasWrite = 0;
	proc->UserBreak = 0;
	proc->Halted = 0;
	proc->Running = 1;
	proc->Dispatches = 0;
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

	if (XrUnlikely(proc->Cr[EB] == 0)) {
		// Reset the processor.

		XrReset(proc);

		return;
	}

	DBGPRINT("exc %d\n", exc);
	// proc->Running = false;

	// Build new mode bits.
	// Enter kernel mode and disable interrupts.

	uint32_t newmode = proc->Cr[RS] & 0xFC;

	if (XrUnlikely((proc->Cr[RS] & RS_LEGACY) != 0)) {
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

#ifdef FASTMEMORY

#include "xrfastaccess.inc.c"

#else

#include "xraccess.inc.c"

#endif

static XrIblock *XrDecodeInstructions(XrProcessor *proc, XrIblock *hazard);

XR_PRESERVE_NONE
static void XrCheckConditions(XrProcessor *proc, XrIblock *nextblock, XrCachedInst *inst) {
	// Check if any conditions are true that indicate we should terminate
	// the execution chain.

#if XR_SIMULATE_CACHES
	if (proc->WbCycles) {
		// Assume enough cycles have passed to empty the writebuffer at no
		// charge.

		proc->WbCycles = 0;
		XrFlushWriteBuffer(proc);
	}
#endif

retry:

	if (XrUnlikely(proc->CyclesDone >= proc->CyclesGoal)) {
		proc->NoMore = 1;
		return;
	}

	if (XrUnlikely(proc->Halted)) {
		proc->NoMore = 1;
		return;
	}

	if (XrUnlikely(proc->PauseCalls >= XR_PAUSE_MAX)) {
		proc->NoMore = 1;
		return;
	}

	if (XrUnlikely(proc->NmiMaskCounter != 0)) {
		proc->NmiMaskCounter--;
	}

	Lsic *lsic = &LsicTable[proc->Id];

	if (XrUnlikely(lsic->InterruptPending && (proc->Cr[RS] & RS_INT))) {
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

		nextblock = 0;
	}

	if (XrUnlikely(!nextblock)) {
		nextblock = XrDecodeInstructions(proc, 0);

		if (XrUnlikely(!nextblock)) {
			// An exception occurred while performing instruction fetch.
			// Loop and let the exception handler execute.

			proc->CyclesDone += XR_IBLOCK_INSTS;

			goto retry;
		}
	}

	// Call it directly.

	XR_TAIL return nextblock->Insts[0].Func(proc, nextblock, &nextblock->Insts[0]);
}

#define XR_NEXT_NO_PC() inst++; XR_TAIL return inst->Func(proc, block, inst);
#define XR_NEXT() proc->Pc += 4; XR_NEXT_NO_PC();

#define XR_DISPATCH(nextblock) \
	proc->CyclesDone += block->Cycles; \
	if (XrUnlikely((proc->Dispatches++ & 31) == 0)) { \
		return; \
	} \
	XR_TAIL return nextblock->Insts[0].Func(proc, nextblock, &nextblock->Insts[0]);

#define XR_TRIVIAL_EXIT() \
	proc->CyclesDone += block->Cycles; \
	XR_TAIL return XrCheckConditions(proc, 0, 0);

#define XR_EARLY_EXIT() \
	proc->CyclesDone += inst - &block->Insts[0]; \
	return;

#define XR_REG_RD() proc->Reg[inst->Imm8_1]
#define XR_REG_RA() proc->Reg[inst->Imm8_2]
#define XR_REG_RB() proc->Reg[inst->Imm32_1]

#define XR_CURRENT_ASID() ((XrLikely(proc->Cr[RS] & RS_MMU) != 0) ? (proc->Cr[ITBTAG] & 0xFFF00000) : 0xFFFFFFFF)

XR_PRESERVE_NONE
static void XrExecuteIllegalInstruction(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 1\n");

	XrBasicException(proc, XR_EXC_INV, proc->Pc);

	XR_TRIVIAL_EXIT();
}

XR_PRESERVE_NONE
static void XrExecuteNor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 2\n");

	XR_REG_RD() = ~(XR_REG_RA() | XR_REG_RB());

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteOr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 3\n");

	XR_REG_RD() = XR_REG_RA() | XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteXor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 4\n");

	XR_REG_RD() = XR_REG_RA() ^ XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteAnd(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 5\n");

	XR_REG_RD() = XR_REG_RA() & XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSltSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 6\n");

	if ((int32_t) XR_REG_RA() < (int32_t) XR_REG_RB()) {
		XR_REG_RD() = 1;
	} else {
		XR_REG_RD() = 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 7\n");

	if (XR_REG_RA() < XR_REG_RB()) {
		XR_REG_RD() = 1;
	} else {
		XR_REG_RD() = 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSub(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 8\n");

	XR_REG_RD() = XR_REG_RA() - XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteAdd(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {

	DBGPRINT("exec 9\n");

	XR_REG_RD() = XR_REG_RA() + XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 10\n");

	XR_REG_RD() = XR_REG_RB() << XR_REG_RA();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteRsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 11\n");

	XR_REG_RD() = XR_REG_RB() >> XR_REG_RA();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteAsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 12\n");

	XR_REG_RD() = (int32_t) XR_REG_RB() >> XR_REG_RA();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteRor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 13\n");

	XR_REG_RD() = RoR(XR_REG_RB(), XR_REG_RA());

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreLongRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 14\n");

	int status = XrWriteLong(proc, XR_REG_RA() + XR_REG_RB(), XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreIntRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 15\n");

	int status = XrWriteInt(proc, XR_REG_RA() + XR_REG_RB(), XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreByteRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 16\n");

	int status = XrWriteByte(proc, XR_REG_RA() + XR_REG_RB(), XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadLongRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 17\n");

	int status = XrReadLong(proc, XR_REG_RA() + XR_REG_RB(), &XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadIntRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 18\n");

	int status = XrReadInt(proc, XR_REG_RA() + XR_REG_RB(), &XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadByteRegOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 19\n");

	int status = XrReadByte(proc, XR_REG_RA() + XR_REG_RB(), &XR_REG_RD());

	if (XrUnlikely(!status)) {
		// An exception occurred, so perform an early exit from the basic block.

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSys(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 20\n");

	XrBasicException(proc, XR_EXC_SYS, proc->Pc + 4);

	XR_TRIVIAL_EXIT();
}

XR_PRESERVE_NONE
static void XrExecuteBrk(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 21\n");

	XrBasicException(proc, XR_EXC_BRK, proc->Pc + 4);

	XR_TRIVIAL_EXIT();
}

XR_PRESERVE_NONE
static void XrExecuteWmb(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 22\n");

#if XR_SIMULATE_CACHES
	XrFlushWriteBuffer(proc);
#endif

#if FASTMEMORY
	atomic_thread_fence(memory_order_release);
#endif

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteMb(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 22\n");

#if XR_SIMULATE_CACHES
	XrFlushWriteBuffer(proc);
#endif

#if FASTMEMORY
	atomic_thread_fence(memory_order_acq_rel);
#endif

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecutePause(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 23\n");

	if (proc->PauseCalls++ >= XR_PAUSE_MAX) {
		// Terminate execution.

		proc->Pc += 4;

		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSC(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 24\n");

	if (XrUnlikely(!proc->Locked)) {
		// Something happened on this processor that caused the
		// lock to go away.

		XR_REG_RD() = 0;
	} else {
		// Store the word in a way that will atomically fail if we no longer
		// have the cache line from LL's load. This is accomplished by passing
		// sc=1 to XrAccess.

		//DBGPRINT("%d: SC %d\n", proc->Id, proc->Reg[rb]);

		int status = XrWriteLongSc(proc, XR_REG_RA(), XR_REG_RB());

		if (XrUnlikely(!status)) {
			XR_EARLY_EXIT();
		}
		
		XR_REG_RD() = (status == 2) ? 0 : 1;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLL(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 25\n");

	int status = XrReadLongLl(proc, XR_REG_RA(), &XR_REG_RD());

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	proc->Locked = 1;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteMod(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 26\n");

	uint32_t val = XR_REG_RB();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = XR_REG_RA() % val;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteDivSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 27\n");

	uint32_t val = XR_REG_RB();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = (int32_t) XR_REG_RA() / (int32_t) val;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteDiv(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 28\n");

	uint32_t val = XR_REG_RB();

	if (val == 0) {
		XR_REG_RD() = 0;
	} else {
		XR_REG_RD() = XR_REG_RA() / val;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteMul(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 29\n");

	XR_REG_RD() = XR_REG_RA() * XR_REG_RB();

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteRfe(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 30\n");

	proc->Locked = 0;

	uint32_t oldrs = proc->Cr[RS];

	if (XrUnlikely((oldrs & RS_USER) != 0)) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		XR_EARLY_EXIT();
	}

	if (XrUnlikely((proc->Cr[RS] & RS_TBMISS) != 0)) {
		proc->Pc = proc->Cr[TBPC];
	} else {
		proc->Pc = proc->Cr[EPC];
	}

	proc->Cr[RS] = (proc->Cr[RS] & 0xF0000000) | ((proc->Cr[RS] >> 8) & 0xFFFF);

	XR_TRIVIAL_EXIT();
}

XR_PRESERVE_NONE
static void XrExecuteHlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 31\n");

	if (XrUnlikely((proc->Cr[RS] & RS_USER) != 0)) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		XR_EARLY_EXIT();
	}

	proc->Halted = true;

	proc->Pc += 4;

	XR_TRIVIAL_EXIT();
}

XR_PRESERVE_NONE
static void XrExecuteMtcr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 32\n");

	if (XrUnlikely((proc->Cr[RS] & RS_USER) != 0)) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		XR_EARLY_EXIT();
	}

	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	uint32_t ra = inst->Imm8_1;
	uint32_t rb = inst->Imm8_2;

	switch(rb) {
		case ICACHECTRL:
#if XR_SIMULATE_CACHES
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
#endif

			// Dump the whole Iblock cache.

			XrInvalidateIblockCache(proc);

			proc->Pc += 4;

			XR_EARLY_EXIT();

		case DCACHECTRL:
#if XR_SIMULATE_CACHES
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
#endif

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

				// We can be a little more optimized when livalidating a single
				// page.

				// Only invalidate the lookup hint if it matches this VPN.

				if (proc->ItbLastVpn == proc->Reg[ra] >> 12) {
					proc->ItbLastVpn = -1;
				}

				// Only invalidate the Iblocks that reside in this VPN.

				proc->Pc += 4;

				XrInvalidateIblockCacheByVpn(proc, proc->Reg[ra] & ~0xFFF);

				XR_EARLY_EXIT();
			}

			// Reset the lookup hint.

			proc->ItbLastVpn = -1;

			// Dump the whole Iblock cache.

			XrInvalidateIblockCache(proc);

			proc->Pc += 4;

			XR_EARLY_EXIT();

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

				// Only invalidate the lookup hint if it matches this VPN.

				if (proc->DtbLastVpn == proc->Reg[ra] >> 12) {
					proc->DtbLastVpn = -1;
				}

				break;
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
static void XrExecuteMfcr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 33\n");

	if (proc->Cr[RS] & RS_USER) {
		// Ope, privilege violation.

		XrBasicException(proc, XR_EXC_PRV, proc->Pc);

		XR_EARLY_EXIT();
	}

	// Reset the NMI mask counter.

	proc->NmiMaskCounter = NMI_MASK_CYCLES;

	proc->Reg[inst->Imm8_1] = proc->Cr[inst->Imm8_2];

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteBpo(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 34\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] & 1) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBpe(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 35\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((proc->Reg[rd] & 1) == 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBge(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 36\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] >= 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBle(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 37\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] <= 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBgt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 38\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] > 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBlt(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 39\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if ((int32_t)proc->Reg[rd] < 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBne(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 40\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] != 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteBeq(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 41\n");

	uint32_t rd = inst->Imm8_1;

	XrIblock *iblock;
	XrIblock **referrent;

	if (proc->Reg[rd] == 0) {
		proc->Pc += inst->Imm32_1;
		referrent = &block->CachedPaths[XR_TRUE_PATH];
	} else {
		proc->Pc += 4;
		referrent = &block->CachedPaths[XR_FALSE_PATH];
	}

	iblock = *referrent;

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, referrent);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteB(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 42\n");

	proc->Pc += inst->Imm32_1;

	XrIblock *iblock = block->CachedPaths[XR_TRUE_PATH];

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, &block->CachedPaths[XR_TRUE_PATH]);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteOri(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 43\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] | imm;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteXori(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 44\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] ^ imm;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteAndi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 45\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] & imm;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSltiSigned(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 46\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	if ((int32_t) proc->Reg[ra] < (int32_t) imm) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSlti(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 47\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	if (proc->Reg[ra] < imm) {
		proc->Reg[rd] = 1;
	} else {
		proc->Reg[rd] = 0;
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteSubi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 48\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] - imm;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteAddi(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 49\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	proc->Reg[rd] = proc->Reg[ra] + imm;

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreLongImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 50\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteLong(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreIntImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 51\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteInt(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreByteImmOffsetImm(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 52\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteByte(proc, proc->Reg[rd] + imm, SignExt5(ra));

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreLongImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 53\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteLong(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreIntImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 54\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteInt(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteStoreByteImmOffsetReg(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 55\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;

	int status = XrWriteByte(proc, proc->Reg[rd] + imm, proc->Reg[ra]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadLongImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 56\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadLong(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadIntImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 57\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadInt(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteLoadByteImmOffset(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 58\n");

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;
	uint32_t imm = inst->Imm32_1;
	
	int status = XrReadByte(proc, proc->Reg[ra] + imm, &proc->Reg[rd]);

	if (XrUnlikely(!status)) {
		XR_EARLY_EXIT();
	}

	XR_NEXT();
}

XR_PRESERVE_NONE
static void XrExecuteJalr(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 59 @ %x\n", proc->Pc);

	uint32_t rd = inst->Imm8_1;
	uint32_t ra = inst->Imm8_2;

	proc->Reg[rd] = proc->Pc + 4;

	uint32_t pc = proc->Reg[ra] + inst->Imm32_1;

	proc->Pc = pc;

	// This is the only time where the program counter can become unaligned, so
	// check for that condition here.

	if (XrUnlikely((pc & 3) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = pc;
		XrBasicException(proc, XR_EXC_UNA, pc);

		XR_EARLY_EXIT();
	}

	XrJalrPredictionTable *ptable = (void*)block->CachedPaths[0];

	if (XrUnlikely(!ptable)) {
		ptable = XrAllocatePtable(proc);
		block->CachedPaths[0] = (void*)ptable;
		block->HasPtable = 1;
	}

	XrIblock *iblock;

	int index = (pc >> 2) & (XR_JALR_PREDICTION_TABLE_ENTRIES - 1);

	if (XrLikely(ptable->Pcs[index] == pc)) {
		iblock = ptable->Iblocks[index];
		if (XrLikely(iblock != 0)) {
			goto dispatch;
		}
	}

	iblock = XrDecodeInstructions(proc, block);

	if (XrUnlikely(!iblock)) {
		XR_EARLY_EXIT();
	}

	ptable->Pcs[index] = pc;

	XrCreateCachedPointerToBlock(iblock, &ptable->Iblocks[index]);

dispatch:

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteJal(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 60\n");

	proc->Reg[LR] = proc->Pc + 4;
	proc->Pc = inst->Imm32_1;

	XrIblock *iblock = block->CachedPaths[XR_TRUE_PATH];

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, &block->CachedPaths[XR_TRUE_PATH]);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteJ(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 61\n");

	proc->Pc = inst->Imm32_1;

	XrIblock *iblock = block->CachedPaths[XR_TRUE_PATH];

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, &block->CachedPaths[XR_TRUE_PATH]);
	}

	XR_DISPATCH(iblock);
}

XR_PRESERVE_NONE
static void XrExecuteVirtualLsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 101\n");

	proc->Reg[XR_FAKE_SHIFT_SINK] = proc->Reg[inst->Imm8_1] << inst->Imm8_2;

	XR_NEXT_NO_PC();
}

XR_PRESERVE_NONE
static void XrExecuteVirtualRsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 102\n");

	proc->Reg[XR_FAKE_SHIFT_SINK] = proc->Reg[inst->Imm8_1] >> inst->Imm8_2;

	XR_NEXT_NO_PC();
}

XR_PRESERVE_NONE
static void XrExecuteVirtualAsh(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 103\n");

	proc->Reg[XR_FAKE_SHIFT_SINK] = (int32_t) proc->Reg[inst->Imm8_1] >> inst->Imm8_2;

	XR_NEXT_NO_PC();
}

XR_PRESERVE_NONE
static void XrExecuteVirtualRor(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 104\n");

	proc->Reg[XR_FAKE_SHIFT_SINK] = RoR(proc->Reg[inst->Imm8_1], inst->Imm8_2);

	XR_NEXT_NO_PC();
}

static XrInstImplF XrVirtualShiftInstructionTable[4] = {
	[0] = &XrExecuteVirtualLsh,
	[1] = &XrExecuteVirtualRsh,
	[2] = &XrExecuteVirtualAsh,
	[3] = &XrExecuteVirtualRor
};

static XrInstImplF XrRegShiftFunctionTable[4] = {
	[0] = &XrExecuteLsh,
	[1] = &XrExecuteRsh,
	[2] = &XrExecuteAsh,
	[3] = &XrExecuteRor
};

#define XR_REDIRECT_ZERO_SRC(src) ((((proc->Cr[RS] & RS_TBMISS) == 0) && ((src) == 0)) ? XR_FAKE_ZERO_REGISTER : (src))

typedef XrCachedInst *(*XrDecodeInstructionF)(XrProcessor *proc, XrCachedInst *inst, uint32_t ir, uint32_t pc);

static XrCachedInst *XrDecodeIllegalInstruction(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteIllegalInstruction;

	return 0;
}

static XrCachedInst *XrDecodeRfe(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteRfe;

	return 0;
}

static XrCachedInst *XrDecodeHlt(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteHlt;

	return 0;
}

static XrCachedInst *XrDecodeMtcr(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMtcr;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm8_2 = (ir >> 16) & 31;

	return inst + 1;
}

static XrCachedInst *XrDecodeMfcr(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMfcr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = (ir >> 16) & 31;

	return inst + 1;
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

static XrCachedInst *XrDecodeSys(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSys;

	return inst + 1;
}

static XrCachedInst *XrDecodeBrk(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBrk;

	return inst + 1;
}

static XrCachedInst *XrDecodeWmb(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteWmb;

	return inst + 1;
}

static XrCachedInst *XrDecodeMb(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteMb;

	return inst + 1;
}

static XrCachedInst *XrDecodePause(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecutePause;

	return inst + 1;
}

static XrCachedInst *XrDecodeSC(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSC;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	return inst + 1;
}

static XrCachedInst *XrDecodeLL(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLL;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);

	return inst + 1;
}

#define XR_INSERT_SHIFT() \
	if (((ir >> 21) & 31) != 0) { \
		/* The inline shift amount is nonzero, so generate a virtual shift */ \
		/* instruction before the proper one. */ \
\
		inst->Func = XrVirtualShiftInstructionTable[(ir >> 26) & 3]; \
		inst->Imm8_1 = rb; \
		inst->Imm8_2 = (ir >> 21) & 31; \
\
		/* Redirect the real instruction to take its RB register from the */ \
		/* result of the virtual instruction. */ \
\
		rb = XR_FAKE_SHIFT_SINK; \
\
		inst++; \
	}

static XrCachedInst *XrDecodeMod(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteMod;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeDivSigned(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteDivSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeDiv(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteDiv;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeMul(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteMul;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
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

static XrCachedInst *XrDecodeNor(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteNor;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeOr(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteOr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeXor(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteXor;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeAnd(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteAnd;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeSltSigned(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteSltSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeSlt(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteSlt;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeSub(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteSub;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeAdd(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteAdd;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeRegShifts(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = XrRegShiftFunctionTable[(ir >> 26) & 3];
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	return inst + 1;
}

static XrCachedInst *XrDecodeStoreLongRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteStoreLongRegOffset;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeStoreIntRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteStoreIntRegOffset;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeStoreByteRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteStoreByteRegOffset;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadLongRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteLoadLongRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadIntRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteLoadIntRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadByteRegOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	int rb = XR_REDIRECT_ZERO_SRC((ir >> 16) & 31);

	XR_INSERT_SHIFT();

	inst->Func = &XrExecuteLoadByteRegOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = rb;

	return inst + 1;
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

static XrCachedInst *XrDecodeLui(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteOri;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = (ir >> 16) << 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBpo(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBpo;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecodeStoreLongImmOffsetImm(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreLongImmOffsetImm;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 2;

	return inst + 1;
}

static XrCachedInst *XrDecodeOri(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteOri;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBpe(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBpe;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecodeStoreIntImmOffsetImm(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreIntImmOffsetImm;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = (ir >> 16) << 1;

	return inst + 1;
}

static XrCachedInst *XrDecodeXori(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteXori;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBge(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBge;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecodeStoreByteImmOffsetImm(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreByteImmOffsetImm;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = (ir >> 11) & 31;
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeAndi(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAndi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBle(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBle;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecodeSltiSigned(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSltiSigned;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = SignExt16(ir >> 16);

	return inst + 1;
}

static XrCachedInst *XrDecodeBgt(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBgt;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecode101001(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions101001[ir >> 28](proc, inst, ir, pc);
}

static XrCachedInst *XrDecodeStoreLongImmOffsetReg(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreLongImmOffsetReg;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = (ir >> 16) << 2;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadLongImmOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadLongImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = (ir >> 16) << 2;

	return inst + 1;
}

static XrCachedInst *XrDecodeSlti(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSlti;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBlt(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBlt;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecode110001(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions110001[ir >> 28](proc, inst, ir, pc);
}

static XrCachedInst *XrDecodeStoreIntImmOffsetReg(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreIntImmOffsetReg;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = (ir >> 16) << 1;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadIntImmOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadIntImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = (ir >> 16) << 1;

	return inst + 1;
}

static XrCachedInst *XrDecodeSubi(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteSubi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBne(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteBne;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);

	return 0;
}

static XrCachedInst *XrDecodeJalr(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJalr;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = SignExt18((ir >> 16) << 2);

	return 0;
}

static XrCachedInst *XrDecode111001(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeFunctions111001[ir >> 28](proc, inst, ir, pc);
}

static XrCachedInst *XrDecodeStoreByteImmOffsetReg(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteStoreByteImmOffsetReg;
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeLoadByteImmOffset(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteLoadByteImmOffset;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeAddi(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteAddi;
	inst->Imm8_1 = (ir >> 6) & 31;
	inst->Imm8_2 = XR_REDIRECT_ZERO_SRC((ir >> 11) & 31);
	inst->Imm32_1 = ir >> 16;

	return inst + 1;
}

static XrCachedInst *XrDecodeBeq(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Imm32_1 = SignExt23((ir >> 11) << 2);
	inst->Imm8_1 = XR_REDIRECT_ZERO_SRC((ir >> 6) & 31);

	if (inst->Imm8_1 == XR_FAKE_ZERO_REGISTER) {
		// This is a BEQ, ZERO, XXX instruction.
		// This is the canonical unconditional branch, generated by the B
		// pseudo-instruction. We can optimize this a bit.

		inst->Func = &XrExecuteB;

		return 0;
	}

	inst->Func = &XrExecuteBeq;

	return 0;
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

static XrCachedInst *XrDecodeJal(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJal;
	inst->Imm32_1 = (pc & 0x80000000) | ((ir >> 3) << 2);

	return 0;
}

static XrCachedInst *XrDecodeJ(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	inst->Func = &XrExecuteJ;
	inst->Imm32_1 = (pc & 0x80000000) | ((ir >> 3) << 2);

	return 0;
}

static XrCachedInst *XrDecodeMajor(XrProcessor* proc, XrCachedInst *inst, uint32_t ir, uint32_t pc) {
	return XrDecodeLowSix[ir & 63](proc, inst, ir, pc);
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
static void XrSpecialLinkageInstruction(XrProcessor *proc, XrIblock *block, XrCachedInst *inst) {
	DBGPRINT("exec 0\n");

	// This instruction is placed at the end of a basic block that didn't
	// terminate in a natural way (with a branch or illegal instruction).
	// It just directs the outer loop to look up the next block.

	XrIblock *iblock = block->CachedPaths[XR_TRUE_PATH];

	if (XrUnlikely(!iblock)) {
		iblock = XrDecodeInstructions(proc, block);

		if (XrUnlikely(!iblock)) {
			XR_EARLY_EXIT();
		}

		XrCreateCachedPointerToBlock(iblock, &block->CachedPaths[XR_TRUE_PATH]);
	}

	XR_DISPATCH(iblock);
}

static XrIblock *XrDecodeInstructions(XrProcessor *proc, XrIblock *hazard) {
	// Decode some instructions starting at the current virtual PC.
	// Return NULLPTR if we fail to fetch the first instruction. This implies
	// that an exception occurred, such as an ITB miss, page fault, or bus
	// error.

	uint32_t pc = proc->Pc;

	uint32_t asid = XR_CURRENT_ASID();

	XrIblock *iblock = XrLookupIblock(proc, pc, asid);

	if (XrLikely(iblock != 0)) {
		// Already cached.

		if (XrUnlikely((iblock->PteFlags & PTE_KERNEL) && (proc->Cr[RS] & RS_USER))) {
fault:
			proc->Cr[EBADADDR] = pc;
			XrBasicException(proc, XR_EXC_PGF, pc);

			return 0;
		}

		return iblock;
	}

#if !XR_SIMULATE_CACHES
	uint32_t fetchpc = pc;

	int instindex = 0;

#else
	// Round down to the last Icache line boundary so that we can fetch one line
	// at a time.

	uint32_t fetchpc = pc & ~(XR_IC_LINE_SIZE - 1);

	int instindex = (pc - fetchpc) >> 2;
#endif

	int instcount = XR_IBLOCK_INSTS;

	// Don't allow fetches to cross page boundaries.

	if (XrUnlikely(((fetchpc + XR_IBLOCK_INSTS_BYTES - 1) & 0xFFFFF000) != (pc & 0xFFFFF000))) {
		instcount = (((pc + 0x1000) & 0xFFFFF000) - fetchpc) >> 2;
	}

	// Translate the program counter.

	int flags = 0;

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		if (XrUnlikely(!XrTranslateIstream(proc, fetchpc, &fetchpc, &flags))) {
			return 0;
		}
	}

#if !XR_SIMULATE_CACHES
	uint32_t *ir = EBusTranslate(fetchpc);

	if (XrUnlikely(!ir)) {
		proc->Cr[EBADADDR] = pc;
		XrBasicException(proc, XR_EXC_BUS, pc);

		return 0;
	}
#else
	// Fetch instructions one line at a time.

	uint32_t ir[XR_IBLOCK_INSTS];

	for (int offset = 0;
		offset < instcount;
		offset += XR_IC_INST_PER_LINE, fetchpc += XR_IC_LINE_SIZE) {

		uint32_t *instptr = XrIcacheAccess(proc, fetchpc);

		if (XrUnlikely(!instptr)) {
			return 0;
		}

		CopyWithLength(&ir[offset], instptr, XR_IC_LINE_SIZE);
	}
#endif

	// Allocate an Iblock.

	iblock = XrAllocateIblock(proc, hazard);

	iblock->Pc = pc;
	iblock->Asid = asid;
	iblock->Cycles = 0;
	iblock->CachedByFifoIndex = 0;
	iblock->PteFlags = flags;
	iblock->HasPtable = 0;

	for (int i = 0; i < XR_CACHED_PATH_MAX; i++) {
		iblock->CachedPaths[i] = 0;
	}

	for (int i = 0; i < XR_IBLOCK_CACHEDBY_MAX; i++) {
		iblock->CachedBy[i] = 0;
	}

#ifdef FASTMEMORY
	for (int i = 0; i < XR_IBLOCK_DTB_CACHE_SIZE; i++) {
		iblock->DtbLoadCache[i].MatchingDtbe = TB_INVALID_MATCHING;
		iblock->DtbStoreCache[i].MatchingDtbe = TB_INVALID_MATCHING;
	}
#endif

	InsertAtHeadList(&proc->IblockHashBuckets[XR_IBLOCK_HASH(pc)], &iblock->HashEntry);
	InsertAtHeadList(&proc->IblockLruList, &iblock->LruEntry);

	XrInsertIblockInVpage(proc, iblock, pc);

	// Decode instructions starting at the offset of the program counter within
	// the fetched chunk, until we reach either a branch, an illegal
	// instruction, or the end of the chunk.

	// printf("decode %x %x %x %x %x %p\n", instindex, pc, fetchpc, ir[instindex], ir[instindex+1], ir);

	XrCachedInst *inst = &iblock->Insts[0];

	for (;
		instindex < instcount;
		instindex++, pc += 4) {

		iblock->Cycles++;

		// The decode routine returns a pointer to the next cached instruction,
		// or 0 if the basic block should be terminated.

		XrCachedInst *nextinst = XrDecodeLowThree[ir[instindex] & 7](proc, inst, ir[instindex], pc);

		if (nextinst == 0) {
			goto done_no_linkage;
		}

		inst = nextinst;

		if (inst >= &iblock->Insts[XR_IBLOCK_INSTS]) {
			break;
		}
	}

	// In case we went right up to the end of the basic block's maximum extent
	// without running into a control flow instruction, we want this special
	// linkage instruction at the end which just directs the interpreter to the
	// next basic block. There's an extra instruction slot in the basic block to
	// make sure there's room for this.

	inst->Func = &XrSpecialLinkageInstruction;

done_no_linkage:

	return iblock;
}

int XrExecuteFast(XrProcessor *proc, uint32_t cycles, uint32_t dt) {
	if (!proc->Running) {
		return cycles;
	}

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

	if (proc->Halted) {
		// We're halted.

		proc->NmiMaskCounter = 0;

		if (!lsic->InterruptPending || (proc->Cr[RS] & RS_INT) == 0) {
			// Interrupts are disabled or there is no interrupt pending. Just
			// return.

			// N.B. There's an assumption here that the host platform will make
			// writes by other host cores to the interrupt pending flag visible
			// to us in a timely manner, without needing any locking.

			return cycles;
		}

		// Interrupts are enabled and there is an interrupt pending.
		// Un-halt the processor.

		proc->Halted = 0;
	}

	proc->CyclesGoal = cycles;
	proc->CyclesDone = 0;

	if (proc->UserBreak && !proc->NmiMaskCounter) {
		// There's a pending user-initiated NMI, so do that.

		XrBasicException(proc, XR_EXC_NMI, proc->Pc);
		proc->UserBreak = 0;
		proc->Halted = 0;
	}

	if (proc->Progress <= 0) {
		// This processor did a poll-y looking thing too many times this
		// tick. Skip the rest of the tick so as not to eat up too much of
		// the host's CPU.

		return cycles;
	}

	proc->PauseCalls = 0;
	proc->NoMore = 0;

	while (!proc->NoMore) {
		// Call XrCheckConditions to start the execution chain.
		// It can return early if the tail-call chain fails due to some
		// exceptional event. In that case we want to just continue.

		XrCheckConditions(proc, 0, 0);
	}

	return proc->CyclesDone;
}

void XrProcessorSchedule(XrSchedulable *schedulable) {
	XrProcessor *proc = schedulable->Context;

	int cyclesperms = (XrProcessorFrequency+999)/1000;

	int timeslice = schedulable->Timeslice;

	while (timeslice > 0) {
		if (RTCIntervalMS && proc->TimerInterruptCounter >= RTCIntervalMS) {
			// Interval timer ran down, send self the interrupt.
			// We do this from the context of each cpu thread so that we get
			// accurate amounts of CPU time between each tick.

			LsicInterruptTargeted(proc, 2);

			proc->TimerInterruptCounter = 0;
		}

		if (proc->Id == 0) {
			// The zeroth thread also does the RTC intervals, once per
			// millisecond of CPU time. 

			RTCUpdateRealTime();
		}

		int realcycles = XrExecuteFast(proc, cyclesperms, 1);

		proc->CyclesThisRound += realcycles;

		if (proc->CyclesThisRound >= cyclesperms) {
			// A millisecond worth of cycles has been executed, so
			// decrement the timeslice and advance the timer interrupt
			// counter.

			proc->CyclesThisRound -= cyclesperms;
			timeslice -= 1;
			proc->TimerInterruptCounter += 1;
		}

		if (proc->PauseCalls >= XR_PAUSE_MAX || proc->Halted || proc->Progress <= 0) {
			// Halted or paused. Advance to next CPU.

			proc->PauseCalls = 0;

			break;
		}
	}

	schedulable->Timeslice = timeslice;

	if (timeslice == 0) {
		XrScheduleWorkForNextFrame(schedulable, 0);
	} else {
		XrScheduleWork(schedulable);
	}
}

void XrProcessorStartTimeslice(XrSchedulable *schedulable, int dt) {
	XrProcessor *proc = schedulable->Context;

	proc->Progress = XR_POLL_MAX;
	proc->PauseCalls = 0;
	proc->CyclesThisRound = 0;

	schedulable->Timeslice += dt;

	if (schedulable->Timeslice >= XR_STEP_MS * 50) {
		// The CPU has too much pending time. The threads are running
		// behind; they can't keep up with the simulated workload. Reset
		// the timeslice to avoid the threads running infinitely and
		// burning someone's lap.

		schedulable->Timeslice = XR_STEP_MS;
	}
}

void XrInitializeProcessor(int id) {
	XrProcessor *proc = malloc(sizeof(XrProcessor));

	if (!proc) {
		fprintf(stderr, "failed to allocate cpu %d\n", id);
		exit(1);
	}

	XrInitializeSchedulable(&proc->Schedulable, &XrProcessorSchedule, &XrProcessorStartTimeslice, proc);

	XrProcessorTable[id] = proc;
	proc->Id = id;
	proc->TimerInterruptCounter = 0;
	proc->CyclesThisRound = 0;

	proc->IblockFreeList = 0;
	proc->PtableFreeList = 0;
	proc->VpageFreeList = 0;

	InitializeList(&proc->IblockLruList);

	for (int i = 0; i < XR_IBLOCK_HASH_BUCKETS; i++) {
		InitializeList(&proc->IblockHashBuckets[i]);
	}

	for (int i = 0; i < XR_VPN_BUCKETS; i++) {
		InitializeList(&proc->VpageHashBuckets[i]);
	}

	XrIblock *iblocks = malloc(sizeof(XrIblock) * XR_IBLOCK_COUNT);

	if (!iblocks) {
		fprintf(stderr, "failed to allocate iblocks for cpu %d\n", id);
		exit(1);
	}

	for (int i = 0; i < XR_IBLOCK_COUNT; i++) {
		iblocks->HashEntry.Next = (void *)proc->IblockFreeList;
		proc->IblockFreeList = iblocks;

		iblocks++;
	}

	XrJalrPredictionTable *ptable = malloc(sizeof(XrJalrPredictionTable) * XR_IBLOCK_COUNT);

	if (!ptable) {
		fprintf(stderr, "failed to allocate ptables for cpu %d\n", id);
		exit(1);
	}

	for (int i = 0; i < XR_IBLOCK_COUNT; i++) {
		ptable->Iblocks[0] = (void *)proc->PtableFreeList;
		proc->PtableFreeList = ptable;

		ptable++;
	}

	XrVirtualPage *vpage = malloc(sizeof(XrVirtualPage) * XR_IBLOCK_COUNT);

	if (!vpage) {
		fprintf(stderr, "failed to allocate virtual page trackers for cpu %d\n", id);
		exit(1);
	}

	for (int i = 0; i < XR_IBLOCK_COUNT; i++) {
		vpage->VpnHashEntry.Next = (void *)proc->VpageFreeList;
		proc->VpageFreeList = vpage;

		InitializeList(&vpage->IblockVpnList);

		vpage++;
	}

	XrReset(proc);

#ifndef SINGLE_THREAD_MP
#if XR_SIMULATE_CACHES
	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		XrInitializeMutex(&proc->CacheMutexes[i]);
	}
#endif

	XrInitializeMutex(&proc->InterruptLock);
#endif

	XrScheduleWorkForNextFrame(&proc->Schedulable, 0);
}

void XrInitializeProcessors(void) {
#if XR_SIMULATE_CACHES
	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		XrInitializeMutex(&ScacheMutexes[i]);
	}
#endif

#if defined(FASTMEMORY) && !defined(SINGLE_THREAD_MP)
	for (int i = 0; i < XR_CLAIM_TABLE_SIZE; i++) {
		XrInitializeMutex(&XrClaimTable[i].Lock);
	}
#endif

	for (int id = 0; id < XrProcessorCount; id++) {
		XrInitializeProcessor(id);
	}
}