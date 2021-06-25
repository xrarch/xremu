#include <SDL.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"
#include "kinnowfb.h"
#include "kinnowpal.h"

uint32_t *KinnowFB = 0;

uint32_t FBSize;

uint32_t SlotInfo[64];

uint32_t KinnowRegisters[64];

enum KINNOWREGISTERS {
	REGSIZE = 0,
	REGVRAM = 1,
	REGSTATUS = 5,
	REGMODE = 6,
	REGCAUSE = 7,
};

uint32_t DirtyRectX1 = 0;
uint32_t DirtyRectX2 = 0;
uint32_t DirtyRectY1 = 0;
uint32_t DirtyRectY2 = 0;

bool IsDirty = false;

void MakeDirty(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
	if (!IsDirty) {
		DirtyRectX1 = x1;
		DirtyRectY1 = y1;

		DirtyRectX2 = x2;
		DirtyRectY2 = y2;

		IsDirty = true;

		return;
	}

	if (x1 < DirtyRectX1)
		DirtyRectX1 = x1;

	if (y1 < DirtyRectY1)
		DirtyRectY1 = y1;

	if (x2 > DirtyRectX2) {
		if (x2 < KINNOW_FRAMEBUFFER_WIDTH) {
			DirtyRectX2 = x2;
		} else {
			DirtyRectX2 = KINNOW_FRAMEBUFFER_WIDTH-1;
		}
	}

	if (y2 > DirtyRectY2) {
		if (y2 < KINNOW_FRAMEBUFFER_HEIGHT) {
			DirtyRectY2 = y2;
		} else {
			DirtyRectY2 = KINNOW_FRAMEBUFFER_HEIGHT-1;
		}
	}
}

int KinnowWrite(uint32_t address, uint32_t type, uint32_t value) {
	if ((address >= 0x3000) && (address < 0x3100)) {
		address -= 0x3000;

		switch(type) {
			case EBUSBYTE:
				((uint8_t*)KinnowRegisters)[address] = value;
				break;

			case EBUSINT:
				((uint16_t*)KinnowRegisters)[address/2] = value;
				break;

			case EBUSLONG:
				KinnowRegisters[address/4] = value;
				break;
		}

		return EBUSSUCCESS;
	} else if (address >= 0x100000) {
		address -= 0x100000;

		if (address >= FBSize)
			return EBUSERROR;

		uint32_t pix = address/2;
		uint32_t x = pix%KINNOW_FRAMEBUFFER_WIDTH;
		uint32_t y = pix/KINNOW_FRAMEBUFFER_WIDTH;

		switch(type) {
			case EBUSBYTE:
				MakeDirty(x, y, x, y);
				((uint8_t*)KinnowFB)[address] = value;
				break;

			case EBUSINT:
				MakeDirty(x, y, x+1, y);
				((uint16_t*)KinnowFB)[address/2] = value;
				break;

			case EBUSLONG:
				MakeDirty(x, y, x+3, y);
				KinnowFB[address/4] = value;
				break;
		}

		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int KinnowRead(uint32_t address, uint32_t type, uint32_t *value) {
	if (address < 0x100) { // SlotInfo
		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)SlotInfo)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)SlotInfo)[address/2];
				break;

			case EBUSLONG:
				*value = SlotInfo[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if ((address >= 0x3000) && (address < 0x3100)) {
		address -= 0x3000;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)KinnowRegisters)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)KinnowRegisters)[address/2];
				break;

			case EBUSLONG:
				*value = KinnowRegisters[address/4];
				break;
		}

		return EBUSSUCCESS;
	} else if (address >= 0x100000) {
		address -= 0x100000;

		if (address >= FBSize)
			return EBUSERROR;

		switch(type) {
			case EBUSBYTE:
				*value = ((uint8_t*)KinnowFB)[address];
				break;

			case EBUSINT:
				*value = ((uint16_t*)KinnowFB)[address/2];
				break;

			case EBUSLONG:
				*value = KinnowFB[address/4];
				break;
		}

		return EBUSSUCCESS;
	}

	return EBUSERROR;
}

static uint32_t PixelBuffer[KINNOW_FRAMEBUFFER_WIDTH*KINNOW_FRAMEBUFFER_HEIGHT];

void KinnowDraw(SDL_Texture *texture) {
	if (!IsDirty)
		return;

	uint32_t dirtyaddr = (DirtyRectY1*KINNOW_FRAMEBUFFER_WIDTH)+DirtyRectX1;

	uint32_t pixbufindex = 0;

	int width = DirtyRectX2-DirtyRectX1+1;
	int height = DirtyRectY2-DirtyRectY1+1;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			PixelBuffer[pixbufindex++] = KinnowPalette[((uint16_t*)KinnowFB)[dirtyaddr+x]&0x7FFF];
		}

		dirtyaddr += KINNOW_FRAMEBUFFER_WIDTH;
	}

    SDL_Rect rect = {
      .x = DirtyRectX1,
      .y = DirtyRectY1,
      .w = width,
      .h = height,
    };

    SDL_UpdateTexture(texture, &rect, PixelBuffer, rect.w * 4);

	IsDirty = false;
}

int KinnowInit() {
	FBSize = KINNOW_FRAMEBUFFER_WIDTH * KINNOW_FRAMEBUFFER_HEIGHT * 2;

	KinnowFB = malloc(FBSize);

	if (KinnowFB == 0)
		return -1;

	memset(KinnowFB, 0, FBSize);

	EBusBranches[24].Present = 1;
	EBusBranches[24].Write = KinnowWrite;
	EBusBranches[24].Read = KinnowRead;
	EBusBranches[24].Reset = 0;

	memset(&SlotInfo, 0, 256);

	SlotInfo[0] = 0x0C007CA1; // ebus magic number
	SlotInfo[1] = 0x4B494E35; // board ID

	strcpy(&((char*)SlotInfo)[8], "kinnowfb,16");

	KinnowRegisters[REGSIZE] = (KINNOW_FRAMEBUFFER_HEIGHT << 12) | KINNOW_FRAMEBUFFER_WIDTH;
	KinnowRegisters[REGVRAM] = FBSize;

	IsDirty = true;
	DirtyRectX2 = KINNOW_FRAMEBUFFER_WIDTH-1;
	DirtyRectY2 = KINNOW_FRAMEBUFFER_HEIGHT-1;

	return 0;
}