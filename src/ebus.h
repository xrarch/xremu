int EBusInit(uint32_t memsize);

#define EBUSBRANCHSIZE (128 * 1024 * 1024)
#define EBUSBRANCHES 32

enum EBusAccessType {
	EBUSBYTE,
	EBUSINT,
	EBUSLONG
};

enum EBusSuccess {
	EBUSSUCCESS,
	EBUSERROR
};

typedef int (*EBusWriteF)(uint32_t address, uint32_t type, uint32_t value);
typedef int (*EBusReadF)(uint32_t address, uint32_t type, uint32_t *value);
typedef void (*EBusResetF)();

struct EBusBranch {
	int Present;
	EBusWriteF Write;
	EBusReadF Read;
	EBusResetF Reset;
};

struct EBusBranch EBusBranches[EBUSBRANCHES];

int EBusRead(uint32_t address, uint32_t type, uint32_t *value);

int EBusWrite(uint32_t address, uint32_t type, uint32_t value);

void EBusReset();

#define EBUSSLOTSTART 24