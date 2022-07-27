#include <SDL.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "screen.h"
#include "tty.h"

void TTYDraw(struct Screen *screen) {
	SDL_Texture *texture = ScreenGetTexture(screen);

	
}

void TTYKeyPressed(struct Screen *screen, int sdlscancode) {

}

struct TTY *TTYCreate(int width, int height, char *title) {
	struct TTY *tty = malloc(sizeof(struct TTY));

	if (!tty) {
		return 0;
	}

	tty->TextBuffer = malloc(width*height*2);

	if (!tty->TextBuffer) {
		free(tty);
		return 0;
	}

	tty->Width = width;
	tty->Height = height;

	tty->CursorX = 0;
	tty->CursorY = 0;

	tty->Screen = ScreenCreate(width*FONTWIDTH, height*FONTHEIGHT, title,
							TTYDraw,
							TTYKeyPressed,
							0,
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