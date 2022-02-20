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

uint32_t *RAM = 0;

uint32_t *RAMSlots[RAMSLOTCOUNT];
uint32_t RAMSlotSizes[RAMSLOTCOUNT];

uint32_t RAMSize;

void RAMDump() {
	// TODO make work in a sane way again
}

int RAMWrite(uint32_t address, uint32_t type, uint32_t value) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot])
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			((uint8_t*)RAMSlots[slot])[offset] = value;
			break;

		case EBUSINT:
			((uint16_t*)RAMSlots[slot])[offset>>1] = value;
			break;

		case EBUSLONG:
			RAMSlots[slot][offset>>2] = value;
			break;
	}

	return EBUSSUCCESS;
}

int RAMRead(uint32_t address, uint32_t type, uint32_t *value) {
	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot])
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			*value = ((uint8_t*)RAMSlots[slot])[offset];
			break;

		case EBUSINT:
			*value = ((uint16_t*)RAMSlots[slot])[offset>>1];
			break;

		case EBUSLONG:
			*value = RAMSlots[slot][offset>>2];
			break;
	}

	return EBUSSUCCESS;
}

int RAMWriteExt(uint32_t address, uint32_t type, uint32_t value) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot]){
		printf("oh\n");
		return EBUSERROR;
	}

	switch(type) {
		case EBUSBYTE:
			((uint8_t*)RAMSlots[slot])[offset] = value;
			break;

		case EBUSINT:
			((uint16_t*)RAMSlots[slot])[offset>>1] = value;
			break;

		case EBUSLONG:
			RAMSlots[slot][offset>>2] = value;
			break;
	}

	return EBUSSUCCESS;
}

int RAMReadExt(uint32_t address, uint32_t type, uint32_t *value) {
	address += EBUSBRANCHSIZE;

	int slot = address >> 25;
	int offset = address & (RAMSLOTSIZE-1);

	if (offset >= RAMSlotSizes[slot]){
		printf("oh read\n");
		return EBUSERROR;
	}

	switch(type) {
		case EBUSBYTE:
			*value = ((uint8_t*)RAMSlots[slot])[offset];
			break;

		case EBUSINT:
			*value = ((uint16_t*)RAMSlots[slot])[offset>>1];
			break;

		case EBUSLONG:
			*value = RAMSlots[slot][offset>>2];
			break;
	}

	return EBUSSUCCESS;
}

int RAMDescWrite(uint32_t address, uint32_t type, uint32_t value) {
	return EBUSERROR;
}

int RAMDescRead(uint32_t address, uint32_t type, uint32_t *value) {
	if (address >= RAMSize)
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
		case EBUSINT:
			return EBUSERROR;
			break;

		case EBUSLONG:
			if (address == 0) {
				*value = RAMSLOTCOUNT;
				return EBUSSUCCESS;
			} else {
				address = (address/4)-1;

				if (address < RAMSLOTCOUNT) {
					*value = RAMSlotSizes[address];
					return EBUSSUCCESS;
				}
			}

			return EBUSERROR;
			break;
	}

	return EBUSSUCCESS;
}

int RAMInit(uint32_t memsize) {
	if (memsize > RAMMAXIMUM) {
		fprintf(stderr, "RAMInit: maximum is %d bytes (%d bytes given)\n", RAMMAXIMUM, memsize);
		return -1;
	}

	RAMSize = memsize;

	if (RAM) {
		memset(&RAMSlotSizes, 0, sizeof(RAMSlotSizes));
		free(RAM);
	}

	EBusBranches[0].Present = 1;
	EBusBranches[0].Write = RAMWrite;
	EBusBranches[0].Read = RAMRead;
	EBusBranches[0].Reset = 0;

	EBusBranches[1].Present = 1;
	EBusBranches[1].Write = RAMWriteExt;
	EBusBranches[1].Read = RAMReadExt;
	EBusBranches[1].Reset = 0;

	EBusBranches[2].Present = 1;
	EBusBranches[2].Write = RAMDescWrite;
	EBusBranches[2].Read = RAMDescRead;
	EBusBranches[2].Reset = 0;

	RAM = malloc(memsize);

	if (RAM == 0)
		return -1;

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