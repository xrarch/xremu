#include <SDL.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPS 60
#define TPF 1
#define TPS (FPS * TPF)

#include "ebus.h"
#include "cpu.h"
#include "kinnowfb.h"
#include "keybd.h"
#include "dks.h"
#include "rtc.h"
#include "pboard.h"
#include "mouse.h"
#include "ram256.h"
#include "serial.h"

void RAMDump();

bool RAMDumpOnExit = false;

static int best_display(const SDL_Rect *rect);

static double scale_display(SDL_Window *window, const SDL_Rect *risc_rect, SDL_Rect *display_rect);

bool KinnowDraw(SDL_Texture *texture);

int main(int argc, char *argv[]) {
	SDL_Rect risc_rect = {
		.w = KINNOW_FRAMEBUFFER_WIDTH,
		.h = KINNOW_FRAMEBUFFER_HEIGHT
	};

	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	if (EBusInit(4 * 1024 * 1024)) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

	SDL_EnableScreenSaver();

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

	SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);

	for (int i = 1; i < argc; i++) {
		// shut up this is beautiful...

		if (strcmp(argv[i], "-dks") == 0) {
			if (i+1 < argc) {
				if (!DKSAttachImage(argv[i+1]))
					return 1;
				i++;
			} else {
				fprintf(stderr, "no disk image specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-nvram") == 0) {
			if (i+1 < argc) {
				if (!NVRAMLoadFile(argv[i+1]))
					return 1;
				i++;
			} else {
				fprintf(stderr, "no nvram image specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-rom") == 0) {
			if (i+1 < argc) {
				if (!ROMLoadFile(argv[i+1]))
					return 1;
				i++;
			} else {
				fprintf(stderr, "no rom image specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-dumpram") == 0) {
			RAMDumpOnExit = true;
		} else if (strcmp(argv[i], "-asyncdisk") == 0) {
			DKSAsynchronous = true;
		} else if (strcmp(argv[i], "-asyncserial") == 0) {
			SerialAsynchronous = true;
		} else if (strcmp(argv[i], "-ramsize") == 0) {
			if (i+1 < argc) {
				if (RAMInit(atoi(argv[i+1])) == -1)
					return 1;
				i++;
			} else {
				fprintf(stderr, "no ram size specified\n");
				return 1;
			}
		} else {
			fprintf(stderr, "don't recognize option %s\n", argv[i]);
			return 1;
		}
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

	int ticks = 0;

	uint32_t tick_start = SDL_GetTicks();
	uint32_t tick_end = SDL_GetTicks();

	bool mousegrabbed = false;

	while (!done) {
		int dt = SDL_GetTicks() - tick_start;

		tick_start = SDL_GetTicks();

		if (!dt)
			dt = 1;

		int cyclespertick = CPUHZ/TPS/dt;
		int extracycles = CPUHZ/TPS - (cyclespertick*dt);

		CPUProgress = 5;

		for (int i = 0; i < dt; i++) {
			int cyclesleft = cyclespertick;

			if (i == dt-1)
				cyclesleft += extracycles;

			RTCInterval(1);
			DKSOperation(1);
			SerialInterval(1);

			while (cyclesleft > 0) {
				cyclesleft -= CPUDoCycles(cyclesleft);
			}
		}

		if ((ticks%TPF) == 0) {
			KinnowDraw(texture);
			
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, &risc_rect, &display_rect);
			SDL_RenderPresent(renderer);
		}

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					if (RAMDumpOnExit)
						RAMDump();

					done = true;
					break;
				}

				case SDL_WINDOWEVENT: {
					break;
				}

				case SDL_MOUSEMOTION: {
					if (mousegrabbed)
						MouseMoved(event.motion.xrel, event.motion.yrel);
					break;
				}

				case SDL_MOUSEBUTTONDOWN: {
					if (!mousegrabbed) {
						SDL_SetWindowGrab(window, true);
						SDL_ShowCursor(false);
						SDL_SetWindowTitle(window, "LIMNstation - strike F12 to uncapture mouse");
						SDL_SetRelativeMouseMode(true);
						mousegrabbed = true;
						break;
					}

					MousePressed(event.button.button);
					break;
				}


				case SDL_MOUSEBUTTONUP: {
					MouseReleased(event.button.button);
					break;
				}

				case SDL_KEYDOWN:
					if ((event.key.keysym.scancode == SDL_SCANCODE_F12) && mousegrabbed) {
						SDL_SetWindowGrab(window, false);
						SDL_ShowCursor(true);
						SDL_SetWindowTitle(window, "LIMNstation");
						SDL_SetRelativeMouseMode(false);
						mousegrabbed = false;
						break;
					}

					KeyboardPressed(event.key.keysym.scancode);
					break;

				case SDL_KEYUP:
					KeyboardReleased(event.key.keysym.scancode);
					break;
			}
		}

		ticks++;

		tick_end = SDL_GetTicks();
		int delay = 1000/TPS - (tick_end - tick_start);
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