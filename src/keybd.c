#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "amtsu.h"
#include "keybd.h"

#include "cpu.h"
#include "lsic.h"

struct KeyInfo {
	unsigned char code;
};

static struct KeyInfo KeyMap[SDL_NUM_SCANCODES];

bool OutstandingPressed[KEYBDMAXCODE+1];
bool OutstandingReleased[KEYBDMAXCODE+1];
bool Pressed[KEYBDMAXCODE+1];

void KeyboardReset(struct AmtsuDevice *dev) {
	dev->PortAValue = 0xFFFF;
	memset(&OutstandingPressed, false, sizeof(OutstandingPressed));
	memset(&OutstandingReleased, false, sizeof(OutstandingReleased));
	memset(&Pressed, false, sizeof(Pressed));
}

int KeyboardAction(struct AmtsuDevice *dev, uint32_t value) {
	switch(value) {
		case 1: // pop scancode
			for (int i = 0; i <= KEYBDMAXCODE; i++) {
				if (OutstandingReleased[i]) {
					dev->PortAValue = (i | 0x8000);
					OutstandingReleased[i] = false;
					OutstandingPressed[i] = false;
					return EBUSSUCCESS;
				} else if (OutstandingPressed[i]) {
					dev->PortAValue = i;
					OutstandingPressed[i] = false;
					return EBUSSUCCESS;
				}
			}

			CPUProgress--;
			dev->PortAValue = 0xFFFF;
			break;

		case 2: // reset
			KeyboardReset(dev);
			break;

		case 3: // check key pressed
			if (dev->PortAValue <= KEYBDMAXCODE) {
				if (Pressed[dev->PortAValue]){
					dev->PortAValue = 1;
				} else {
					CPUProgress--;
					dev->PortAValue = 0;
				}
			} else {
				dev->PortAValue = 0;
			}
			break;
	}

	return EBUSSUCCESS;
}

void KeyboardPressed(struct Screen *screen, int sdlscancode) {
	int code = KeyMap[sdlscancode].code;

	if (code) {
		OutstandingPressed[code-1] = true;
		Pressed[code-1] = true;
		if (AmtsuDevices[1].InterruptNumber)
			LSICInterrupt(AmtsuDevices[1].InterruptNumber);
	}
}

void KeyboardReleased(struct Screen *screen, int sdlscancode) {
	int code = KeyMap[sdlscancode].code;

	if (code) {
		OutstandingReleased[code-1] = true;
		Pressed[code-1] = false;
		if (AmtsuDevices[1].InterruptNumber)
			LSICInterrupt(AmtsuDevices[1].InterruptNumber);
	}
}

void KeyboardInit() {
	AmtsuDevices[1].Present = 1;
	AmtsuDevices[1].MID = 0x8FC48FC4; // keyboard
	AmtsuDevices[1].PortAValue = 0xFFFF;
	AmtsuDevices[1].Action = KeyboardAction;
	AmtsuDevices[1].Reset = KeyboardReset;
}

static struct KeyInfo KeyMap[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_A] = { 0x01 },
	[SDL_SCANCODE_B] = { 0x02 },
	[SDL_SCANCODE_C] = { 0x03 },
	[SDL_SCANCODE_D] = { 0x04 },
	[SDL_SCANCODE_E] = { 0x05 },
	[SDL_SCANCODE_F] = { 0x06 },
	[SDL_SCANCODE_G] = { 0x07 },
	[SDL_SCANCODE_H] = { 0x08 },
	[SDL_SCANCODE_I] = { 0x09 },
	[SDL_SCANCODE_J] = { 0x0A },
	[SDL_SCANCODE_K] = { 0x0B },
	[SDL_SCANCODE_L] = { 0x0C },
	[SDL_SCANCODE_M] = { 0x0D },
	[SDL_SCANCODE_N] = { 0x0E },
	[SDL_SCANCODE_O] = { 0x0F },
	[SDL_SCANCODE_P] = { 0x10 },
	[SDL_SCANCODE_Q] = { 0x11 },
	[SDL_SCANCODE_R] = { 0x12 },
	[SDL_SCANCODE_S] = { 0x13 },
	[SDL_SCANCODE_T] = { 0x14 },
	[SDL_SCANCODE_U] = { 0x15 },
	[SDL_SCANCODE_V] = { 0x16 },
	[SDL_SCANCODE_W] = { 0x17 },
	[SDL_SCANCODE_X] = { 0x18 },
	[SDL_SCANCODE_Y] = { 0x19 },
	[SDL_SCANCODE_Z] = { 0x1A },

	[SDL_SCANCODE_0] = { 0x1B },
	[SDL_SCANCODE_1] = { 0x1C },
	[SDL_SCANCODE_2] = { 0x1D },
	[SDL_SCANCODE_3] = { 0x1E },
	[SDL_SCANCODE_4] = { 0x1F },
	[SDL_SCANCODE_5] = { 0x20 },
	[SDL_SCANCODE_6] = { 0x21 },
	[SDL_SCANCODE_7] = { 0x22 },
	[SDL_SCANCODE_8] = { 0x23 },
	[SDL_SCANCODE_9] = { 0x24 },

	[SDL_SCANCODE_SEMICOLON] = { 0x25 },
	[SDL_SCANCODE_SPACE]     = { 0x26 },
	[SDL_SCANCODE_TAB]       = { 0x27 },

	[SDL_SCANCODE_MINUS]        = { 0x28 },
	[SDL_SCANCODE_EQUALS]       = { 0x29 },
	[SDL_SCANCODE_LEFTBRACKET]  = { 0x2A },
	[SDL_SCANCODE_RIGHTBRACKET] = { 0x2B },
	[SDL_SCANCODE_BACKSLASH]    = { 0x2C },
	[SDL_SCANCODE_NONUSHASH]    = { 0x2C },  // same key as BACKSLASH

	[SDL_SCANCODE_SLASH]      = { 0x2E },
	[SDL_SCANCODE_PERIOD]     = { 0x2F },
	[SDL_SCANCODE_APOSTROPHE] = { 0x30 },
	[SDL_SCANCODE_COMMA]      = { 0x31 },
	[SDL_SCANCODE_GRAVE]      = { 0x32 },

	[SDL_SCANCODE_RETURN]    = { 0x33 },
	[SDL_SCANCODE_BACKSPACE] = { 0x34 },
	[SDL_SCANCODE_CAPSLOCK]  = { 0x35 },
	[SDL_SCANCODE_ESCAPE]    = { 0x36 },

	[SDL_SCANCODE_LEFT]     = { 0x37 },
	[SDL_SCANCODE_RIGHT]    = { 0x38 },
	[SDL_SCANCODE_DOWN]     = { 0x39 },
	[SDL_SCANCODE_UP]       = { 0x3A },

	[SDL_SCANCODE_LCTRL]  = { 0x51 },
	[SDL_SCANCODE_RCTRL]  = { 0x52 },
	[SDL_SCANCODE_LSHIFT] = { 0x53 },
	[SDL_SCANCODE_RSHIFT] = { 0x54 },
	[SDL_SCANCODE_LALT]   = { 0x55 },
	[SDL_SCANCODE_RALT]   = { 0x56 },

	[SDL_SCANCODE_KP_DIVIDE]   = { 0x2E },
	[SDL_SCANCODE_KP_MINUS]    = { 0x28 },
	[SDL_SCANCODE_KP_ENTER]    = { 0x33 },
	[SDL_SCANCODE_KP_0]        = { 0x1B },
	[SDL_SCANCODE_KP_1]        = { 0x1C },
	[SDL_SCANCODE_KP_2]        = { 0x1D },
	[SDL_SCANCODE_KP_3]        = { 0x1E },
	[SDL_SCANCODE_KP_4]        = { 0x1F },
	[SDL_SCANCODE_KP_5]        = { 0x20 },
	[SDL_SCANCODE_KP_6]        = { 0x21 },
	[SDL_SCANCODE_KP_7]        = { 0x22 },
	[SDL_SCANCODE_KP_8]        = { 0x23 },
	[SDL_SCANCODE_KP_9]        = { 0x24 },
	[SDL_SCANCODE_KP_PERIOD]   = { 0x2F },
};