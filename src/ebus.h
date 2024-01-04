#include <string.h>

int EBusInit(uint32_t memsize);

#define EBUSBRANCHSIZE (128 * 1024 * 1024)
#define EBUSBRANCHES 32

enum EBusSuccess {
	EBUSSUCCESS,
	EBUSERROR
};

extern void LockIoMutex();
extern void UnlockIoMutex();

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
		return EBusBranches[branch].Read(address & 0x7FFFFFF, dest, length);
	} else if (branch >= 24) {
		*(uint32_t*)dest = 0;
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

static inline int EBusWrite(uint32_t address, void *src, uint32_t length) {
	int branch = address >> 27;

	if (EBusBranches[branch].Present) {
		return EBusBranches[branch].Write(address & 0x7FFFFFF, src, length);
	} else if (branch >= 24) {
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

static inline void CopyWithLength(void *dest, void *src, uint32_t length) {
	switch (length) {
		case 1:
			*(uint8_t*)dest = *(uint8_t*)src;
			break;

		case 2:
			*(uint8_t*)dest     = *(uint8_t*)src;
			*(uint8_t*)(dest+1) = *(uint8_t*)(src+1);

			break;

		case 4:
			*(uint8_t*)dest     = *(uint8_t*)src;
			*(uint8_t*)(dest+1) = *(uint8_t*)(src+1);
			*(uint8_t*)(dest+2) = *(uint8_t*)(src+2);
			*(uint8_t*)(dest+3) = *(uint8_t*)(src+3);

			break;

		case 16:
			*(uint8_t*)dest      = *(uint8_t*)src;
			*(uint8_t*)(dest+1)  = *(uint8_t*)(src+1);
			*(uint8_t*)(dest+2)  = *(uint8_t*)(src+2);
			*(uint8_t*)(dest+3)  = *(uint8_t*)(src+3);
			*(uint8_t*)(dest+4)  = *(uint8_t*)(src+4);
			*(uint8_t*)(dest+5)  = *(uint8_t*)(src+5);
			*(uint8_t*)(dest+6)  = *(uint8_t*)(src+6);
			*(uint8_t*)(dest+7)  = *(uint8_t*)(src+7);
			*(uint8_t*)(dest+8)  = *(uint8_t*)(src+8);
			*(uint8_t*)(dest+9)  = *(uint8_t*)(src+9);
			*(uint8_t*)(dest+10) = *(uint8_t*)(src+10);
			*(uint8_t*)(dest+11) = *(uint8_t*)(src+11);
			*(uint8_t*)(dest+12) = *(uint8_t*)(src+12);
			*(uint8_t*)(dest+13) = *(uint8_t*)(src+13);
			*(uint8_t*)(dest+14) = *(uint8_t*)(src+14);
			*(uint8_t*)(dest+15) = *(uint8_t*)(src+15);

			break;

		default:
			memcpy(dest, src, length);

			break;
	}
}

void EBusReset();

#define EBUSSLOTSTART 24