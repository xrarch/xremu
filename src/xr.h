// Configurable stall parameters.

#define XR_UNCACHED_STALL 3
#define XR_MISS_STALL (XR_UNCACHED_STALL + 1)

// Configurable TB size parameters.

#define XR_DTB_SIZE_LOG 6
#define XR_ITB_SIZE_LOG 5

// Configurable ICache and DCache size parameters.

#define XR_IC_LINE_COUNT_LOG 14
#define XR_DC_LINE_COUNT_LOG 14
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

#define XR_POLL_MAX 8

typedef struct _XrProcessor {
	uint64_t Itb[XR_ITB_SIZE];
	uint64_t Dtb[XR_DTB_SIZE];

	uint64_t ItbLastResult;
	uint64_t DtbLastResult;

	void *CacheMutex;
	void *LoopSemaphore;

	uint32_t IcTags[XR_IC_LINE_COUNT];
	uint32_t DcTags[XR_DC_LINE_COUNT];
	uint32_t WbTags[XR_WB_DEPTH];

	uint32_t ItbLastVpn;
	uint32_t DtbLastVpn;

	uint32_t WbIndex;
	uint32_t WbSize;
	uint32_t WbCyclesTilNextWrite;

	uint32_t Reg[32];
	uint32_t Cr[32];
	uint32_t Pc;

	uint32_t StallCycles;
	uint32_t Id;
	int32_t Progress;

	uint32_t IcReplacementIndex;
	uint32_t DcReplacementIndex;

	uint32_t IcLastTag;
	uint32_t IcLastOffset;

	uint32_t DcLastTag;
	uint32_t DcLastIndex;

#ifdef PROFCPU
	uint32_t DcMissCount;
	uint32_t DcHitCount;

	uint32_t IcMissCount;
	uint32_t IcHitCount;

	int32_t TimeToNextPrint;
#endif

	uint8_t Ic[XR_IC_BYTE_COUNT];
	uint8_t Dc[XR_DC_BYTE_COUNT];
	uint8_t Wb[XR_WB_BYTE_COUNT];

	uint8_t IcFlags[XR_IC_LINE_COUNT];
	uint8_t DcFlags[XR_DC_LINE_COUNT];

	uint8_t NmiMaskCounter;
	uint8_t DcLastFlags;
	uint8_t Locked;
	uint8_t LastTbMissWasWrite;
	uint8_t IFetch;
	uint8_t UserBreak;
	uint8_t Halted;
	uint8_t Running;
} XrProcessor;

extern uint8_t XrSimulateCaches;
extern uint8_t XrSimulateCacheStalls;
extern uint8_t XrPrintCache;

extern uint32_t XrProcessorCount;

extern XrProcessor *CpuTable[XR_PROC_MAX];
extern XrProcessor *XrIoMutexProcessor;

extern void XrLockScache();
extern void XrUnlockScache();

extern void XrLockIoMutex(XrProcessor *proc);
extern void XrUnlockIoMutex();

extern void XrLockCache(XrProcessor *proc);
extern void XrUnlockCache(XrProcessor *proc);

extern void XrReset(XrProcessor *proc);
extern void XrExecute(XrProcessor *proc, uint32_t cycles, uint32_t dt);

extern void XrPokeCpu(XrProcessor *proc);