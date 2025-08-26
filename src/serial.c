#include <SDL.h>
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
#include <unistd.h>

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
	XrSchedulable Schedulable;
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
	int Enqueued;

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

void SerialInterval(uint32_t dt) {
	for (int port = 0; port < 2; port++) {
		SerialPort *thisport = &SerialPorts[port];

#ifndef EMSCRIPTEN
		XrLockMutex(&thisport->Tty->Mutex);

		if ((thisport->RXFile != -1) && (thisport->DoInterrupts)) {
			struct pollfd rxpoll = { 0 };

			rxpoll.fd = thisport->RXFile;
			rxpoll.events = POLLIN;

			if (poll(&rxpoll, 1, 0) > 0) {
				LsicInterrupt(0x4 + port);
			}
		}

		XrUnlockMutex(&thisport->Tty->Mutex);
#endif
	}
}

#define CHARS_PER_SCHEDULE 4

void SerialSchedule(XrSchedulable *schedulable) {
	SerialPort *port = schedulable->Context;

	int timeslice = schedulable->Timeslice;

	XrLockMutex(&port->Tty->Mutex);

	if (port->Cost) {
		if (port->Cost < timeslice) {
			timeslice -= port->Cost;
			port->Cost = 0;
		} else {
			port->Cost -= timeslice;
			timeslice = 0;
		}

		goto exit;
	}

	int pending = port->TransmitBufferIndex - port->SendIndex;

	if (pending > schedulable->Timeslice) {
		pending = schedulable->Timeslice;
	}

	if (pending > CHARS_PER_SCHEDULE) {
		pending = CHARS_PER_SCHEDULE;
	}

	timeslice -= pending;

	for (int i = 0; i < pending; i++) {
		if (port->SendIndex < port->TransmitBufferIndex) {
			SerialPutCharacter(port, port->TransmitBuffer[port->SendIndex++]);
		} else {
			break;
		}

		if (port->SendIndex == port->TransmitBufferIndex) {
			port->SendIndex = 0;
			port->TransmitBufferIndex = 0;
			port->WriteBusy = false;

			if (port->DoInterrupts) {
				LsicInterrupt(0x4 + port->Number);
			}
		}
	}

exit:

	if (!timeslice && (port->Cost || (port->TransmitBufferIndex - port->SendIndex))) {
		XrScheduleWorkForNextFrame(schedulable, 1);
	} else if (timeslice && (port->Cost || (port->TransmitBufferIndex - port->SendIndex))) {
		XrScheduleWork(schedulable);
	} else {
		port->Enqueued = 0;
	}

	XrUnlockMutex(&port->Tty->Mutex);

	schedulable->Timeslice = timeslice;
}

void SerialStartTimeslice(XrSchedulable *schedulable, int dt) {
	schedulable->Timeslice = dt;
}

int SerialWriteCMD(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

	XrLockMutex(&thisport->Tty->Mutex);

	switch(value) {
		case SERIALCMDDOINT:
			thisport->DoInterrupts = true;
			break;

		case SERIALCMDDONTINT:
			thisport->DoInterrupts = false;
			break;
	}

	XrUnlockMutex(&thisport->Tty->Mutex);

	return EBUSSUCCESS;
}

int SerialReadCMD(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	SerialPort *thisport;

	if (port == 0x10) {
		thisport = &SerialPorts[0];
	} else if (port == 0x12) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

	XrLockMutex(&thisport->Tty->Mutex);

	*value = thisport->WriteBusy;

	if (thisport->WriteBusy) {
		XrDecrementProgress(proc, thisport->DoInterrupts);
	}

	XrUnlockMutex(&thisport->Tty->Mutex);

	return EBUSSUCCESS;
}

int SerialWriteData(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	SerialPort *thisport;

	int status;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

	XrLockMutex(&thisport->Tty->Mutex);

	if ((!SerialAsynchronous || (thisport->Cost < ACCUMULATED_COST)) && !thisport->TransmitBufferIndex) {
		SerialPutCharacter(thisport, value&0xFF);
		thisport->Cost++;
		
		status = EBUSSUCCESS;
		goto exit;
	}

	if (thisport->TransmitBufferIndex == TRANSMIT_BUFFER_SIZE) {
		status = EBUSSUCCESS;
		goto exit;
	}
	
	thisport->TransmitBuffer[thisport->TransmitBufferIndex++] = value;

	if (thisport->TransmitBufferIndex == TRANSMIT_BUFFER_SIZE) {
		thisport->WriteBusy = true;
	}

	if (!thisport->Enqueued) {
		thisport->Enqueued = 1;
		XrScheduleWorkBorrow(&((XrProcessor *)proc)->Schedulable, &thisport->Schedulable);
	}

exit:

	XrUnlockMutex(&thisport->Tty->Mutex);
	return status;
}

int SerialReadData(uint32_t port, uint32_t length, uint32_t *value, void *proc) {
	SerialPort *thisport;

	if (port == 0x11) {
		thisport = &SerialPorts[0];
	} else if (port == 0x13) {
		thisport = &SerialPorts[1];
	} else {
		return EBUSERROR;
	}

	int status;

	XrLockMutex(&thisport->Tty->Mutex);

#ifndef EMSCRIPTEN
	if (thisport->RXFile != -1) {
		uint8_t nextchar;

		if (read(thisport->RXFile, &nextchar, 1) > 0) {
			*value = nextchar;

			status = EBUSSUCCESS;
			goto exit;
		}
	}
#endif

	if ((thisport->ReceiveBufferIndex - thisport->ReceiveIndex) == 0) {
		XrDecrementProgress(proc, thisport->DoInterrupts);
		*value = 0xFFFF;

		status = EBUSSUCCESS;
		goto exit;
	}

	*value = thisport->ReceiveBuffer[thisport->ReceiveIndex++ % RECEIVE_BUFFER_SIZE];
	thisport->ReceiveRemaining++;

exit:

	XrUnlockMutex(&thisport->Tty->Mutex);
	return status;
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

	SerialPort *port = &SerialPorts[num];

	CitronPorts[0x10+citronoffset].Present = 1;
	CitronPorts[0x10+citronoffset].WritePort = SerialWriteCMD;
	CitronPorts[0x10+citronoffset].ReadPort = SerialReadCMD;

	CitronPorts[0x11+citronoffset].Present = 1;
	CitronPorts[0x11+citronoffset].WritePort = SerialWriteData;
	CitronPorts[0x11+citronoffset].ReadPort = SerialReadData;

	port->ReceiveRemaining = RECEIVE_BUFFER_SIZE;
	port->Number = num;
	port->Enqueued = 0;

#ifndef EMSCRIPTEN
	if ((num == 1) && SerialTXFile) {
		port->TXFile = SerialTXFile;
	} else {
		port->TXFile = -1;
	}

	if ((num == 1) && SerialRXFile) {
		port->RXFile = SerialRXFile;
	} else {
		port->RXFile = -1;
	}
#endif

	if (TTY132ColumnMode) {
		port->Tty = TTYCreate(132, 24, SerialNames[num], SerialInput);
	} else {
		port->Tty = TTYCreate(80, 24, SerialNames[num], SerialInput);
	}

	port->Tty->Context = port;

	if (SerialAsynchronous) {
		XrInitializeSchedulable(&port->Schedulable, &SerialSchedule, &SerialStartTimeslice, port);
	}

	return 0;
}

void SerialReset() {
	SerialPorts[0].DoInterrupts = false;
	SerialPorts[1].DoInterrupts = false;
}
