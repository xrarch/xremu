#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"
#include "kinnowfb.h"

uint32_t *KinnowFB = 0;

uint32_t FBSize;

uint32_t SlotInfo[64];

int KinnowWrite(uint32_t address, uint32_t type, uint32_t value) {
	if (address >= 0x100000) {
		address -= 0x100000;

		if (address >= FBSize)
			return EBUSERROR;

		switch(type) {
			case EBUSBYTE:
				((uint8_t*)KinnowFB)[address] = value;
				break;

			case EBUSINT:
				((uint16_t*)KinnowFB)[address/2] = value;
				break;

			case EBUSLONG:
				KinnowFB[address/4] = value;
				break;
		}

		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int KinnowRead(uint32_t address, uint32_t type, uint32_t *value) {
	if (address < 0x100) { // SlotInfo
		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)SlotInfo)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)SlotInfo)[address/2];
				break;

			case EBUSLONG:
				*value = SlotInfo[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if (address >= 0x100000) {
		address -= 0x100000;

		if (address >= FBSize)
			return EBUSERROR;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)KinnowFB)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)KinnowFB)[address/2];
				break;

			case EBUSLONG:
				*value = KinnowFB[address/4];
				break;
		}

		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int KinnowInit() {
	FBSize = KINNOW_FRAMEBUFFER_WIDTH * KINNOW_FRAMEBUFFER_HEIGHT * 2;

	KinnowFB = malloc(FBSize);

	if (KinnowFB == 0)
		return -1;

	EBusBranches[24].Present = 1;
	EBusBranches[24].Write = KinnowWrite;
	EBusBranches[24].Read = KinnowRead;
	EBusBranches[24].Reset = 0;

	memset(&SlotInfo, 0, 256);

	SlotInfo[0] = 0x0C007CA1; // ebus magic number
	SlotInfo[1] = 0x4B494E35; // board ID

	strcpy((char *) &SlotInfo[2], "kinnowfb,16");

	return 0;
}