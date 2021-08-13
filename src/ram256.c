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

uint32_t RAMSize;

void RAMDump() {
	FILE *ramdump;

	ramdump = fopen("ramdump.bin", "wb");

	if (!ramdump) {
		return;
	}

	fwrite(RAM, RAMSize, 1, ramdump);

	fclose(ramdump);
}

int RAMWrite(uint32_t address, uint32_t type, uint32_t value) {
	if (address >= RAMSize)
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			((uint8_t*)RAM)[address] = value;
			break;

		case EBUSINT:
			((uint16_t*)RAM)[address/2] = value;
			break;

		case EBUSLONG:
			RAM[address/4] = value;
			break;
	}

	return EBUSSUCCESS;
}

int RAMRead(uint32_t address, uint32_t type, uint32_t *value) {
	if (address >= RAMSize)
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			*value = ((uint8_t*)RAM)[address];
			break;

		case EBUSINT:
			*value = ((uint16_t*)RAM)[address/2];
			break;

		case EBUSLONG:
			*value = RAM[address/4];
			break;
	}

	return EBUSSUCCESS;
}

int RAMWriteExt(uint32_t address, uint32_t type, uint32_t value) {
	address += EBUSBRANCHSIZE;

	if (address >= RAMSize)
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			((uint8_t*)RAM)[address] = value;
			break;

		case EBUSINT:
			((uint16_t*)RAM)[address/2] = value;
			break;

		case EBUSLONG:
			RAM[address/4] = value;
			break;
	}

	return EBUSSUCCESS;
}

int RAMReadExt(uint32_t address, uint32_t type, uint32_t *value) {
	address += EBUSBRANCHSIZE;

	if (address >= RAMSize)
		return EBUSERROR;

	switch(type) {
		case EBUSBYTE:
			*value = ((uint8_t*)RAM)[address];
			break;

		case EBUSINT:
			*value = ((uint16_t*)RAM)[address/2];
			break;

		case EBUSLONG:
			*value = RAM[address/4];
			break;
	}

	return EBUSSUCCESS;
}

uint32_t RAMSlotSizes[RAMSLOTCOUNT];

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

	if (RAM)
		free(RAM);

	RAM = malloc(memsize);

	if (RAM == 0)
		return -1;

	EBusBranches[0].Present = 1;
	EBusBranches[0].Write = RAMWrite;
	EBusBranches[0].Read = RAMRead;
	EBusBranches[0].Reset = 0;

	if (memsize > EBUSBRANCHSIZE) {
		EBusBranches[1].Present = 1;
		EBusBranches[1].Write = RAMWriteExt;
		EBusBranches[1].Read = RAMReadExt;
		EBusBranches[1].Reset = 0;
	}

	EBusBranches[2].Present = 1;
	EBusBranches[2].Write = RAMDescWrite;
	EBusBranches[2].Read = RAMDescRead;
	EBusBranches[2].Reset = 0;

	int fullslots = memsize / RAMSLOTSIZE;
	int count = 0;

	for (; count < fullslots; count++) {
		RAMSlotSizes[count] = RAMSLOTSIZE;
	}

	int leftover = memsize - (fullslots * RAMSLOTSIZE);

	if (leftover)
		RAMSlotSizes[count] = leftover;

	return 0;
}