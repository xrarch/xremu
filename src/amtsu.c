#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "amtsu.h"
#include "keybd.h"
#include "mouse.h"
#include "fastmutex.h"

struct AmtsuDevice AmtsuDevices[AMTSUDEVICES];

int CurrentDevice = AMTSU_CONTROLLER;

extern bool Headless;

int AmtsuWrite30(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	if (value < AMTSUDEVICES) {
		CurrentDevice = value;
		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int AmtsuRead30(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	*value = CurrentDevice;
	return EBUSSUCCESS;
}

int AmtsuWriteMID(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	return EBUSERROR;
}

int AmtsuReadMID(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	*value = AmtsuDevices[CurrentDevice].MID;
	return EBUSSUCCESS;
}

int AmtsuWriteCMD(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	if (!dev->Action)
		return EBUSSUCCESS;

	XrLockMutex(&dev->Mutex);
	int retval = dev->Action(dev, value, proc);
	XrUnlockMutex(&dev->Mutex);

	return retval;
}

int AmtsuReadCMD(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	if (!dev->IsBusy) {
		*value = 0;
		return EBUSSUCCESS;
	}

	XrLockMutex(&dev->Mutex);
	int retval = dev->IsBusy(dev, value, proc);
	XrUnlockMutex(&dev->Mutex);

	return retval;
}

int AmtsuWritePortA(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	XrLockMutex(&dev->Mutex);
	dev->PortAValue = value;
	XrUnlockMutex(&dev->Mutex);

	return EBUSSUCCESS;
}

int AmtsuReadPortA(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	*value = dev->PortAValue;
	return EBUSSUCCESS;
}

int AmtsuWritePortB(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	XrLockMutex(&dev->Mutex);
	dev->PortBValue = value;
	XrUnlockMutex(&dev->Mutex);

	return EBUSSUCCESS;
}

int AmtsuReadPortB(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	struct AmtsuDevice *dev = &AmtsuDevices[CurrentDevice];

	if (!dev->Present)
		return EBUSERROR;

	*value = dev->PortBValue;
	return EBUSSUCCESS;
}

void AmtsuReset() {
	for (int i = 1; i < AMTSUDEVICES; i++) {
		if (AmtsuDevices[i].Present) {
			if (AmtsuDevices[i].Reset)
				AmtsuDevices[i].Reset(&AmtsuDevices[i]);

			AmtsuDevices[i].InterruptNumber = 0;
		}
	}
}

int AmtsuControllerAction(struct AmtsuDevice *dev, uint32_t value, void *proc) {
	switch(value) {
		case 1:
			// enable interrupts on device
			if (dev->PortBValue < AMTSUDEVICES)
				AmtsuDevices[dev->PortBValue].InterruptNumber = 48+dev->PortBValue;
			break;

		case 2:
			// reset
			AmtsuReset();
			break;

		case 3:
			// disable interrupts on device
			if (dev->PortBValue < AMTSUDEVICES)
				AmtsuDevices[dev->PortBValue].InterruptNumber = 0;
			break;
	}

	return EBUSSUCCESS;
}

void AmtsuInit() {
	CitronPorts[0x30].Present = 1;
	CitronPorts[0x30].ReadPort = AmtsuRead30;
	CitronPorts[0x30].WritePort = AmtsuWrite30;

	CitronPorts[0x31].Present = 1;
	CitronPorts[0x31].ReadPort = AmtsuReadMID;
	CitronPorts[0x31].WritePort = AmtsuWriteMID;

	CitronPorts[0x32].Present = 1;
	CitronPorts[0x32].ReadPort = AmtsuReadCMD;
	CitronPorts[0x32].WritePort = AmtsuWriteCMD;

	CitronPorts[0x33].Present = 1;
	CitronPorts[0x33].ReadPort = AmtsuReadPortA;
	CitronPorts[0x33].WritePort = AmtsuWritePortA;

	CitronPorts[0x34].Present = 1;
	CitronPorts[0x34].ReadPort = AmtsuReadPortB;
	CitronPorts[0x34].WritePort = AmtsuWritePortB;

	AmtsuDevices[AMTSU_CONTROLLER].Present = 1;
	AmtsuDevices[AMTSU_CONTROLLER].Action = AmtsuControllerAction;

	for (int i = 0; i < AMTSUDEVICES; i++) {
		XrInitializeMutex(&AmtsuDevices[i].Mutex);
	}

	if (!Headless) {
		KeyboardInit();
		MouseInit();
	}
}