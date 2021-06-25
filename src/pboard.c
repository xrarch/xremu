#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "serial.h"
#include "amtsu.h"
#include "dks.h"

uint32_t PBoardRegisters[PBOARDREGISTERS];

bool NVRAMDirty = false;

int PBoardWrite(uint32_t address, uint32_t type, uint32_t value) {
	if (address < 0x400) {
		// citron

		uint32_t port = address/4;

		if (CitronPorts[port].Present)
			return CitronPorts[port].WritePort(port, type, value);
		else
			return EBUSSUCCESS;
	} else if (address >= 0x7FE0000) {
		// bootrom

		return EBUSSUCCESS;
	} else if ((address >= 0x1000) && (address < 0x11000)) {
		// nvram

		address -= 0x1000;

		NVRAMDirty = true;

		switch(type) {
			case EBUSBYTE:
				((uint8_t*)NVRAM)[address] = value;
				break;

			case EBUSINT:
				((uint16_t*)NVRAM)[address/2] = value;
				break;

			case EBUSLONG:
				NVRAM[address/4] = value;
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x800) && (address < 0x880)) {
		// pboard registers

		address -= 0x800;

		if (type == EBUSLONG) {
			if (address != 0)
				PBoardRegisters[address/4] = value;

			return EBUSSUCCESS;
		}
	} else if ((address >= 0x20000) && (address < 0x21000)) {
		address -= 0x20000;

		switch(type) {
			case EBUSBYTE:
				((uint8_t*)DKSBlockBuffer)[address] = value;
				break;

			case EBUSINT:
				((uint16_t*)DKSBlockBuffer)[address/2] = value;
				break;

			case EBUSLONG:
				DKSBlockBuffer[address/4] = value;
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x30000) && (address < 0x30100)) {
		// LSIC registers

		address -= 0x30000;

		if (type == EBUSLONG) {
			return LSICWrite(address/4, value);
		}
	} else if (address == 0x800000) {
		// reset

		if (value == RESETMAGIC) {
			EBusReset();

			return EBUSSUCCESS;
		}
	}

	return EBUSERROR;
}

int PBoardRead(uint32_t address, uint32_t type, uint32_t *value) {
	if (address < 0x400) {
		// citron

		uint32_t port = address/4;

		if (CitronPorts[port].Present)
			return CitronPorts[port].ReadPort(port, type, value);
		else {
			// XXX this is incorrect behavior, should bus error here
			*value = 0;
			return EBUSSUCCESS;
		}
	} else if (address >= 0x7FE0000) {
		// bootrom

		address -= 0x7FE0000;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)BootROM)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)BootROM)[address/2];
				break;

			case EBUSLONG:
				*value = BootROM[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x1000) && (address < 0x11000)) {
		// nvram

		address -= 0x1000;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)NVRAM)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)NVRAM)[address/2];
				break;

			case EBUSLONG:
				*value = NVRAM[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x800) && (address < 0x880)) {
		// pboard registers

		address -= 0x800;

		if (type == EBUSLONG) {
			*value = PBoardRegisters[address/4];

			return EBUSSUCCESS;
		}
	} else if ((address >= 0x20000) && (address < 0x21000)) {
		address -= 0x20000;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)DKSBlockBuffer)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)DKSBlockBuffer)[address/2];
				break;

			case EBUSLONG:
				*value = DKSBlockBuffer[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x30000) && (address < 0x30100)) {
		// LSIC registers

		address -= 0x30000;

		if (type == EBUSLONG) {
			return LSICRead(address/4, value);
		}
	}

	return EBUSERROR;
}

void PBoardReset() {
	// RTCReset();
	SerialReset();
	DKSReset();
	AmtsuReset();
	LSICReset();
}

FILE *nvramfile;

void NVRAMSave() {
	if (NVRAMDirty) {
		// printf("saving nvram...\n");

		fseek(nvramfile, 0, SEEK_SET);
		fwrite(&NVRAM, NVRAMSIZE, 1, nvramfile);

		NVRAMDirty = false;
	}
}

int PBoardInit() {
	EBusBranches[31].Present = 1;
	EBusBranches[31].Write = PBoardWrite;
	EBusBranches[31].Read = PBoardRead;
	EBusBranches[31].Reset = PBoardReset;

	PBoardRegisters[0] = 0x00030001; // pboard version

	for (int i = 0; i < CITRONPORTS; i++)
		CitronPorts[i].Present = 0;

	SerialInit(0);
	SerialInit(1);
	DKSInit();

	FILE *romfile;

	romfile = fopen("boot.bin", "r");

	if (!romfile) {
		fprintf(stderr, "couldn't open boot ROM file 'boot.bin'\n");
		return -1;
	}

	fread(&BootROM, ROMSIZE, 1, romfile);

	fclose(romfile);

	nvramfile = fopen("nvram", "a+");

	if (!nvramfile) {
		fprintf(stderr, "couldn't open NVRAM file 'nvram'\n");
		return -1;
	}

	FILE *oldhandle = nvramfile;

	nvramfile = fopen("nvram", "r+");

	fclose(oldhandle);

	fseek(nvramfile, 0, SEEK_SET);

	fread(&NVRAM, NVRAMSIZE, 1, nvramfile);

	AmtsuInit();

	return 0;
}