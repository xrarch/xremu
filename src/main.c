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

#include "keybd.h"

static int best_display(const SDL_Rect *rect);

static double scale_display(SDL_Window *window, const SDL_Rect *risc_rect, SDL_Rect *display_rect);

void KinnowDraw(SDL_Texture *texture);

void NVRAMSave();

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

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == NULL) {
		fprintf(stderr, "Could not create renderer: %s", SDL_GetError());
		return 1;
	}

	SDL_Texture *texture = SDL_CreateTexture(renderer,
											SDL_PIXELFORMAT_ARGB8888,
											SDL_TEXTUREACCESS_STREAMING,
											risc_rect.w,
											risc_rect.h);
	if (texture == NULL) {
		fprintf(stderr, "Could not create texture: %s", SDL_GetError());
		return 1;
	}

	if (EBusInit(4 * 1024 * 1024)) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

	CPUReset();

	SDL_Rect display_rect;
	double display_scale = scale_display(window, &risc_rect, &display_rect);
	KinnowDraw(texture);

	SDL_ShowWindow(window);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, &risc_rect, &display_rect);
	SDL_RenderPresent(renderer);

	bool done = false;
	bool mouse_was_offscreen = false;
	while (!done) {
		uint32_t frame_start = SDL_GetTicks();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					done = true;
					break;
				}

				case SDL_WINDOWEVENT: {
					break;
				}

				case SDL_MOUSEMOTION: {
					break;
				}

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					break;
				}

				case SDL_KEYDOWN:
					KeyboardPressed(event.key.keysym.scancode);
					break;

				case SDL_KEYUP:
					KeyboardReleased(event.key.keysym.scancode);
					break;
			}
		}

		//risc_set_time(risc, frame_start);
		CPUDoCycles(CPUHZ/FPS);

		KinnowDraw(texture);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, &risc_rect, &display_rect);
		SDL_RenderPresent(renderer);

		uint32_t frame_end = SDL_GetTicks();
		int delay = frame_start + 1000/FPS - frame_end;
		if (delay > 0) {
			SDL_Delay(delay);
		} else {
			// printf("time overrun %d\n", delay);
		}
	}

	NVRAMSave();

	return 0;
}

static double scale_display(SDL_Window *window, const SDL_Rect *risc_rect, SDL_Rect *display_rect) {
	int win_w, win_h;
	SDL_GetWindowSize(window, &win_w, &win_h);
	double limn_aspect = (double)risc_rect->w / risc_rect->h;
	double window_aspect = (double)win_w / win_h;

	double scale;
	if (limn_aspect > window_aspect) {
		scale = (double)win_w / risc_rect->w;
	} else {
		scale = (double)win_h / risc_rect->h;
	}

	int w = (int)ceil(risc_rect->w * scale);
	int h = (int)ceil(risc_rect->h * scale);
	*display_rect = (SDL_Rect){
		.w = w, .h = h,
		.x = (win_w - w) / 2,
		.y = (win_h - h) / 2
	};
	return scale;
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