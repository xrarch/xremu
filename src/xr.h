#ifndef XR_H
#define XR_H

#include "queue.h"
#include "fastmutex.h"

#include "scheduler.h"

// Configurable stall parameters.

#define XR_UNCACHED_STALL 3
#define XR_MISS_STALL (XR_UNCACHED_STALL + 1)

// Configurable TB size parameters.

#define XR_DTB_SIZE_LOG 5
#define XR_ITB_SIZE_LOG 5

// Configurable ICache and DCache size parameters.

#define XR_IC_LINE_COUNT_LOG 11
#define XR_DC_LINE_COUNT_LOG 11
#define XR_IC_LINE_SIZE_LOG 4 // WARNING  1<<4=16 bytes is special cased in CopyWithLength.
#define XR_DC_LINE_SIZE_LOG 4 // WARNING  1<<4=16 bytes is special cased in CopyWithLength.
#define XR_IC_WAY_LOG 1
#define XR_DC_WAY_LOG 1

#define XR_SC_LINE_COUNT_LOG (XR_DC_LINE_COUNT_LOG + 2)
#define XR_SC_LINE_SIZE_LOG XR_DC_LINE_SIZE_LOG
#define XR_SC_WAY_LOG 0

// Configurable write buffer size parameters.

#define XR_WB_LOG 2

// TB size constants.
// Don't change these directly.

#define XR_DTB_SIZE (1 << XR_DTB_SIZE_LOG)
#define XR_ITB_SIZE (1 << XR_ITB_SIZE_LOG)

// ICache and DCache size constants.
// Don't change these directly.

#define XR_IC_SET_LOG (XR_IC_LINE_COUNT_LOG - XR_IC_WAY_LOG)
#define XR_DC_SET_LOG (XR_DC_LINE_COUNT_LOG - XR_DC_WAY_LOG)
#define XR_SC_SET_LOG (XR_SC_LINE_COUNT_LOG - XR_SC_WAY_LOG)

#define XR_IC_SETS (1 << XR_IC_SET_LOG)
#define XR_DC_SETS (1 << XR_DC_SET_LOG)
#define XR_IC_LINE_COUNT (1 << XR_IC_LINE_COUNT_LOG)
#define XR_DC_LINE_COUNT (1 << XR_DC_LINE_COUNT_LOG)
#define XR_IC_LINE_SIZE (1 << XR_IC_LINE_SIZE_LOG)
#define XR_DC_LINE_SIZE (1 << XR_DC_LINE_SIZE_LOG)
#define XR_IC_WAYS (1 << XR_IC_WAY_LOG)
#define XR_DC_WAYS (1 << XR_DC_WAY_LOG)
#define XR_IC_BYTE_COUNT (1 << (XR_IC_LINE_COUNT_LOG + XR_IC_LINE_SIZE_LOG))
#define XR_DC_BYTE_COUNT (1 << (XR_DC_LINE_COUNT_LOG + XR_DC_LINE_SIZE_LOG))

#define XR_IC_INST_PER_LINE (XR_IC_LINE_SIZE / 4)

#define XR_SC_SETS (1 << XR_SC_SET_LOG)
#define XR_SC_LINE_COUNT (1 << XR_SC_LINE_COUNT_LOG)
#define XR_SC_LINE_SIZE (1 << XR_SC_LINE_SIZE_LOG)
#define XR_SC_WAYS (1 << XR_SC_WAY_LOG)
#define XR_SC_BYTE_COUNT (1 << (XR_SC_LINE_COUNT_LOG + XR_SC_LINE_SIZE_LOG))

// Write buffer size constants.
// Don't change these directly.

#define XR_WB_DEPTH (1 << XR_WB_LOG)
#define XR_WB_BYTE_COUNT (1 << (XR_WB_LOG + XR_DC_LINE_SIZE_LOG))

// Exception codes.

#define XR_EXC_INT 1
#define XR_EXC_SYS 2
#define XR_EXC_BUS 4
#define XR_EXC_NMI 5
#define XR_EXC_BRK 6
#define XR_EXC_INV 7
#define XR_EXC_PRV 8
#define XR_EXC_UNA 9
#define XR_EXC_PGF 12
#define XR_EXC_PGW 13
#define XR_EXC_ITB 14
#define XR_EXC_DTB 15

#define XR_NONCACHED_PHYS_BASE 0xC0000000

// Maximum number of times a processor can poll an I/O device before it loses
// the rest of its tick. Reset before each 20ms tick and also when an interrupt
// is received.

#define XR_POLL_MAX 32

// Number of times the PAUSE instruction can be executed before the timeslice is
// yielded to another CPU.

#define XR_PAUSE_MAX 256

#define XR_CACHE_MUTEXES 256

#define XR_IC_SET_NUMBER(tag) ((tag >> XR_IC_LINE_SIZE_LOG) & (XR_IC_SETS - 1))
#define XR_DC_SET_NUMBER(tag) ((tag >> XR_DC_LINE_SIZE_LOG) & (XR_DC_SETS - 1))
#define XR_SC_SET_NUMBER(tag) ((tag >> XR_SC_LINE_SIZE_LOG) & (XR_SC_SETS - 1))

#define XR_MUTEX_INDEX(setnumber) (setnumber & (XR_CACHE_MUTEXES - 1))

typedef struct _XrClaimTableEntry {
#ifndef SINGLE_THREAD_MP
	XrMutex Lock;
#endif
	uint8_t ClaimedBy;
} XrClaimTableEntry;

#define XR_CLAIM_TABLE_SIZE 16384

extern XrClaimTableEntry XrClaimTable[XR_CLAIM_TABLE_SIZE];

#define XR_WB_INDEX_INVALID 255
#define XR_CACHE_INDEX_INVALID 0xFFFFFFFF

#define XR_IBLOCK_DTB_CACHE_SIZE 4

#define XR_IBLOCK_DTB_CACHE_INDEX(address) ((address >> 12) & (XR_IBLOCK_DTB_CACHE_SIZE - 1))

// XR_IBLOCK_INSTS should be defined as a multiple of the Icache line size,
// because the instruction decode logic fetches lines at a time.

#define XR_IBLOCK_INSTS_LOG 3
#define XR_IBLOCK_HASH_BUCKETS 256
#define XR_IBLOCK_COUNT 2048
#define XR_IBLOCK_RECLAIM 32

#define XR_VPN_BUCKETS 32
#define XR_VPN_BUCKET_INDEX(pc) ((pc >> 12) & (XR_VPN_BUCKETS - 1))

#define XR_IBLOCK_CACHEDBY_MAX 4

// Don't modify this XR_IBLOCK_INSTS, modify XR_IBLOCK_INSTS_LOG.

#define XR_IBLOCK_INSTS ((XR_IC_LINE_SIZE >> 2) << XR_IBLOCK_INSTS_LOG)
#define XR_IBLOCK_INSTS_BYTES (XR_IBLOCK_INSTS * 4)

#define XR_IBLOCK_HASH(pc) ((pc >> 2) & (XR_IBLOCK_HASH_BUCKETS - 1))

typedef struct _XrProcessor XrProcessor;
typedef struct _XrIblock XrIblock;
typedef struct _XrCachedInst XrCachedInst;

#define XR_JALR_PREDICTION_TABLE_ENTRIES 8

typedef struct _XrJalrPredictionTable {
	XrIblock *Iblocks[XR_JALR_PREDICTION_TABLE_ENTRIES];
	uint32_t Pcs[XR_JALR_PREDICTION_TABLE_ENTRIES];
} XrJalrPredictionTable;

typedef struct _XrVirtualPage {
	ListEntry VpnHashEntry;
	ListEntry IblockVpnList;
	uint32_t Vpn;
	uint32_t References;
} XrVirtualPage;

typedef void (*XrInstImplF XR_PRESERVE_NONE)(XrProcessor *proc, XrIblock *block, XrCachedInst *inst);

struct _XrCachedInst {
	XrInstImplF Func;
	uint32_t Imm32_1;
	uint8_t Imm8_1;
	uint8_t Imm8_2;
};

#define XR_INVALID_DTB_INDEX 0xFFFFFFFF

typedef struct _XrIblockDtbEntry {
	void *HostPointer;
	uint64_t MatchingDtbe;
	uint32_t Index;
} XrIblockDtbEntry;

#define XR_TRUE_PATH 0
#define XR_FALSE_PATH 1

#define XR_CACHED_PATH_MAX 2

struct _XrIblock {
#ifdef FASTMEMORY
	XrIblockDtbEntry DtbLoadCache[XR_IBLOCK_DTB_CACHE_SIZE];
	XrIblockDtbEntry DtbStoreCache[XR_IBLOCK_DTB_CACHE_SIZE];
#endif

	ListEntry VpageEntry;
	ListEntry HashEntry;
	ListEntry LruEntry;

	XrVirtualPage *Vpage;

	XrIblock *CachedPaths[XR_CACHED_PATH_MAX];

	// The CachedBy array stores a list of backpointers to pointers to this
	// block. When this block is invalidated, we can iterate this array and
	// zero out these pointers, thereby invalidating cached pointers to this
	// Iblock from other Iblocks. The CachedByFifoIndex field is used to
	// implement the "replacement policy".

	XrIblock **CachedBy[XR_IBLOCK_CACHEDBY_MAX];

	uint32_t Pc;
	uint32_t Asid;
	uint8_t Cycles;
	uint8_t CachedByFifoIndex;
	uint8_t PteFlags;
	uint8_t HasPtable;

	// TWO extra instructions: One is reserved for if a real instruction decodes
	// into two virtual instructions (happens for example with inline shifts),
	// to avoid special casing if there's no room for the virtual instruction.
	// The second is for the special linkage instruction placed at the end of a
	// basic block that doesn't otherwise terminate naturally. Both slots are
	// needed for the case where both of these situations occur.

	XrCachedInst Insts[XR_IBLOCK_INSTS + 2];
};

enum XrFakeRegisters {
	XR_FAKE_ZERO_REGISTER = 32,
	XR_FAKE_SHIFT_SINK,

	XR_REG_MAX,
};

#ifdef FASTMEMORY

#define XR_SIMULATE_CACHES 0
#define XR_SIMULATE_CACHE_STALLS 0

#else

#define XR_SIMULATE_CACHES 1
#define XR_SIMULATE_CACHE_STALLS 0

#endif

struct _XrProcessor {
	uint64_t Itb[XR_ITB_SIZE];
	uint64_t Dtb[XR_DTB_SIZE];

	uint64_t ItbLastResult;
	uint32_t ItbLastVpn;

	uint32_t DtbLastVpn;

#ifdef FASTMEMORY
	XrIblockDtbEntry DtbLastEntry;
#else
	uint32_t DtbLastResult;
#endif

#if XR_SIMULATE_CACHES
	XrMutex CacheMutexes[XR_CACHE_MUTEXES];
#endif
	XrMutex InterruptLock;

	XrIblock *IblockFreeList;
	XrJalrPredictionTable *PtableFreeList;
	XrVirtualPage *VpageFreeList;

	ListEntry IblockLruList;
	ListEntry IblockHashBuckets[XR_IBLOCK_HASH_BUCKETS];

#if XR_SIMULATE_CACHES
	uint32_t IcTags[XR_IC_LINE_COUNT];
	uint32_t DcTags[XR_DC_LINE_COUNT];
	uint32_t WbIndices[XR_WB_DEPTH];
#endif

	uint32_t TimerInterruptCounter;

	uint32_t WbFillIndex;
	uint32_t WbWriteIndex;
	uint32_t WbCycles;

	uint32_t Reg[XR_REG_MAX];
	uint32_t Cr[32];
	uint32_t Pc;

	uint8_t Dispatches;

#if XR_SIMULATE_CACHE_STALLS
	uint32_t StallCycles;
#endif
	uint32_t Id;
	int32_t Progress;
	uint32_t CyclesDone;
	uint32_t CyclesGoal;
	uint32_t PauseCalls;
	uint32_t CyclesThisRound;

	XrSchedulable Schedulable;

#if XR_SIMULATE_CACHES
	uint32_t IcReplacementIndex;
	uint32_t DcReplacementIndex;
#endif

#ifdef PROFCPU
	uint32_t DcMissCount;
	uint32_t DcHitCount;

	uint32_t IcMissCount;
	uint32_t IcHitCount;

	int32_t TimeToNextPrint;
#endif


#if XR_SIMULATE_CACHES
	uint8_t Ic[XR_IC_BYTE_COUNT];
	uint8_t Dc[XR_DC_BYTE_COUNT];

	uint8_t IcFlags[XR_IC_LINE_COUNT];
	uint8_t DcFlags[XR_DC_LINE_COUNT];
	uint8_t DcIndexToWbIndex[XR_DC_LINE_COUNT];
#endif

	uint8_t NmiMaskCounter;
	uint8_t DcLastFlags;
	uint8_t Locked;
	uint8_t LastTbMissWasWrite;
	uint8_t UserBreak;
	uint8_t Halted;
	uint8_t Running;
	uint8_t NoMore;

	ListEntry VpageHashBuckets[XR_VPN_BUCKETS];
};

extern uint8_t XrPrintCache;

extern int XrProcessorCount;

extern XrProcessor *XrProcessorTable[XR_PROC_MAX];

extern void XrReset(XrProcessor *proc);
extern int XrExecuteFast(XrProcessor *proc, uint32_t cycles, uint32_t dt);

extern void XrInitializeProcessors(void);

extern long XrProcessorFrequency;

#ifndef EMSCRIPTEN

#if XR_SIMULATE_CACHES

extern XrMutex XrScacheMutexes[XR_CACHE_MUTEXES];

static inline void XrLockCache(XrProcessor *proc, uint32_t tag) {
	XrLockMutex(&proc->CacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrUnlockCache(XrProcessor *proc, uint32_t tag) {
	XrUnlockMutex(&proc->CacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrLockScache(uint32_t tag) {
	XrLockMutex(&XrScacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrUnlockScache(uint32_t tag) {
	XrUnlockMutex(&XrScacheMutexes[XR_MUTEX_INDEX(tag)]);
}

#endif

static inline void XrLockInterrupt(XrProcessor *proc) {
	XrLockMutex(&proc->InterruptLock);
}

static inline void XrUnlockInterrupt(XrProcessor *proc) {
	XrUnlockMutex(&proc->InterruptLock);
}

#else

static inline void XrLockCache(XrProcessor *proc, uint32_t tag) {}
static inline void XrUnlockCache(XrProcessor *proc, uint32_t tag) {}
static inline void XrLockScache(uint32_t tag) {}
static inline void XrUnlockScache(uint32_t tag) {}
static inline void XrLockInterrupt(XrProcessor *proc) {}
static inline void XrUnlockInterrupt(XrProcessor *proc) {}

#endif

static inline void XrDecrementProgress(XrProcessor *proc, int ints) {
	if (!ints || (proc->Cr[0] & 2) == 0) {
		proc->Progress--;
	}
}

#endif // XR_H