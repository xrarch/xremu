#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "limn2500.h"

#include "ebus.h"

uint32_t Reg[32];

uint32_t ControlReg[16];

uint32_t PC = 0;

int CurrentException;

void Limn2500Exception(int exception) {
	if (CurrentException) {
		fprintf(stderr, "double exception, shouldnt ever happen");
		abort();
	}

	CurrentException = exception;
}