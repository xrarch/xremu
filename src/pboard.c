#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "serial.h"
#include "amtsu.h"
#include "dks.h"
#include "rtc.h"

uint32_t PBoardRegisters[PBOARDREGISTERS];

#define NVRAMSIZE (64 * 1024)

uint8_t NVRAM[NVRAMSIZE];

#define ROMSIZE (128 * 1024)

uint8_t BootROM[ROMSIZE];

struct CitronPort CitronPorts[CITRONPORTS];

bool NVRAMDirty = false;

int PBoardWrite(uint32_t address, void *src, uint32_t length) {
	if (address < 0x400) {
		// citron

		uint32_t port = address/4;

		if (CitronPorts[port].Present)
			return CitronPorts[port].WritePort(port, length, *(uint32_t*)src);
		else {
			return EBUSERROR;
		}
	} else if (address >= 0x7FE0000) {
		// bootrom

		return EBUSSUCCESS;
	} else if ((address >= 0x1000) && (address < 0x11000)) {
		// nvram

		address -= 0x1000;

		NVRAMDirty = true;

		if (address+length > NVRAMSIZE)
			return false;

		memcpy(&NVRAM[address], src, length);

		return EBUSSUCCESS;
	} else if ((address >= 0x800) && (address < 0x880)) {
		// pboard registers

		address -= 0x800;

		if (length == 4) {
			if (address != 0)
				PBoardRegisters[address/4] = *(uint32_t*)src;

			return EBUSSUCCESS;
		}
	} else if ((address >= 0x20000) && (address < 0x21000)) {
		// DKS controller buffer

		address -= 0x20000;

		if (address+length > 0x1000)
			return false;

		memcpy(&DKSBlockBuffer[address], src, length);

		return EBUSSUCCESS;
	} else if ((address >= 0x30000) && (address < 0x30100)) {
		// LSIC registers

		address -= 0x30000;

		if (length == 4) {
			return LSICWrite(address/4, *(uint32_t*)src);
		}
	} else if (address == 0x800000) {
		// reset

		if ((length == 4) && (*(uint32_t*)src == RESETMAGIC)) {
			EBusReset();

			return EBUSSUCCESS;
		}
	}

	return EBUSERROR;
}

int PBoardRead(uint32_t address, void *dest, uint32_t length) {
	if (address < 0x400) {
		// citron

		uint32_t port = address/4;

		if (CitronPorts[port].Present)
			return CitronPorts[port].ReadPort(port, length, dest);
		else {
			return EBUSERROR;
		}
	} else if (address >= 0x7FE0000) {
		// bootrom

		address -= 0x7FE0000;

		if (address+length > ROMSIZE)
			return false;

		memcpy(dest, &BootROM[address], length);

		return EBUSSUCCESS;
	} else if ((address >= 0x1000) && (address < 0x11000)) {
		// nvram

		address -= 0x1000;

		if (address+length > NVRAMSIZE)
			return false;

		memcpy(dest, &NVRAM[address], length);

		return EBUSSUCCESS;
	} else if ((address >= 0x800) && (address < 0x880)) {
		// pboard registers

		address -= 0x800;

		if (length == 4) {
			*(uint32_t*)dest = PBoardRegisters[address/4];

			return EBUSSUCCESS;
		}
	} else if ((address >= 0x20000) && (address < 0x21000)) {
		// DKS controller buffer

		address -= 0x20000;

		if (address+length > 0x1000)
			return false;

		memcpy(dest, &DKSBlockBuffer[address], length);

		return EBUSSUCCESS;
	} else if ((address >= 0x30000) && (address < 0x30100)) {
		// LSIC registers

		address -= 0x30000;

		if (length == 4) {
			return LSICRead(address/4, dest);
		}
	}

	return EBUSERROR;
}

void PBoardReset() {
	RTCReset();
	SerialReset();
	DKSReset();
	AmtsuReset();
	LSICReset();
}

FILE *nvramfile;

void NVRAMSave() {
	if (NVRAMDirty && nvramfile) {
		// printf("saving nvram...\n");

		fseek(nvramfile, 0, SEEK_SET);
		fwrite(&NVRAM, NVRAMSIZE, 1, nvramfile);

		NVRAMDirty = false;
	}
}

bool ROMLoadFile(char *romname) {
	FILE *romfile;

	romfile = fopen(romname, "r");

	if (!romfile) {
		fprintf(stderr, "couldn't open boot ROM file '%s'\n", romname);
		return false;
	}

	fread(&BootROM, ROMSIZE, 1, romfile);

	fclose(romfile);

	return true;
}

bool NVRAMLoadFile(char *nvramname) {
	nvramfile = fopen(nvramname, "a+");

	if (!nvramfile) {
		fprintf(stderr, "couldn't open NVRAM file '%s'\n", nvramname);
		return false;
	}

	fclose(nvramfile);

	nvramfile = fopen(nvramname, "r+");

	if (!nvramfile) {
		fprintf(stderr, "couldn't open NVRAM file '%s'\n", nvramname);
		return false;
	}

	fseek(nvramfile, 0, SEEK_SET);

	fread(&NVRAM, NVRAMSIZE, 1, nvramfile);

	return true;
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
	RTCInit();

	AmtsuInit();

	return 0;
}