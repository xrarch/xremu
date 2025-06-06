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
#include "ebus.h"

bool TTY132ColumnMode = false;

uint32_t TTYPalette[16] = {
	0x000000, // black
	0xFF0000, // red
	0x007300, // green
	0xFFFF00, // yellow
	0x8373EE, // blue
	0xFF00FF, // magenta
	0x00FFFF, // cyan
	0xB4B4B4, // light gray
	0x393939, // dark gray
	0xF68373, // light red
	0x7BFFBD, // light green
	0xFFFFB4, // light yellow
	0xB4D5FF, // light blue
	0xFF7BFF, // light magenta
	0xB4FFFF, // light cyan
	0xFFFFFF, // white
};

#define TTYDEFAULTATTR 0x0F

#define TTYMARGINCHARTOP  1
#define TTYMARGINCHARSIDE 2

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

	if (x2 >= tty->Width) {
		x2 = tty->Width-1;
	}

	if (y2 >= tty->Height) {
		y2 = tty->Height-1;
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
		tty->DirtyX2 = x2;
	}

	if (y2 > tty->DirtyY2) {
		tty->DirtyY2 = y2;
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

static uint32_t PixelBufferTty[1024 * 768];

void TTYDraw(struct Screen *screen) {
	struct TTY *tty = screen->Context1;

	if (!tty->IsDirty) {
		return;
	}

	SDL_Texture *texture = ScreenGetTexture(screen);

	uint16_t *textbuffer = tty->TextBuffer;

	SDL_LockMutex(tty->Mutex);

	uint32_t dirtyaddr = (tty->DirtyY1*tty->Width)+tty->DirtyX1;

	uint32_t pixbufindex = 0;

	int width = tty->DirtyX2-tty->DirtyX1+1;
	int height = tty->DirtyY2-tty->DirtyY1+1;

	int checkcurx;

	if (tty->CursorX < tty->Width) {
		checkcurx = tty->CursorX;
	} else {
		checkcurx = tty->Width-1;
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint16_t cell = textbuffer[dirtyaddr+x];

			if ((((tty->DirtyX1+x) == checkcurx) && ((tty->DirtyY1+y) == tty->CursorY))
				&& !tty->IsCursorHidden) {
				TextBlitCharacter(cell&0x7F,
							tty->FontBMP,
							tty->FontWidth,
							tty->FontHeight,
							x*tty->FontWidth,
							y*tty->FontHeight,
							width*tty->FontWidth,
							TTYPalette[cell>>12],
							TTYPalette[(cell>>8)&15],
							PixelBufferTty);
			} else {
				TextBlitCharacter(cell&0x7F,
							tty->FontBMP,
							tty->FontWidth,
							tty->FontHeight,
							x*tty->FontWidth,
							y*tty->FontHeight,
							width*tty->FontWidth,
							TTYPalette[(cell>>8)&15],
							TTYPalette[cell>>12],
							PixelBufferTty);
			}
		}

		dirtyaddr += tty->Width;
	}

	tty->IsDirty = 0;

	SDL_UnlockMutex(tty->Mutex);

	SDL_Rect rect = {
		.x = (tty->DirtyX1 + TTYMARGINCHARSIDE) * tty->FontWidth,
		.y = (tty->DirtyY1 + TTYMARGINCHARTOP) * tty->FontHeight,
		.w = width * tty->FontWidth,
		.h = height * tty->FontHeight,
	};

	SDL_UpdateTexture(texture, &rect, PixelBufferTty, rect.w * 4);
}

void TTYKeyPressed(struct Screen *screen, int sdlscancode) {
	struct TTY *tty = (struct TTY *)(screen->Context1);

	SDL_LockMutex(tty->Mutex);

	switch (sdlscancode) {
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
			tty->IsCtrl = 1;

			goto exit;

		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
			tty->IsShift = 1;

			goto exit;

		case SDL_SCANCODE_LEFT:
			tty->Input(tty, 0x1B);
			tty->Input(tty, '[');
			tty->Input(tty, 'D');
			
			goto exit;

		case SDL_SCANCODE_RIGHT:
			tty->Input(tty, 0x1B);
			tty->Input(tty, '[');
			tty->Input(tty, 'C');
			
			goto exit;

		case SDL_SCANCODE_UP:
			tty->Input(tty, 0x1B);
			tty->Input(tty, '[');
			tty->Input(tty, 'A');
			
			goto exit;

		case SDL_SCANCODE_DOWN:
			tty->Input(tty, 0x1B);
			tty->Input(tty, '[');
			tty->Input(tty, 'B');
			
			goto exit;
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

exit:
	SDL_UnlockMutex(tty->Mutex);
}

void TTYKeyReleased(struct Screen *screen, int sdlscancode) {
	struct TTY *tty = (struct TTY *)(screen->Context1);

	SDL_LockMutex(tty->Mutex);

	if (sdlscancode == SDL_SCANCODE_LCTRL || sdlscancode == SDL_SCANCODE_RCTRL) {
		tty->IsCtrl = 0;
	} else if (sdlscancode == SDL_SCANCODE_LSHIFT || sdlscancode == SDL_SCANCODE_RSHIFT) {
		tty->IsShift = 0;
	}

	SDL_UnlockMutex(tty->Mutex);
}

void TTYScrollUp(struct TTY *tty) {
	uint16_t *textbuffer = tty->TextBuffer;

	textbuffer += tty->ScrollWindowTop * tty->Width;

	memmove(textbuffer,
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

	TTYMakeDirty(tty, tty->CursorX, tty->CursorY-1, 0, tty->CursorY);

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
	char querystr[16];

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

			case 'n':
				// query cursor position

				querystr[0] = 0x1B;

				int num = sprintf(&querystr[1], "[%d;%dR", tty->CursorY+1, tty->CursorX+1);

				for (int i = 0; i < num+1; i++) {
					tty->Input(tty, querystr[i]);
				}

				break;

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

		case '\t':
			tty->CursorX += 1;
			tty->CursorX = (tty->CursorX + 7) & ~7;

			if (tty->CursorX >= tty->Width) {
				tty->CursorX = tty->Width - 1;
			}

			TTYMakeDirty(tty, curx, cury, tty->CursorX, tty->CursorY);
			return;

		case 0x1B:
			tty->IsEscape = 1;
			tty->EscapeIndex = 0;

			for (int i = 0; i < TTYPARAMCOUNT; i++) {
				tty->EscapeParams[i] = 0;
			}

			return;

		default:
			if (tty->CursorX >= tty->Width) {
				tty->CursorX = 0;
				TTYNewline(tty);
			}

			tty->TextBuffer[tty->CursorY*tty->Width+tty->CursorX] = (tty->CurrentAttributes<<8)|c;

			tty->CursorX++;

			TTYMakeDirty(tty, curx, cury, tty->CursorX, tty->CursorY);

			return;
	}
}

struct TTY *TTYCreate(int width, int height, char *title, TTYInputF input) {
	struct TTY *tty = malloc(sizeof(struct TTY));

	if (!tty) {
		abort();
	}

	tty->Mutex = SDL_CreateMutex();

	if (!tty->Mutex) {
		abort();
	}

	tty->TextBuffer = malloc(width*height*2);

	if (!tty->TextBuffer) {
		abort();
	}

	for (int i = 0; i < width*height; i++) {
		tty->TextBuffer[i] = TTYDEFAULTATTR<<8;
	}

	tty->Width = width;
	tty->Height = height;

	tty->CursorX = 0;
	tty->CursorY = 0;

	if (width < 132) {
		tty->FontWidth = FONTWIDTH_80COL;
		tty->FontHeight = FONTHEIGHT_80COL;
		tty->FontBMP = TextFont80COL;
	} else {
		tty->FontWidth = FONTWIDTH_132COL;
		tty->FontHeight = FONTHEIGHT_132COL;
		tty->FontBMP = TextFont132COL;
	}

	int FontHeight;
	int FontWidth;

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

	tty->Screen = ScreenCreate((width + TTYMARGINCHARSIDE*2) * tty->FontWidth, (height + TTYMARGINCHARTOP*2) * tty->FontHeight, title,
							TTYDraw,
							TTYKeyPressed,
							TTYKeyReleased,
							0,
							0,
							0);

	if (!tty->Screen) {
		abort();
	}

	tty->Screen->Context1 = tty;

	return tty;
}