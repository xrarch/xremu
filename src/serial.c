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

#include "xr.h"

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "serial.h"

#include "screen.h"
#include "tty.h"

#define min(a,b) ((a < b) ? a : b)

// assumes about 1ms per character transmission

#define TRANSMIT_BUFFER_SIZE 16
#define RECEIVE_BUFFER_SIZE 32

// Accumulated characters before paying the time cost for all of them at once.

#define ACCUMULATED_COST 50

enum SerialCommands {
	SERIALCMDDOINT = 3,
	SERIALCMDDONTINT = 4,
};

typedef struct _SerialPort {
	struct TTY *Tty;
	uint32_t DoInterrupts;

	unsigned char TransmitBuffer[TRANSMIT_BUFFER_SIZE];
	int TransmitBufferIndex;
	int SendIndex;
	int Cost;
	bool WriteBusy;

	unsigned char ReceiveBuffer[RECEIVE_BUFFER_SIZE];
	int ReceiveBufferIndex;
	int ReceiveIndex;
	int ReceiveRemaining;

	int Number;

#ifndef EMSCRIPTEN
	int RXFile;
	int TXFile;
#endif
} SerialPort;

SerialPort SerialPorts[2];

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

void SerialPutCharacter(SerialPort *port, char c) {
	TTYPutCharacter(port->Tty, c);

#ifndef EMSCRIPTEN
	if (port->TXFile != -1) {
		write(port->TXFile, &c, 1);
	}
#endif
}

#define SERIAL_QUANTUM_MS TRANSMIT_BUFFER_SIZE

uint8_t SerialTimerEnqueued = 0;

uint32_t SerialCallback(uint32_t interval, void *param) {
	SerialPort *thisport = param;

	LockIoMutex();

	SerialTimerEnqueued = 0;

#ifndef EMSCRIPTEN
	interval = thisport->TransmitBufferIndex - thisport->SendIndex;
#endif

	thisport->Cost = 0;

	for (int i = 0; i < interval; i++) {
		if (thisport->SendIndex < thisport->TransmitBufferIndex) {
			SerialPutCharacter(thisport, thisport->TransmitBuffer[thisport->SendIndex++]);
		} else {
			break;
		}

		if (thisport->SendIndex == thisport->TransmitBufferIndex) {
			thisport->SendIndex = 0;
			thisport->TransmitBufferIndex = 0;
			thisport->WriteBusy = false;

			if (thisport->DoInterrupts) {
				LsicInterrupt(0x4 + thisport->Number);
			}
		}
	}

	UnlockIoMutex();

	return 0;
}

void SerialInterval(uint32_t dt) {
	for (int port = 0; port < 2; port++) {
		SerialPort *thisport = &SerialPorts[port];

#ifndef EMSCRIPTEN
		if ((thisport->RXFile != -1) && (thisport->DoInterrupts)) {
			struct pollfd rxpoll = { 0 };

			rxpoll.fd = thisport->RXFile;
			rxpoll.events = POLLIN;

			if (poll(&rxpoll, 1, 0) > 0) {
				LockIoMutex();
				LsicInterrupt(0x4 + port);
				UnlockIoMutex();
			}
		}
#endif

#ifdef EMSCRIPTEN
		if (!SerialAsynchronous)
			continue;

		SerialCallback(dt, thisport);
#endif
	}
}

int SerialWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	SerialPort *thisport;

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
	SerialPort *thisport;

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
	SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

	thisport->Cost += 1;

	if ((!SerialAsynchronous || thisport->Cost < ACCUMULATED_COST) && !thisport->TransmitBufferIndex) {
		SerialPutCharacter(thisport, value&0xFF);
		return EBUSSUCCESS;
	}

	if (thisport->TransmitBufferIndex == TRANSMIT_BUFFER_SIZE) {
		return EBUSSUCCESS;
	}
	
	thisport->TransmitBuffer[thisport->TransmitBufferIndex++] = value;

	if (thisport->TransmitBufferIndex == TRANSMIT_BUFFER_SIZE) {
		thisport->WriteBusy = true;
	}

#ifndef EMSCRIPTEN
	if (!SerialTimerEnqueued) {
		EnqueueCallback(ACCUMULATED_COST, &SerialCallback, thisport);
		
		SerialTimerEnqueued = 1;
	}
#endif

	return EBUSSUCCESS;
}

int SerialReadData(uint32_t port, uint32_t length, uint32_t *value) {
	SerialPort *thisport;

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

	if ((thisport->ReceiveBufferIndex - thisport->ReceiveIndex) == 0) {
		XrIoMutexProcessor->Progress--;
		*value = 0xFFFF;

		return EBUSSUCCESS;
	}

	*value = thisport->ReceiveBuffer[thisport->ReceiveIndex++ % RECEIVE_BUFFER_SIZE];
	thisport->ReceiveRemaining++;

	return EBUSSUCCESS;
}

void SerialInput(struct TTY *tty, uint16_t c) {
	SerialPort *port = (SerialPort *)(tty->Context);

	if (!port->ReceiveRemaining) {
		return;
	}

	port->ReceiveBuffer[port->ReceiveBufferIndex++ % RECEIVE_BUFFER_SIZE] = c;
	port->ReceiveRemaining--;

	if (port->DoInterrupts) {
		LsicInterrupt(0x4 + port->Number);
	}
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

	SerialPorts[num].ReceiveRemaining = RECEIVE_BUFFER_SIZE;

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

	if (TTY132ColumnMode) {
		SerialPorts[num].Tty = TTYCreate(132, 24, SerialNames[num], SerialInput);
	} else {
		SerialPorts[num].Tty = TTYCreate(80, 24, SerialNames[num], SerialInput);
	}

	SerialPorts[num].Tty->Context = &SerialPorts[num];

	return 0;
}

void SerialReset() {
	SerialPorts[0].DoInterrupts = false;
	SerialPorts[1].DoInterrupts = false;
}