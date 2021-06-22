#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"
#include "pboard.h"

uint32_t PBoardRegisters[PBOARDREGISTERS];

int PBoardWrite(uint32_t address, uint32_t type, uint32_t value) {
	if (address < 0x400) {
		// citron

	} else if (address >= 0x7FE0000) {
		// bootrom

	} else if ((address >= 0x1000) && (address <= 0x11000)) {
		// nvram

	} else if ((address >= 0x800) && (address <= 0x880)) {
		// pboard registers

	} else if ((address >= 0x30000) && (address <= 0x30100)) {
		// LSIC registers

	} else if (address == 0x800000) {
		// reset

	}

	return EBUSERROR;
}

int PBoardRead(uint32_t address, uint32_t type, uint32_t *value) {
	return EBUSERROR;
}

int PBoardInit() {
	EBusBranches[31].Present = 1;
	EBusBranches[31].Write = PBoardWrite;
	EBusBranches[31].Read = PBoardRead;

	PBoardRegisters[0] = 0x00030001; // pboard version

	return 0;
}