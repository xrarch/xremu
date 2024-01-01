#define LSIC_MASK_0 0
#define LSIC_MASK_1 1
#define LSIC_PENDING_0 2
#define LSIC_PENDING_1 3
#define LSIC_CLAIM_COMPLETE 4
#define LSIC_IPL 5

#define LSIC_REGISTERS 6

typedef struct _Lsic {
	uint32_t Registers[LSIC_REGISTERS];

	uint32_t LowIplMask;
	uint32_t HighIplMask;
	uint8_t InterruptPending;
	uint8_t Enabled;
} Lsic;

extern Lsic LsicTable[];
extern int LsicWrite(int reg, uint32_t value);
extern int LsicRead(int reg, uint32_t *value);
extern void LsicReset();
extern void LsicInterrupt(int intsrc);
extern void LsicEnable(int id);