#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "ram256.h"
#include "pboard.h"
#include "kinnowfb.h"

struct EBusBranch EBusBranches[EBUSBRANCHES];

extern bool Headless;

int EmptyRead(uint32_t address, void *dest, uint32_t length, void *proc) {
	memset(dest, 0, length);

	return EBUSSUCCESS;
}

int EmptyWrite(uint32_t address, void *dest, uint32_t length, void *proc) {
	return EBUSSUCCESS;
}

int EmptyMemRead(uint32_t address, void *dest, uint32_t length, void *proc) {
	return EBUSERROR;
}

int EmptyMemWrite(uint32_t address, void *dest, uint32_t length, void *proc) {
	return EBUSERROR;
}

int EBusInit(uint32_t memsize) {
	for (int i = 0; i < 24; i++) {
		EBusBranches[i].Present = 0;
		EBusBranches[i].Read = EmptyMemRead;
		EBusBranches[i].Write = EmptyMemWrite;
		EBusBranches[i].Reset = 0;
	}

	for (int i = 24; i < EBUSBRANCHES; i++) {
		EBusBranches[i].Present = 0;
		EBusBranches[i].Read = EmptyRead;
		EBusBranches[i].Write = EmptyWrite;
		EBusBranches[i].Reset = 0;
	}

	if (RAMInit(memsize))
		return -1;

	if (PBoardInit())
		return -1;

	if (!Headless) {
		if (KinnowInit())
			return -1;
	}

	return 0;
}

void EBusReset() {
	for (int i = 0; i < EBUSBRANCHES; i++) {
		if (EBusBranches[i].Present && EBusBranches[i].Reset)
			EBusBranches[i].Reset();
	}
}