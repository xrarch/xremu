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
#include <pthread.h>

#define FPS 60

#include "ebus.h"
#include "xr.h"
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
#include "lsic.h"

bool RAMDumpOnExit = false;
bool KinnowDumpOnExit = false;

bool done = false;

bool Headless = false;

extern void TLBDump(void);
extern void DbgInit(void);

int main(int argc, char *argv[]) {
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitor");
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // 0: point, 1 = linear

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();

	uint32_t memsize = 4 * 1024 * 1024;
	int threads = 0;

#ifndef EMSCRIPTEN
	for (int i = 1; i < argc; i++) {
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

		} else if (strcmp(argv[i], "-dumpfb") == 0) {
			KinnowDumpOnExit = true;

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
				XrProcessorFrequency = atoi(argv[i+1]);
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

#if XR_SIMULATE_CACHES
		} else if (strcmp(argv[i], "-cacheprint") == 0) {
			XrPrintCache = true;
#endif

		} else if (strcmp(argv[i], "-diskprint") == 0) {
			DKSPrint = true;

		} else if (strcmp(argv[i], "-132column") == 0) {
			TTY132ColumnMode = true;

		} else if (strcmp(argv[i], "-cpus") == 0) {
			if (i+1 < argc) {
				XrProcessorCount = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "no processor count specified\n");
				return 1;
			}

		} else if (strcmp(argv[i], "-threads") == 0) {
			if (i+1 < argc) {
				threads = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "no thread count specified\n");
				return 1;
			}

		} else {
			fprintf(stderr, "don't recognize option %s\n", argv[i]);
			return 1;
		}
	}
#endif // !EMSCRIPTEN

	if (XrProcessorCount <= 0 || XrProcessorCount > XR_PROC_MAX) {
		fprintf(stderr, "Bad processor count %d, should be between 1 and %d\n", XrProcessorCount, XR_PROC_MAX);
		return 1;
	}

#ifndef SINGLE_THREAD_MP
	if (threads > XrProcessorCount || threads == 0) {
		threads = (XrProcessorCount + 1) / 2;
	}
#else
	threads = 1;
#endif

	XrInitializeScheduler(threads);

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

#ifdef EMSCRIPTEN
	DKSAsynchronous = true;
	SerialAsynchronous = true;

	ROMLoadFile("bin/boot.bin");
	DKSAttachImage("bin/mintia.img");
	DKSAttachImage("bin/aisix.img");
#endif

	DbgInit();

	ScreenInit();
	ScreenDraw();

	done = false;

	XrInitializeProcessors();

	XrStartScheduler();

	int tick_end = 0;
	int tick_start = 0;

	while (!done) {
		tick_start = SDL_GetTicks();

		ScreenDraw();
		done = ScreenProcessEvents();

		int tick_after_draw = SDL_GetTicks();

		SerialInterval(tick_after_draw - tick_end);

		// Kick all the CPU threads to get them to execute a frame time worth of
		// CPU simulation. We do this from the frame update thread (the main
		// thread) so that the execution of all CPUs is synchronized with frame
		// updates, which makes vblank interrupts useful. It also makes it so
		// that the CPU threads are (hopefully, assuming the host OS scheduler
		// is willing) running at around the same time. This is extremely
		// important for IPI response and spinlocks. It would suck for a
		// simulated CPU to spend its entire timeslice spinning for IPI
		// completion or spinlock release just because the other CPU's host
		// thread is asleep waiting for its next timeslice.

		XrScheduleAllNextFrameWork(tick_after_draw - tick_end);

		tick_end = SDL_GetTicks();
		int delay = 1000/FPS - (tick_end - tick_start);

		if (delay > 0) {
			SDL_Delay(delay);
		}
	}

	NVRAMSave();

	if (RAMDumpOnExit) {
		RAMDump();
	}

	if (KinnowDumpOnExit) {
		KinnowDump();
	}

	// TLBDump();

	return 0;
}