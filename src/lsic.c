#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ebus.h"
#include "lsic.h"

bool LSICInterruptPending = false;

int LSICWrite(int reg, uint32_t value) {
	return EBUSERROR;
}

int LSICRead(int reg, uint32_t *value) {
	return EBUSERROR;
}

void LSICReset() {
	LSICInterruptPending = false;
}