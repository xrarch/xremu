#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "amtsu.h"
#include "mouse.h"

#include "cpu.h"
#include "lsic.h"

int MousePressedButton = 0;
int MouseReleasedButton = 0;

uint32_t MouseMovedV = 0;

int MouseDX = 0;
int MouseDY = 0;

void MouseReset(struct AmtsuDevice *dev) {
	dev->PortAValue = 0;
	dev->PortBValue = 0;
	MousePressedButton  = 0;
	MouseReleasedButton = 0;
	MouseDX = 0;
	MouseDY = 0;
}

int MouseAction(struct AmtsuDevice *dev, uint32_t value) {
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

			CPUProgress--;
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

		MousePressedButton = button;
		if (AmtsuDevices[2].InterruptNumber)
			LSICInterrupt(AmtsuDevices[2].InterruptNumber);
	}
}

void MouseReleased(struct Screen *screen, int button) {
	if ((button >= 1) && (button <= 3)) {
		if (button == 3)
			button = 2;
		else if (button == 2)
			button = 3;

		MouseReleasedButton = button;
		if (AmtsuDevices[2].InterruptNumber)
			LSICInterrupt(AmtsuDevices[2].InterruptNumber);
	}
}

void MouseMoved(struct Screen *screen, int dx, int dy) {
	MouseDX += dx;
	MouseDY += dy;

	MouseMovedV = ((MouseDX&0xFFFF)<<16)|(MouseDY&0xFFFF);

	if (AmtsuDevices[2].InterruptNumber)
		LSICInterrupt(AmtsuDevices[2].InterruptNumber);
}

void MouseInit() {
	AmtsuDevices[2].Present = 1;
	AmtsuDevices[2].MID = 0x4D4F5553; // mouse
	AmtsuDevices[2].PortAValue = 0;
	AmtsuDevices[2].Action = MouseAction;
	AmtsuDevices[2].Reset = MouseReset;
}