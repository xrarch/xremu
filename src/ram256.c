#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"
#include "ram256.h"

uint8_t  *RAMSlots[RAMSLOTCOUNT];
uint32_t RAMSlotSizes[RAMSLOTCOUNT];

uint32_t RAMSize;

char *RAMSlotNames[RAMSLOTCOUNT] = {
	"bank0.bin",
	"bank1.bin",
	"bank2.bin",
	"bank3.bin",
	"bank4.bin",
	"bank5.bin",
	"bank6.bin",
	"bank7.bin"
};

void RAMDump() {
	// dump each bank

	for (int i = 0; i < RAMSLOTCOUNT; i++) {
		if (RAMSlotSizes[i] > 0) {
			// dump

			FILE *dumpfile = fopen(RAMSlotNames[i], "wb");

			if (dumpfile) {
				fwrite(RAMSlots[i], 1, RAMSlotSizes[i], dumpfile);
				fclose(dumpfile);
			}
		}
	}
}

int RAMWrite(uint32_t address, void *src, uint32_t length) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot])
		return EBUSERROR;

	memcpy(RAMSlots[slot]+offset, src, length);

	return EBUSSUCCESS;
}

int RAMRead(uint32_t address, void *dest, uint32_t length) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot])
		return EBUSERROR;

	memcpy(dest, RAMSlots[slot]+offset, length);

	return EBUSSUCCESS;
}

int RAMWriteExt(uint32_t address, void *src, uint32_t length) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot]){
		return EBUSERROR;
	}

	memcpy(RAMSlots[slot]+offset, src, length);

	return EBUSSUCCESS;
}

int RAMReadExt(uint32_t address, void *dest, uint32_t length) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot]){
		return EBUSERROR;
	}

	memcpy(dest, RAMSlots[slot]+offset, length);

	return EBUSSUCCESS;
}

int RAMInit(uint32_t memsize) {
	if (memsize > RAMMAXIMUM) {
		fprintf(stderr, "RAMInit: maximum is %d bytes (%d bytes given)\n", RAMMAXIMUM, memsize);
		return -1;
	}

	RAMSize = memsize;

	EBusBranches[0].Present = 1;
	EBusBranches[0].Write = RAMWrite;
	EBusBranches[0].Read = RAMRead;
	EBusBranches[0].Reset = 0;

	EBusBranches[1].Present = 1;
	EBusBranches[1].Write = RAMWriteExt;
	EBusBranches[1].Read = RAMReadExt;
	EBusBranches[1].Reset = 0;

	// try to stack the RAM into slots in units of 4MB.
	// add the remainder to the first slot.

	int i = 0;

	while (memsize >= 0x400000) {
		if (i >= RAMSLOTCOUNT)
			i = 0;

		RAMSlotSizes[i] += 0x400000;

		i += 1;
		memsize -= 0x400000;
	}

	RAMSlotSizes[0] += memsize;

	for (i = 0; i < RAMSLOTCOUNT; i++) {
		if (RAMSlotSizes[i])
			RAMSlots[i] = malloc(RAMSlotSizes[i]);
	}

	return 0;
}