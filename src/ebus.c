#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "ram256.h"
#include "pboard.h"
#include "kinnowfb.h"

int EBusInit(uint32_t memsize) {
	for (int i = 0; i < EBUSBRANCHES; i++) {
		EBusBranches[i].Present = 0;
	}

	if (RAMInit(memsize))
		return -1;

	if (PBoardInit())
		return -1;

	if (KinnowInit())
		return -1;

	return 0;
}

int EBusRead(uint32_t address, uint32_t type, uint32_t *value) {
	int branch = address >> 27;

	if (EBusBranches[branch].Present) {
		return EBusBranches[branch].Read(address&0x7FFFFFF, type, value);
	} else if (branch >= 24) {
		*value = 0;
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int EBusWrite(uint32_t address, uint32_t type, uint32_t value) {
	int branch = address >> 27;

	if (EBusBranches[branch].Present) {
		return EBusBranches[branch].Write(address&0x7FFFFFF, type, value);
	} else if (branch >= 24) {
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

void EBusReset() {
	for (int i = 0; i < EBUSBRANCHES; i++) {
		if (EBusBranches[i].Present && EBusBranches[i].Reset)
			EBusBranches[i].Reset();
	}
}