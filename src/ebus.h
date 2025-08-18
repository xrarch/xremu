#ifndef XR_EBUS_H
#define XR_EBUS_H

#include <stdint.h>
#include <string.h>

#include "xrdefs.h"

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
typedef void *(*EBusTranslateF)(uint32_t address);
typedef void (*EBusResetF)();

struct EBusBranch {
	int Present;
	EBusWriteF Write;
	EBusReadF Read;
	EBusTranslateF Translate;
	EBusResetF Reset;
};

extern struct EBusBranch EBusBranches[EBUSBRANCHES];

static XR_ALWAYS_INLINE int EBusRead(uint32_t address, void *dest, uint32_t length, void *proc) {
	return EBusBranches[address >> 27].Read(address & 0x7FFFFFF, dest, length, proc);
}

static XR_ALWAYS_INLINE int EBusWrite(uint32_t address, void *src, uint32_t length, void *proc) {
	return EBusBranches[address >> 27].Write(address & 0x7FFFFFF, src, length, proc);
}

static XR_ALWAYS_INLINE void *EBusTranslate(uint32_t address) {
	return EBusBranches[address >> 27].Translate(address & 0x7FFFFFF);
}

static XR_ALWAYS_INLINE void CopyWithLength(void *dest, void *src, uint32_t length) {
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

static XR_ALWAYS_INLINE void CopyWithLengthZext(void *dest, void *src, uint32_t length) {
	switch (length) {
		case 1:
			*(uint32_t*)(dest) = *(uint8_t*)(src);
			break;

		case 2:
			*(uint32_t*)(dest) = *(uint16_t*)(src);
			break;

		case 4:
			*(uint32_t*)(dest) = *(uint32_t*)(src);
			break;
	}
}

void EBusReset();

#define EBUSSLOTSTART 24

#endif // XR_EBUS_H