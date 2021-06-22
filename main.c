#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"

int main(int argc, char *argv[]) {
	if (EBusInit(4 * 1024 * 1024)) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

	int success = 0;

	success = EBusWrite(0xFFFE0000, EBUSLONG, 0xAABBCCDD);

	return 0;
}