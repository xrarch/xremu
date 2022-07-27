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

int EBusInit(uint32_t memsize) {
	for (int i = 0; i < EBUSBRANCHES; i++) {
		EBusBranches[i].Present = 0;
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