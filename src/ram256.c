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

int RAMWrite(uint32_t address, void *src, uint32_t length, void *proc) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot])
		return EBUSERROR;

	CopyWithLength(&RAMSlots[slot][offset], src, length);

	return EBUSSUCCESS;
}

int RAMRead(uint32_t address, void *dest, uint32_t length, void *proc) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot])
		return EBUSERROR;

	CopyWithLength(dest, &RAMSlots[slot][offset], length);

	return EBUSSUCCESS;
}

void *RAMTranslate(uint32_t address) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot])
		return 0;

	return &RAMSlots[slot][offset];
}

int RAMWriteExt(uint32_t address, void *src, uint32_t length, void *proc) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot]){
		return EBUSERROR;
	}

	CopyWithLength(&RAMSlots[slot][offset], src, length);

	return EBUSSUCCESS;
}

int RAMReadExt(uint32_t address, void *dest, uint32_t length, void *proc) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset+length > RAMSlotSizes[slot]){
		return EBUSERROR;
	}

	CopyWithLength(dest, &RAMSlots[slot][offset], length);

	return EBUSSUCCESS;
}

void *RAMTranslateExt(uint32_t address) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot])
		return 0;

	return &RAMSlots[slot][offset];
}

int RAMInit() {
	EBusBranches[0].Present = 1;
	EBusBranches[0].Write = RAMWrite;
	EBusBranches[0].Read = RAMRead;
	EBusBranches[0].Translate = RAMTranslate;
	EBusBranches[0].Reset = 0;

	EBusBranches[1].Present = 1;
	EBusBranches[1].Write = RAMWriteExt;
	EBusBranches[1].Read = RAMReadExt;
	EBusBranches[1].Translate = RAMTranslateExt;
	EBusBranches[1].Reset = 0;

	for (int nodeid = 0; nodeid < XR_NODE_MAX; nodeid++) {
		uint32_t noderam = XrNumaNodes[nodeid].RamSize;

		if (noderam != 0) {
			// For realism, try to stack the RAM into the slots evenly.
			// try to stack the RAM into slots in units of 4MB.
			// add the remainder to the first slot.

			int i = 0;

			while (noderam >= 0x400000) {
				if (i >= SLOTS_PER_NODE)
					i = 0;

				RAMSlotSizes[nodeid * SLOTS_PER_NODE + i] += 0x400000;

				i += 1;
				noderam -= 0x400000;
			}

			RAMSlotSizes[nodeid * SLOTS_PER_NODE + 0] += noderam;
		}
	}

	for (int i = 0; i < RAMSLOTCOUNT; i++) {
		if (RAMSlotSizes[i])
			RAMSlots[i] = malloc(RAMSlotSizes[i]);
	}

	return 0;
}