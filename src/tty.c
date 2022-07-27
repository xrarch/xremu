#include <SDL.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kinnowfb.h"
#include "text.h"
#include "screen.h"
#include "tty.h"

uint32_t TTYPalette[16] = {
	0x191919,
	0xCC4C4C,
	0x57A64E,
	0xDEDE6C,
	0x3366CC,
	0xE57FD8,
	0x00FFFF,
	0x999999,

// light palette

	0x4C4C4C,
	0xF2B2CC,
	0x7FCC19,
	0xDEDE6C,
	0x99B2F2,
	0x99B2F2,
	0xB4FFFF,
	0xF0F0F0,
};

#define TTYDEFAULTATTR 0x0F

void TTYMakeDirty(struct TTY *tty, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
	if (x2 < x1) {
		int tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	if (y2 < y1) {
		int tmp = y2;
		y2 = y1;
		y1 = tmp;
	}

	if (!tty->IsDirty) {
		tty->DirtyX1 = x1;
		tty->DirtyY1 = y1;

		tty->DirtyX2 = x2;
		tty->DirtyY2 = y2;

		tty->IsDirty = true;

		return;
	}

	if (x1 < tty->DirtyX1)
		tty->DirtyX1 = x1;

	if (y1 < tty->DirtyY1)
		tty->DirtyY1 = y1;

	if (x2 > tty->DirtyX2) {
		if (x2 < tty->Width) {
			tty->DirtyX2 = x2;
		} else {
			tty->DirtyX2 = tty->Width-1;
		}
	}

	if (y2 > tty->DirtyY2) {
		if (y2 < tty->Height) {
			tty->DirtyY2 = y2;
		} else {
			tty->DirtyY2 = tty->Height-1;
		}
	}
}

void TTYMoveCursor(struct TTY *tty, int x, int y) {
	if (x >= tty->Width)
		x = tty->Width-1;

	if (y >= tty->Height)
		y = tty->Height-1;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	TTYMakeDirty(tty, tty->CursorX, tty->CursorY, x, y);

	tty->CursorX = x;
	tty->CursorY = y;
}

// reuse the kinnowfb one to save memory
extern uint32_t PixelBuffer[KINNOW_FRAMEBUFFER_WIDTH*KINNOW_FRAMEBUFFER_HEIGHT];

void TTYDraw(struct Screen *screen) {
	struct TTY *tty = screen->Context1;

	if (!tty->IsDirty)
		return;

	SDL_Texture *texture = ScreenGetTexture(screen);

	uint16_t *textbuffer = tty->TextBuffer;

	uint32_t dirtyaddr = (tty->DirtyY1*tty->Width)+tty->DirtyX1;

	uint32_t pixbufindex = 0;

	int width = tty->DirtyX2-tty->DirtyX1+1;
	int height = tty->DirtyY2-tty->DirtyY1+1;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint16_t cell = textbuffer[dirtyaddr+x];

			if ((((tty->DirtyX1+x) == tty->CursorX) && ((tty->DirtyY1+y) == tty->CursorY))
				&& !tty->IsCursorHidden) {
				TextBlitCharacter(cell&0xFF,
							TextFont8x16,
							FONTWIDTH,
							FONTHEIGHT,
							x*FONTWIDTH,
							y*FONTHEIGHT,
							width*FONTWIDTH,
							TTYPalette[cell>>12],
							TTYPalette[(cell>>8)&15],
							PixelBuffer);
			} else {
				TextBlitCharacter(cell&0xFF,
							TextFont8x16,
							FONTWIDTH,
							FONTHEIGHT,
							x*FONTWIDTH,
							y*FONTHEIGHT,
							width*FONTWIDTH,
							TTYPalette[(cell>>8)&15],
							TTYPalette[cell>>12],
							PixelBuffer);
			}
		}

		dirtyaddr += tty->Width;
	}

	SDL_Rect rect = {
		.x = tty->DirtyX1*FONTWIDTH,
		.y = tty->DirtyY1*FONTHEIGHT,
		.w = width*FONTWIDTH,
		.h = height*FONTHEIGHT,
	};

	SDL_UpdateTexture(texture, &rect, PixelBuffer, rect.w * 4);

	tty->IsDirty = 0;
}

void TTYKeyPressed(struct Screen *screen, int sdlscancode) {
	struct TTY *tty = (struct TTY *)(screen->Context1);

	if (sdlscancode == SDL_SCANCODE_LCTRL || sdlscancode == SDL_SCANCODE_RCTRL) {
		tty->IsCtrl = 1;
		return;
	} else if (sdlscancode == SDL_SCANCODE_LSHIFT || sdlscancode == SDL_SCANCODE_RSHIFT) {
		tty->IsShift = 1;
		return;
	} else if (sdlscancode == SDL_SCANCODE_LEFT) {
		tty->Input(tty, 0x8000 | 'D');
		return;
	} else if (sdlscancode == SDL_SCANCODE_RIGHT) {
		tty->Input(tty, 0x8000 | 'C');
		return;
	} else if (sdlscancode == SDL_SCANCODE_UP) {
		tty->Input(tty, 0x8000 | 'A');
		return;
	} else if (sdlscancode == SDL_SCANCODE_DOWN) {
		tty->Input(tty, 0x8000 | 'B');
		return;
	}

	char c;

	if (tty->IsCtrl) {
		c = KeyMapCtrl[sdlscancode];
	} else if (tty->IsShift) {
		c = KeyMapShift[sdlscancode];
	} else {
		c = KeyMapNormal[sdlscancode];
	}

	if (c != -1)
		tty->Input(tty, c);
}

void TTYKeyReleased(struct Screen *screen, int sdlscancode) {
	struct TTY *tty = (struct TTY *)(screen->Context1);

	if (sdlscancode == SDL_SCANCODE_LCTRL || sdlscancode == SDL_SCANCODE_RCTRL) {
		tty->IsCtrl = 0;
		return;
	} else if (sdlscancode == SDL_SCANCODE_LSHIFT || sdlscancode == SDL_SCANCODE_RSHIFT) {
		tty->IsShift = 0;
		return;
	}
}

void TTYScrollUp(struct TTY *tty) {
	uint16_t *textbuffer = tty->TextBuffer;

	textbuffer += tty->ScrollWindowTop * tty->Width;

	memcpy(textbuffer,
		&textbuffer[tty->Width],
		(tty->ScrollWindowBottom - tty->ScrollWindowTop)*tty->Width*2);

	textbuffer += (tty->ScrollWindowBottom - tty->ScrollWindowTop) * tty->Width;

	for (int i = 0; i < tty->Width; i++) {
		textbuffer[i] = TTYDEFAULTATTR<<8;
	}

	TTYMakeDirty(tty, 0, tty->ScrollWindowTop, tty->Width-1, tty->ScrollWindowBottom);
}

void TTYScrollDown(struct TTY *tty) {
	uint16_t *textbuffer = tty->TextBuffer;

	textbuffer += tty->ScrollWindowTop * tty->Width;

	memmove(&textbuffer[tty->Width],
		textbuffer,
		(tty->ScrollWindowBottom - tty->ScrollWindowTop)*tty->Width*2);

	for (int i = 0; i < tty->Width; i++) {
		textbuffer[i] = TTYDEFAULTATTR<<8;
	}

	TTYMakeDirty(tty, 0, tty->ScrollWindowTop, tty->Width-1, tty->ScrollWindowBottom);
}

void TTYNewline(struct TTY *tty) {
	tty->CursorY++;

	int oldx = tty->CursorX;

	tty->CursorX = 0;

	TTYMakeDirty(tty, oldx, tty->CursorY-1, 0, tty->CursorY);

	if (tty->CursorY > tty->ScrollWindowBottom) {
		tty->CursorY = tty->ScrollWindowBottom;
		TTYScrollUp(tty);
	}
}

void TTYBackspace(struct TTY *tty) {
	int oldx = tty->CursorX;
	int oldy = tty->CursorY;

	tty->CursorX--;

	if (tty->CursorX == -1) {
		tty->CursorX = tty->Width-1;
		tty->CursorY--;

		if (tty->CursorY == -1) {
			tty->CursorY = 0;
			tty->CursorX = 0;
		}
	}

	TTYMakeDirty(tty, oldx, oldy, tty->CursorX, tty->CursorY);
}

void TTYEscSetColor(struct TTY *tty) {
	int color;

	int param0 = tty->EscapeParams[0];

	if (param0 == 0) {
		// reset

		tty->CurrentAttributes = TTYDEFAULTATTR;

		return;
	} else if (param0 == 7) {
		// invert

		color = tty->CurrentAttributes;
		tty->CurrentAttributes >>= 4;
		tty->CurrentAttributes |= color<<4;
	} else if (param0 == 39) {
		// default fg
		tty->CurrentAttributes &= 0xF0;
		tty->CurrentAttributes |= (TTYDEFAULTATTR&0xF);
	} else if (param0 == 49) {
		// default bg
		tty->CurrentAttributes &= 0x0F;
		tty->CurrentAttributes |= (TTYDEFAULTATTR&0xF0);
	} else if (param0 >= 30 && param0 <= 37) {
		// dark foreground
		tty->CurrentAttributes &= 0xF0;
		tty->CurrentAttributes |= tty->EscapeParams[0]-30;
	} else if (param0 >= 40 && param0 <= 47) {
		// dark background
		tty->CurrentAttributes &= 0x0F;
		tty->CurrentAttributes |= (tty->EscapeParams[0]-40)<<4;
	} else if (param0 >= 90 && param0 <= 97) {
		// light foreground
		tty->CurrentAttributes &= 0xF0;
		tty->CurrentAttributes |= tty->EscapeParams[0]-90+8;
	} else if (param0 >= 100 && param0 <= 107) {
		// light background
		tty->CurrentAttributes &= 0x0F;
		tty->CurrentAttributes |= (tty->EscapeParams[0]-100+8)<<4;
	}
}

void TTYEscClear(struct TTY *tty) {
	if (tty->EscapeParams[0] == 2) {
		for (int i = 0; i < tty->Width*tty->Height; i++) {
			tty->TextBuffer[i] = TTYDEFAULTATTR<<8;
		}

		tty->IsCursorHidden = 0;
		tty->ScrollWindowTop = 0;
		tty->ScrollWindowBottom = tty->Height-1;

		TTYMakeDirty(tty, 0, 0, tty->Width-1, tty->Height-1);
	}
}

void TTYEscClearLine(struct TTY *tty) {
	uint16_t *textbuffer = &tty->TextBuffer[tty->CursorY*tty->Width];

	int count = 0;

	if (tty->EscapeParams[0] == 0) {
		// clear to end of line
		textbuffer += tty->CursorX;
		count = tty->Width - tty->CursorX;
		TTYMakeDirty(tty, tty->CursorX, tty->CursorY, tty->Width-1, tty->CursorY);
	} else if (tty->EscapeParams[0] == 2) {
		// clear entire line
		count = tty->Width;
		TTYMakeDirty(tty, 0, tty->CursorY, tty->Width-1, tty->CursorY);
	}

	for (int i = 0; i < count; i++) {
		textbuffer[i] = TTYDEFAULTATTR<<8;
	}
}

void TTYEscSetScrollMargins(struct TTY *tty) {
	if (tty->EscapeParams[0] == 0) {
		tty->ScrollWindowTop = 0;
	} else {
		tty->ScrollWindowTop = tty->EscapeParams[0]-1;

		if (tty->ScrollWindowTop >= tty->Height) {
			tty->ScrollWindowTop = 0;
		}
	}

	if (tty->EscapeParams[1] == 0) {
		tty->ScrollWindowBottom = tty->Height-1;
	} else {
		tty->ScrollWindowBottom = tty->EscapeParams[1]-1;

		if (tty->ScrollWindowBottom >= tty->Height) {
			tty->ScrollWindowBottom = 0;
		}
	}

	if (tty->ScrollWindowBottom < tty->ScrollWindowTop) {
		tty->ScrollWindowBottom = tty->ScrollWindowTop;
	}
}

void TTYParseEscape(struct TTY *tty, char c) {
	if (c >= '0' && c <= '9') {
		tty->EscapeParams[tty->EscapeIndex] *= 10;
		tty->EscapeParams[tty->EscapeIndex] += c - '0';
		return;
	}

	if (tty->IsEscape == 1) {
		switch (c) {
			case '[':
				// this is supposed to do something but i ignore it because lazy
				return;

			case ';':
				tty->EscapeIndex++;
				if (tty->EscapeIndex >= TTYPARAMCOUNT) {
					tty->EscapeIndex = 0;
				}
				return;
		}

		tty->IsEscape = 0;

		switch (c) {
			case 'm':
				// set color
				TTYEscSetColor(tty);
				return;

			case 'H':
				// set cursor pos
				TTYMoveCursor(tty, tty->EscapeParams[1]-1, tty->EscapeParams[0]-1);
				return;

			case 'J':
				// clear
				TTYEscClear(tty);
				return;

			case 'K':
				// clear line
				TTYEscClearLine(tty);
				return;

			case '?':
				tty->IsEscape = 2;
				return;

			case 'r':
				// set scroll margins
				TTYEscSetScrollMargins(tty);
				return;

			case 'S':
				// scroll up
				TTYScrollUp(tty);
				return;

			case 'T':
				// scroll down
				TTYScrollDown(tty);
				return;
		}
	} else if (tty->IsEscape == 2) {
		tty->IsEscape = 0;

		if (tty->EscapeParams[0] == 25) {
			if (c == 'h') {
				// show cursor
				if (tty->IsCursorHidden) {
					TTYMakeDirty(tty, tty->CursorX, tty->CursorY, tty->CursorX, tty->CursorY);
				}

				tty->IsCursorHidden = 0;

				return;
			} else if (c == 'l') {
				// hide cursor
				if (!tty->IsCursorHidden) {
					TTYMakeDirty(tty, tty->CursorX, tty->CursorY, tty->CursorX, tty->CursorY);
				}

				tty->IsCursorHidden = 1;

				return;
			}
		}
	}
}

void TTYPutCharacter(struct TTY *tty, char c) {
	int curx = tty->CursorX;
	int cury = tty->CursorY;

	if (c > 0x7F)
		return;

	if (c == 0)
		return;

	if (tty->IsEscape) {
		TTYParseEscape(tty, c);
		return;
	}

	switch (c) {
		case '\n':
			TTYNewline(tty);
			return;

		case '\b':
			TTYBackspace(tty);
			return;

		case '\r':
			tty->CursorX = 0;
			TTYMakeDirty(tty, curx, cury, tty->CursorX, cury);
			return;

		case 0x1B:
			tty->IsEscape = 1;
			tty->EscapeIndex = 0;

			for (int i = 0; i < TTYPARAMCOUNT; i++) {
				tty->EscapeParams[i] = 0;
			}

			return;

		default:
			tty->TextBuffer[cury*tty->Width+curx] = (tty->CurrentAttributes<<8)|c;

			tty->CursorX++;

			TTYMakeDirty(tty, curx, cury, tty->CursorX, cury);

			if (tty->CursorX >= tty->Width)
				TTYNewline(tty);

			return;
	}
}

struct TTY *TTYCreate(int width, int height, char *title, TTYInputF input) {
	struct TTY *tty = malloc(sizeof(struct TTY));

	if (!tty) {
		return 0;
	}

	tty->TextBuffer = malloc(width*height*2);

	if (!tty->TextBuffer) {
		free(tty);
		return 0;
	}

	for (int i = 0; i < width*height; i++) {
		tty->TextBuffer[i] = TTYDEFAULTATTR<<8;
	}

	tty->Width = width;
	tty->Height = height;

	tty->CursorX = 0;
	tty->CursorY = 0;

	tty->DirtyX1 = 0;
	tty->DirtyY1 = 0;
	tty->DirtyX2 = width-1;
	tty->DirtyY2 = height-1;
	tty->IsDirty = 1;

	tty->IsShift = 0;
	tty->IsCtrl = 0;
	tty->IsEscape = 0;
	tty->IsCursorHidden = 0;

	tty->Input = input;

	tty->ScrollWindowTop = 0;
	tty->ScrollWindowBottom = height-1;

	tty->CurrentAttributes = 0x0F;

	tty->Screen = ScreenCreate(width*FONTWIDTH, height*FONTHEIGHT, title,
							TTYDraw,
							TTYKeyPressed,
							TTYKeyReleased,
							0,
							0,
							0);

	if (!tty->Screen) {
		free(tty->TextBuffer);
		free(tty);
		return 0;
	}

	tty->Screen->Context1 = tty;

	return tty;
}