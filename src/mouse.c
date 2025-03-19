#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "amtsu.h"
#include "mouse.h"

#include "lsic.h"
#include "xr.h"

int MousePressedButton = 0;
int MouseReleasedButton = 0;

uint32_t MouseMovedV = 0;

uint32_t MouseDX = 0;
uint32_t MouseDY = 0;

void MouseReset(struct AmtsuDevice *dev) {
	dev->PortAValue = 0;
	dev->PortBValue = 0;
	MousePressedButton  = 0;
	MouseReleasedButton = 0;
	MouseDX = 0;
	MouseDY = 0;
}

int MouseAction(struct AmtsuDevice *dev, uint32_t value, void *proc) {
	switch(value) {
		case 1: // read info
			if (MousePressedButton) {
				dev->PortAValue = 1;
				dev->PortBValue = MousePressedButton;
				MousePressedButton = 0;
				return EBUSSUCCESS;
			} else if (MouseReleasedButton) {
				dev->PortAValue = 2;
				dev->PortBValue = MouseReleasedButton;
				MouseReleasedButton = 0;
				return EBUSSUCCESS;
			} else if (MouseMovedV) {
				dev->PortAValue = 3;
				dev->PortBValue = MouseMovedV;
				MouseDX = 0;
				MouseDY = 0;
				MouseMovedV = 0;
				return EBUSSUCCESS;
			}

			((XrProcessor *)(proc))->Progress--;
			dev->PortAValue = 0;
			break;

		case 2: // reset
			MouseReset(dev);
			break;
	}

	return EBUSSUCCESS;
}

void MousePressed(struct Screen *screen, int button) {
	if ((button >= 1) && (button <= 3)) {
		if (button == 3)
			button = 2;
		else if (button == 2)
			button = 3;

		SDL_LockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);

		MousePressedButton = button;
		if (AmtsuDevices[AMTSU_MOUSE].InterruptNumber)
			LsicInterrupt(AmtsuDevices[AMTSU_MOUSE].InterruptNumber);

		SDL_UnlockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);
	}
}

void MouseReleased(struct Screen *screen, int button) {
	if ((button >= 1) && (button <= 3)) {
		if (button == 3)
			button = 2;
		else if (button == 2)
			button = 3;

		SDL_LockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);

		MouseReleasedButton = button;
		if (AmtsuDevices[AMTSU_MOUSE].InterruptNumber)
			LsicInterrupt(AmtsuDevices[AMTSU_MOUSE].InterruptNumber);

		SDL_UnlockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);
	}
}

void MouseMoved(struct Screen *screen, int dx, int dy) {
	SDL_LockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);

	MouseDX += dx;
	MouseDY += dy;

	MouseMovedV = ((MouseDX&0xFFFF)<<16)|(MouseDY&0xFFFF);

	if (AmtsuDevices[AMTSU_MOUSE].InterruptNumber)
		LsicInterrupt(AmtsuDevices[AMTSU_MOUSE].InterruptNumber);

	SDL_UnlockMutex(AmtsuDevices[AMTSU_MOUSE].Mutex);
}

void MouseInit() {
	AmtsuDevices[AMTSU_MOUSE].Present = 1;
	AmtsuDevices[AMTSU_MOUSE].MID = 0x4D4F5553; // mouse
	AmtsuDevices[AMTSU_MOUSE].PortAValue = 0;
	AmtsuDevices[AMTSU_MOUSE].Action = MouseAction;
	AmtsuDevices[AMTSU_MOUSE].Reset = MouseReset;
}