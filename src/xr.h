#ifndef XR_H
#define XR_H

#include "queue.h"
#include "fastmutex.h"

#define XrLikely(x)       __builtin_expect(!!(x), 1)
#define XrUnlikely(x)     __builtin_expect(!!(x), 0)

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

#define XR_PROC_MAX 8

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

#define XR_WB_INDEX_INVALID 255
#define XR_CACHE_INDEX_INVALID 0xFFFFFFFF

// XR_IBLOCK_INSTS should be defined as a multiple of the Icache line size,
// because the instruction decode logic fetches lines at a time.

#define XR_IBLOCK_INSTS_LOG 3
#define XR_IBLOCK_HASH_BUCKETS 256
#define XR_IBLOCK_COUNT 2048
#define XR_IBLOCK_RECLAIM 32

#define XR_IBLOCK_CACHEDBY_MAX 4

// Don't modify this XR_IBLOCK_INSTS, modify XR_IBLOCK_INSTS_LOG.

#define XR_IBLOCK_INSTS ((XR_IC_LINE_SIZE >> 2) << XR_IBLOCK_INSTS_LOG)
#define XR_IBLOCK_INSTS_BYTES (XR_IBLOCK_INSTS * 4)

#define XR_IBLOCK_HASH(pc) ((pc >> 2) & (XR_IBLOCK_HASH_BUCKETS - 1))

#define XR_PRESERVE_NONE [[clang::preserve_none]]

#define XR_TAIL [[clang::musttail]]

typedef struct _XrProcessor XrProcessor;
typedef struct _XrIblock XrIblock;
typedef struct _XrCachedInst XrCachedInst;

typedef void (*XrInstImplF XR_PRESERVE_NONE)(XrProcessor *proc, XrIblock *block, XrCachedInst *inst);

struct _XrCachedInst {
	XrInstImplF Func;
	uint32_t Imm32_1;
	uint8_t Imm8_1;
	uint8_t Imm8_2;
};

struct _XrIblock {
	ListEntry HashEntry;
	ListEntry LruEntry;

	XrIblock *TruePath;
	XrIblock *FalsePath;

	XrIblock **CachedBy[XR_IBLOCK_CACHEDBY_MAX];

	uint32_t Asid;
	uint32_t Pc;
	uint8_t Cycles;
	uint8_t CachedByFifoIndex;
	uint8_t PteFlags;

	// Two extra instructions. One is reserved for if a real instruction decodes
	// into two virtual instructions (happens for example with inline shifts),
	// to avoid special casing if there's no room for the virtual instruction.
	// The second is for the special linkage instruction placed at the end of a
	// basic block that doesn't otherwise terminate naturally.

	XrCachedInst Insts[XR_IBLOCK_INSTS + 2];
};

enum XrFakeRegisters {
	XR_FAKE_ZERO_REGISTER = 32,
	XR_FAKE_SHIFT_SINK,

	XR_REG_MAX,
};

struct _XrProcessor {
	uint64_t Itb[XR_ITB_SIZE];
	uint64_t Dtb[XR_DTB_SIZE];

	uint64_t ItbLastResult;
	uint64_t DtbLastResult;

	XrMutex CacheMutexes[XR_CACHE_MUTEXES];
	XrSemaphore LoopSemaphore;
	XrMutex InterruptLock;
	XrMutex RunLock;

	XrIblock *IblockFreeList;

	ListEntry IblockLruList;
	ListEntry IblockHashBuckets[XR_IBLOCK_HASH_BUCKETS];

	uint32_t IcTags[XR_IC_LINE_COUNT];
	uint32_t DcTags[XR_DC_LINE_COUNT];
	uint32_t WbIndices[XR_WB_DEPTH];

	uint32_t TimerInterruptCounter;

	uint32_t ItbLastVpn;
	uint32_t DtbLastVpn;

	uint32_t WbFillIndex;
	uint32_t WbWriteIndex;
	uint32_t WbCycles;

	uint32_t Reg[XR_REG_MAX];
	uint32_t Cr[32];
	uint32_t Pc;

#if XR_SIMULATE_CACHE_STALLS
	uint32_t StallCycles;
#endif
	uint32_t Id;
	int32_t Progress;
	uint32_t CyclesDone;
	uint32_t CyclesGoal;
	uint32_t PauseCalls;
	uint32_t Timeslice;
	uint32_t PauseReward;

	uint32_t IcReplacementIndex;
	uint32_t DcReplacementIndex;

#ifdef PROFCPU
	uint32_t DcMissCount;
	uint32_t DcHitCount;

	uint32_t IcMissCount;
	uint32_t IcHitCount;

	int32_t TimeToNextPrint;
#endif

	uint8_t Ic[XR_IC_BYTE_COUNT];
	uint8_t Dc[XR_DC_BYTE_COUNT];

	uint8_t IcFlags[XR_IC_LINE_COUNT];
	uint8_t DcFlags[XR_DC_LINE_COUNT];
	uint8_t DcIndexToWbIndex[XR_DC_LINE_COUNT];

	uint8_t NmiMaskCounter;
	uint8_t DcLastFlags;
	uint8_t Locked;
	uint8_t LastTbMissWasWrite;
	uint8_t UserBreak;
	uint8_t Halted;
	uint8_t Running;
	uint8_t Dispatches;
};

#define XR_SIMULATE_CACHES 1
#define XR_SIMULATE_CACHE_STALLS 0

extern uint8_t XrPrintCache;

extern uint32_t XrProcessorCount;

extern XrProcessor *CpuTable[XR_PROC_MAX];

extern void XrReset(XrProcessor *proc);
extern int XrExecuteFast(XrProcessor *proc, uint32_t cycles, uint32_t dt);

#ifndef EMSCRIPTEN

extern XrMutex ScacheMutexes[XR_CACHE_MUTEXES];

static inline void XrLockCache(XrProcessor *proc, uint32_t tag) {
	XrLockMutex(&proc->CacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrUnlockCache(XrProcessor *proc, uint32_t tag) {
	XrUnlockMutex(&proc->CacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrLockScache(uint32_t tag) {
	XrLockMutex(&ScacheMutexes[XR_MUTEX_INDEX(tag)]);
}

static inline void XrUnlockScache(uint32_t tag) {
	XrUnlockMutex(&ScacheMutexes[XR_MUTEX_INDEX(tag)]);
}

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

#endif // XR_H