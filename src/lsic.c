#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ebus.h"
#include "lsic.h"

bool LSICInterruptPending = false;

uint32_t LSICRegisters[5];

void LSICInterrupt(int intsrc) {
	if ((intsrc >= 64) || (intsrc == 0)) {
		fprintf(stderr, "bad interrupt source\n");
		abort();
	}

	int srcbmp = intsrc/32;
	int srcbmpoff = intsrc&31;
	int ri = srcbmp+2;

	LSICRegisters[ri] |= (1 << srcbmpoff);

	int rm = srcbmp;

	if (((LSICRegisters[rm]>>srcbmpoff)&1) == 0)
		LSICInterruptPending = true;
}

int LSICWrite(int reg, uint32_t value) {
	switch(reg) {
		case 4:
			// complete
			if (value >= 64)
				return EBUSERROR;

			int rg = value/32+2;

			LSICRegisters[rg] &= ~(1 << (value&31));

			LSICInterruptPending = ((~LSICRegisters[0]) & LSICRegisters[2]) || ((~LSICRegisters[1]) & LSICRegisters[3]);

			return EBUSSUCCESS;

		case 0:
		case 1:
		case 2:
		case 3:
			// masks and interrupt sources
			LSICRegisters[reg] = value;
			
			LSICInterruptPending = ((~LSICRegisters[0]) & LSICRegisters[2]) || ((~LSICRegisters[1]) & LSICRegisters[3]);

			return EBUSSUCCESS;

		default:
			return EBUSERROR;
	}

	return EBUSERROR;
}

int LSICRead(int reg, uint32_t *value) {
	switch(reg) {
		case 4:
			// claim

			for (int i = 1; i < 64; i++) {
				int bmp = i/32;
				int bmpoff = i&31;

				// todo finish im sleepy
			}

			return EBUSSUCCESS;

		case 0:
		case 1:
		case 2:
		case 3:
			*value = LSICRegisters[reg];
			return EBUSSUCCESS;

		default:
			return EBUSERROR;
	}

	return EBUSERROR;
}

void LSICReset() {
	LSICRegisters[0] = 0;
	LSICRegisters[1] = 0;
	LSICRegisters[2] = 0;
	LSICRegisters[3] = 0;
	LSICRegisters[4] = 0;

	LSICInterruptPending = false;
}