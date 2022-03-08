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

// assumes about 1ms per character transmission

#define TRANSMITBUFFERSIZE 16

enum SerialCommands {
	SERIALCMDDOINT = 3,
	SERIALCMDDONTINT = 4,
};

struct SerialPort {
	uint32_t DataValue;
	uint32_t DoInterrupts;
	uint32_t LastChar;
	unsigned char TransmitBuffer[TRANSMITBUFFERSIZE];
	int TransmitBufferIndex;
	int SendIndex;
	bool ReadBusy;
	bool WriteBusy;
};

struct SerialPort SerialPorts[2];

void SerialInterval(uint32_t dt) {
	for (int port = 0; port < 2; port++) {
		struct SerialPort *thisport = &SerialPorts[port];

		while (dt) {
			if (thisport->SendIndex < thisport->TransmitBufferIndex)
				putchar(thisport->TransmitBuffer[thisport->SendIndex++]);
			else
				break;

			if (thisport->SendIndex == thisport->TransmitBufferIndex) {
				thisport->SendIndex = 0;
				thisport->TransmitBufferIndex = 0;
				thisport->WriteBusy = false;

				if (thisport->DoInterrupts)
					LSICInterrupt(0x4+port);
			}

			dt--;
		}

		fflush(stdout);
	}
}

int SerialWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	struct SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	}

	switch(value) {
		case SERIALCMDDOINT:
			thisport->DoInterrupts = true;
			break;

		case SERIALCMDDONTINT:
			thisport->DoInterrupts = false;
			break;
	}

	return EBUSSUCCESS;
}

int SerialReadCMD(uint32_t port, uint32_t type, uint32_t *value) {
	struct SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	}

	*value = thisport->WriteBusy;

	return EBUSSUCCESS;
}

int SerialWriteData(uint32_t port, uint32_t type, uint32_t value) {
	struct SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	}

	if (thisport->TransmitBufferIndex == TRANSMITBUFFERSIZE) {
		return EBUSSUCCESS;
	}
	
	thisport->TransmitBuffer[thisport->TransmitBufferIndex++] = value;

	if (thisport->TransmitBufferIndex == TRANSMITBUFFERSIZE) {
		thisport->WriteBusy = true;
	}

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
			*value = 0xFF;
			break;

		case EBUSINT:
			*value = 0xFFFF;
			break;

		case EBUSLONG:
			*value = 0xFFFF;
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