int EBusInit(uint32_t memsize);

#define EBUSBRANCHSIZE (128 * 1024 * 1024)
#define EBUSBRANCHES 32

enum EBusSuccess {
	EBUSSUCCESS,
	EBUSERROR
};

typedef int (*EBusWriteF)(uint32_t address, void *src, uint32_t length);
typedef int (*EBusReadF)(uint32_t address, void *dest, uint32_t length);
typedef void (*EBusResetF)();

struct EBusBranch {
	int Present;
	EBusWriteF Write;
	EBusReadF Read;
	EBusResetF Reset;
};

extern struct EBusBranch EBusBranches[EBUSBRANCHES];

static inline int EBusRead(uint32_t address, void *dest, uint32_t length) {
	int branch = address >> 27;

	if (EBusBranches[branch].Present) {
		return EBusBranches[branch].Read(address&0x7FFFFFF, dest, length);
	} else if (branch >= 24) {
		*(uint32_t*)dest = 0;
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

static inline int EBusWrite(uint32_t address, void *src, uint32_t length) {
	int branch = address >> 27;

	if (EBusBranches[branch].Present) {
		return EBusBranches[branch].Write(address&0x7FFFFFF, src, length);
	} else if (branch >= 24) {
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

void EBusReset();

#define EBUSSLOTSTART 24