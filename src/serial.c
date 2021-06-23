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
#include "lsic.h"
#include "serial.h"

enum SerialCommands {
	SERIALCMDWRITECHAR = 1,
	SERIALCMDREADCHAR = 2,
	SERIALCMDDOINT = 3,
	SERIALCMDDONTINT = 4,
};

struct SerialPort {
	uint32_t DataValue;
	uint32_t DoInterrupts;
	uint32_t LastChar;
};

struct SerialPort SerialPorts[2];

int SerialWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	struct SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	}

	switch(value) {
		case SERIALCMDWRITECHAR:
			putchar(thisport->DataValue);
			return EBUSSUCCESS;

		case SERIALCMDREADCHAR:
			thisport->DataValue = thisport->LastChar;
			thisport->LastChar = 0xFFFF;
			return EBUSSUCCESS;

		case SERIALCMDDOINT:
			thisport->DoInterrupts = true;
			return EBUSSUCCESS;

		case SERIALCMDDONTINT:
			thisport->DoInterrupts = false;
			return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int SerialReadCMD(uint32_t port, uint32_t type, uint32_t *value) {
	*value = 0;

	return EBUSSUCCESS;
}

int SerialWriteData(uint32_t port, uint32_t type, uint32_t value) {
	struct SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	}

	thisport->DataValue = value;

	return EBUSSUCCESS;
}

int SerialReadData(uint32_t port, uint32_t type, uint32_t *value) {
	struct SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	}

	switch(type) {
		case EBUSBYTE:
			*value = thisport->DataValue&0xFF;
			break;

		case EBUSINT:
			*value = thisport->DataValue&0xFFFF;
			break;

		case EBUSLONG:
			*value = thisport->DataValue;
			break;
	}

	return EBUSSUCCESS;
}

int SerialInit(int num) {
	int citronoffset = num*2;

	CitronPorts[0x10+citronoffset].Present = 1;
	CitronPorts[0x10+citronoffset].WritePort = SerialWriteCMD;
	CitronPorts[0x10+citronoffset].ReadPort = SerialReadCMD;

	CitronPorts[0x11+citronoffset].Present = 1;
	CitronPorts[0x11+citronoffset].WritePort = SerialWriteData;
	CitronPorts[0x11+citronoffset].ReadPort = SerialReadData;

	SerialPorts[0].LastChar = 0xFFFF;
	SerialPorts[1].LastChar = 0xFFFF;

	return 0;
}

void SerialReset() {
	SerialPorts[0].DoInterrupts = false;
	SerialPorts[1].DoInterrupts = false;
}