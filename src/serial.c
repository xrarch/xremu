#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "serial.h"

#include "screen.h"
#include "tty.h"

// assumes about 1ms per character transmission

#define TRANSMITBUFFERSIZE 16

enum SerialCommands {
	SERIALCMDDOINT = 3,
	SERIALCMDDONTINT = 4,
};

struct SerialPort {
	struct TTY *Tty;
	uint32_t DataValue;
	uint32_t DoInterrupts;
	uint32_t LastChar;
	uint8_t LastArrowKey;
	uint8_t ArrowKeyState;
	unsigned char TransmitBuffer[TRANSMITBUFFERSIZE];
	int TransmitBufferIndex;
	int SendIndex;
	bool WriteBusy;
	int Number;

#ifndef EMSCRIPTEN
	int RXFile;
	int TXFile;
#endif
};

struct SerialPort SerialPorts[2];

bool SerialAsynchronous = false;

#ifndef EMSCRIPTEN
// emcc dies compiling this code for reasons I am too lazy to discover. plus
// this feature is irrelevant for the web emulator, so just ifdef everything
// related to it out.

int SerialRXFile = 0;
int SerialTXFile = 0;

bool SerialSetRXFile(char *filename) {
	if (SerialRXFile)
		close(SerialRXFile);

	SerialRXFile = open(filename, O_RDWR | O_NONBLOCK);

	if (SerialRXFile == -1) {
		SerialRXFile = 0;
		fprintf(stderr, "couldn't open serialrx file '%s': %s\n", filename, strerror(errno));
		return false;
	}

	return true;
}

bool SerialSetTXFile(char *filename) {
	if (SerialTXFile)
		close(SerialTXFile);

	SerialTXFile = open(filename, O_RDWR | O_NONBLOCK);

	if (SerialTXFile == -1) {
		SerialTXFile = 0;
		fprintf(stderr, "couldn't open serialtx file '%s': %s\n", filename, strerror(errno));
		return false;
	}

	return true;
}

#endif

void SerialPutCharacter(struct SerialPort *port, char c) {
	TTYPutCharacter(port->Tty, c);

#ifndef EMSCRIPTEN
	if (port->TXFile != -1) {
		write(port->TXFile, &c, 1);
	}
#endif
}

void SerialInterval(uint32_t dt) {
	for (int port = 0; port < 2; port++) {
		struct SerialPort *thisport = &SerialPorts[port];

#ifndef EMSCRIPTEN
		if ((thisport->RXFile != -1) && (thisport->DoInterrupts)) {
			struct pollfd rxpoll = { 0 };

			rxpoll.fd = thisport->RXFile;
			rxpoll.events = POLLIN;

			if (poll(&rxpoll, 1, 0) > 0) {
				LSICInterrupt(0x4+port);
			}
		}
#endif

		if (!SerialAsynchronous)
			continue;

		for (int i = 0; i < dt; i++) {
			if (thisport->SendIndex < thisport->TransmitBufferIndex)
				SerialPutCharacter(thisport, thisport->TransmitBuffer[thisport->SendIndex++]);
			else
				break;

			if (thisport->SendIndex == thisport->TransmitBufferIndex) {
				thisport->SendIndex = 0;
				thisport->TransmitBufferIndex = 0;
				thisport->WriteBusy = false;

				if (thisport->DoInterrupts)
					LSICInterrupt(0x4+port);
			}
		}

		// fflush(stdout);
	}
}

int SerialWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	struct SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
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
	} else {
		return EBUSERROR;
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
	} else {
		return EBUSERROR;
	}

	if (!SerialAsynchronous) {
		SerialPutCharacter(thisport, value&0xFF);
		return EBUSSUCCESS;
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

int SerialReadData(uint32_t port, uint32_t length, uint32_t *value) {
	struct SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

#ifndef EMSCRIPTEN
	if (thisport->RXFile != -1) {
		uint8_t nextchar;

		if (read(thisport->RXFile, &nextchar, 1) > 0) {
			*value = nextchar;
			return EBUSSUCCESS;
		}
	}
#endif

	if (thisport->LastArrowKey) {
		if (thisport->ArrowKeyState == 0) {
			*value = 0x1B;
		} else if (thisport->ArrowKeyState == 1) {
			*value = '[';
		} else if (thisport->ArrowKeyState == 2) {
			*value = thisport->LastArrowKey;
			thisport->LastArrowKey = 0;
		}

		thisport->ArrowKeyState++;
	} else {
		*value = thisport->LastChar;
		thisport->LastChar = 0xFFFF;
	}

	return EBUSSUCCESS;
}

void SerialInput(struct TTY *tty, uint16_t c) {
	struct SerialPort *port = (struct SerialPort *)(tty->Context);

	if (c&0x8000) {
		// hacky way to do arrow keys
		port->LastArrowKey = c&0xFF;
		port->ArrowKeyState = 0;
	} else {
		port->LastChar = c;
	}

	if (port->DoInterrupts)
		LSICInterrupt(0x4+port->Number);
}

char *SerialNames[] = {
	"ttyS0",
	"ttyS1"
};

int SerialInit(int num) {
	int citronoffset = num*2;

	CitronPorts[0x10+citronoffset].Present = 1;
	CitronPorts[0x10+citronoffset].WritePort = SerialWriteCMD;
	CitronPorts[0x10+citronoffset].ReadPort = SerialReadCMD;

	CitronPorts[0x11+citronoffset].Present = 1;
	CitronPorts[0x11+citronoffset].WritePort = SerialWriteData;
	CitronPorts[0x11+citronoffset].ReadPort = SerialReadData;

	SerialPorts[num].LastChar = 0xFFFF;
	SerialPorts[num].LastArrowKey = 0;
	SerialPorts[num].ArrowKeyState = 0;
	SerialPorts[num].Number = num;

#ifndef EMSCRIPTEN
	if ((num == 1) && SerialTXFile) {
		SerialPorts[num].TXFile = SerialTXFile;
	} else {
		SerialPorts[num].TXFile = -1;
	}

	if ((num == 1) && SerialRXFile) {
		SerialPorts[num].RXFile = SerialRXFile;
	} else {
		SerialPorts[num].RXFile = -1;
	}
#endif

	SerialPorts[num].Tty = TTYCreate(80, 24, SerialNames[num], SerialInput);
	SerialPorts[num].Tty->Context = &SerialPorts[num];

	return 0;
}

void SerialReset() {
	SerialPorts[0].DoInterrupts = false;
	SerialPorts[1].DoInterrupts = false;
}