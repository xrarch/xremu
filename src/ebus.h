#ifndef XR_EBUS_H
#define XR_EBUS_H

#include <stdint.h>
#include <string.h>

int EBusInit(uint32_t memsize);

#define EBUSBRANCHSIZE (128 * 1024 * 1024)
#define EBUSBRANCHES 32

enum EBusSuccess {
	EBUSSUCCESS,
	EBUSERROR
};

extern void EnqueueCallback(uint32_t interval, uint32_t (*callback)(uint32_t, void*), void *param);

typedef int (*EBusWriteF)(uint32_t address, void *src, uint32_t length, void *proc);
typedef int (*EBusReadF)(uint32_t address, void *dest, uint32_t length, void *proc);
typedef void (*EBusResetF)();

struct EBusBranch {
	int Present;
	EBusWriteF Write;
	EBusReadF Read;
	EBusResetF Reset;
};

extern struct EBusBranch EBusBranches[EBUSBRANCHES];

static inline int EBusRead(uint32_t address, void *dest, uint32_t length, void *proc) {
	return EBusBranches[address >> 27].Read(address & 0x7FFFFFF, dest, length, proc);
}

static inline int EBusWrite(uint32_t address, void *src, uint32_t length, void *proc) {
	return EBusBranches[address >> 27].Write(address & 0x7FFFFFF, src, length, proc);
}

static inline void CopyWithLength(void *dest, void *src, uint32_t length) {
	switch (length) {
		case 1:
			*(uint8_t*)(dest) = *(uint8_t*)(src);
			break;

		case 2:
			*(uint16_t*)(dest) = *(uint16_t*)(src);
			break;

		case 4:
			*(uint32_t*)(dest) = *(uint32_t*)(src);
			break;

		case 16:
			*(uint64_t*)(dest) = *(uint64_t*)(src);
			*(uint64_t*)(dest+8) = *(uint64_t*)(src+8);
			break;

		default:
			memcpy(dest, src, length);
			break;
	}
}

static inline void CopyWithLengthZext(void *dest, void *src, uint32_t length) {
	switch (length) {
		case 1:
			*(uint8_t*)(dest) = *(uint8_t*)(src);
			*(uint8_t*)(dest+1) = 0;
			*(uint16_t*)(dest+2) = 0;
			break;

		case 2:
			*(uint16_t*)(dest) = *(uint16_t*)(src);
			*(uint16_t*)(dest+2) = 0;
			break;

		case 4:
			*(uint32_t*)(dest) = *(uint32_t*)(src);
			break;
	}
}

void EBusReset();

#define EBUSSLOTSTART 24

#endif // XR_EBUS_H