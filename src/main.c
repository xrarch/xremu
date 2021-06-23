#include <SDL.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPS 60

#include "ebus.h"

#include "cpu.h"

#include "kinnowfb.h"

static int best_display(const SDL_Rect *rect);

int main(int argc, char *argv[]) {
	SDL_Rect risc_rect = {
		.w = KINNOW_FRAMEBUFFER_WIDTH,
		.h = KINNOW_FRAMEBUFFER_HEIGHT
	};

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();
	SDL_ShowCursor(false);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

	bool fullscreen = false;

	int zoom = 0;

	int window_flags = SDL_WINDOW_HIDDEN;
	int display = 0;
	if (fullscreen) {
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		display = best_display(&risc_rect);
	}
	if (zoom == 0) {
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(display, &bounds) == 0 &&
			bounds.h >= risc_rect.h * 2 && bounds.w >= risc_rect.w * 2) {
			zoom = 2;
		} else {
			zoom = 1;
		}
	}
	SDL_Window *window = SDL_CreateWindow("LIMNstation",
										SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
										SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
										(int)(risc_rect.w * zoom),
										(int)(risc_rect.h * zoom),
										window_flags);

	if (!window) {
		fprintf(stderr, "failed to create window\n");
		return 1;
	}

	if (EBusInit(4 * 1024 * 1024)) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

	CPUReset();

	CPUDoCycles(CPUHZ);

	return 0;
}

static int best_display(const SDL_Rect *rect) {
	int best = 0;
	int display_cnt = SDL_GetNumVideoDisplays();
	for (int i = 0; i < display_cnt; i++) {
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(i, &bounds) == 0 &&
			bounds.h == rect->h && bounds.w >= rect->w) {
			best = i;
			if (bounds.w == rect->w) {
				break;  // exact match
			}
		}
	}
	return best;
}