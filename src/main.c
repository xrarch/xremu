#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <SDL.h>
#include <getopt.h>
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
#include "dks.h"
#include "rtc.h"
#include "pboard.h"
#include "mouse.h"
#include "ram256.h"
#include "serial.h"
#include "screen.h"
#include "tty.h"

SDL_mutex *IoMutex;

void LockIoMutex() {
	SDL_LockMutex(IoMutex);
}

void UnlockIoMutex() {
	SDL_UnlockMutex(IoMutex);
}

uint32_t SimulatorHz = CPUHZDEFAULT;

bool RAMDumpOnExit = false;

int ProcessorCount = 1;

uint32_t tick_start;
uint32_t tick_end;
int ticks;
bool done = false;

bool Headless = false;

void MainLoop(void);

#define CPUSTEPMS 20

int CPULoop(void *context) {
	CPUReset();

	int last_tick = SDL_GetTicks();

	while (1) {
		int this_tick = SDL_GetTicks();

		int dt = this_tick - last_tick;
		last_tick = this_tick;

		int cyclespertick = SimulatorHz/1000;
		int extracycles = SimulatorHz%1000; // squeeze in the sub-millisecond cycles

		CPUProgress = 20;

		for (int i = 0; i < dt; i++) {
			int cyclesleft = cyclespertick;

			if (i == CPUSTEPMS-1)
				cyclesleft += extracycles;

			while (cyclesleft > 0) {
				cyclesleft -= CPUDoCycles(cyclesleft, 1);
			}
		}

		int final_tick = SDL_GetTicks();

		int this_tick_duration = final_tick - this_tick;

		printf("%d\n", this_tick_duration);

		if (this_tick_duration < CPUSTEPMS) {
			SDL_Delay(CPUSTEPMS - this_tick_duration);
		}
	}
}

void CPUCreate(int id) {
	if (id != 0) {
		// TEMP
		return;
	}

	SDL_CreateThread(&CPULoop, "CPULoop", 0);
}

extern void TLBDump(void);

int main(int argc, char *argv[]) {
	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	IoMutex = SDL_CreateMutex();

	if (IoMutex == NULL) {
		fprintf(stderr, "Unable to allocate IoMutex: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();

	uint32_t memsize = 4 * 1024 * 1024;

#ifndef EMSCRIPTEN
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
		} else if (strcmp(argv[i], "-headless") == 0) {
			Headless = true;
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
				memsize = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "no ram size specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-cpuhz") == 0) {
			if (i+1 < argc) {
				SimulatorHz = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "no frequency specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-serialrx") == 0) {
			if (i+1 < argc) {
				if (!SerialSetRXFile(argv[i+1]))
					return 1;
				i++;
			} else {
				fprintf(stderr, "no file name specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-serialtx") == 0) {
			if (i+1 < argc) {
				if (!SerialSetTXFile(argv[i+1]))
					return 1;
				i++;
			} else {
				fprintf(stderr, "no file name specified\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-nocachesim") == 0) {
			CPUSimulateCaches = false;
		} else if (strcmp(argv[i], "-cachemiss") == 0) {
			CPUSimulateCacheStalls = true;
		} else if (strcmp(argv[i], "-cacheprint") == 0) {
			CPUPrintCache = true;
		} else if (strcmp(argv[i], "-diskprint") == 0) {
			DKSPrint = true;
		} else if (strcmp(argv[i], "-132column") == 0) {
			TTY132ColumnMode = true;
		} else if (strcmp(argv[i], "-cpus") == 0) {
			if (i+1 < argc) {
				ProcessorCount = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "no processor count specified\n");
				return 1;
			}
		} else {
			fprintf(stderr, "don't recognize option %s\n", argv[i]);
			return 1;
		}
	}
#endif // !EMSCRIPTEN

	if (!Headless) {
		ScreenCreate(KINNOW_FRAMEBUFFER_WIDTH,
					KINNOW_FRAMEBUFFER_HEIGHT,
					"XR/station Framebuffer",
					KinnowDraw,
					KeyboardPressed,
					KeyboardReleased,
					MousePressed,
					MouseReleased,
					MouseMoved);
	}

	if (EBusInit(memsize)) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

	if (ProcessorCount <= 0 || ProcessorCount > 8) {
		fprintf(stderr, "Bad processor count %d, should be between 1 and 8\n", ProcessorCount);
		return 1;
	}

#ifdef EMSCRIPTEN
	DKSAsynchronous = true;
	SerialAsynchronous = true;

	ROMLoadFile("bin/boot.bin");
	DKSAttachImage("bin/mintia.img");
	DKSAttachImage("bin/aisix.img");
#endif

	for (int i = 0; i < ProcessorCount; i++) {
		CPUCreate(i);
	}

	ScreenInit();
	ScreenDraw();

	done = false;

	ticks = 0;

	tick_start = SDL_GetTicks();
	tick_end = SDL_GetTicks();

#ifndef EMSCRIPTEN
	while (!done) {
		MainLoop();

		tick_end = SDL_GetTicks();
		int delay = 1000/FPS - (tick_end - tick_start);

		if (delay > 0) {
			SDL_Delay(delay);
		} else {
			// printf("time overrun %d\n", delay);
		}
	}
#else
	emscripten_set_main_loop(MainLoop, FPS, 0);
#endif

	NVRAMSave();

	if (RAMDumpOnExit)
		RAMDump();

	// TLBDump();

	return 0;
}

void MainLoop(void) {
	int dt = SDL_GetTicks() - tick_start;

	tick_start = SDL_GetTicks();

	if (dt < 1)
		dt = 1;

	if (dt > 20)
		dt = 20;

	for (int i = 0; i < dt; i++) {
		LockIoMutex();

		RTCInterval(1);
		DKSOperation(1);
		SerialInterval(1);

		UnlockIoMutex();
	}

	ScreenDraw();

	done = ScreenProcessEvents();

	ticks++;
}