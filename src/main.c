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

XrProcessor *CpuTable[XR_PROC_MAX];
XrProcessor *XrIoMutexProcessor;

#ifndef EMSCRIPTEN

SDL_mutex *ScacheMutexes[XR_CACHE_MUTEXES];

SDL_mutex *IoMutex;
SDL_sem* CpuSemaphore;

void LockIoMutex() {
	SDL_LockMutex(IoMutex);
	XrIoMutexProcessor = 0;
}

void UnlockIoMutex() {
	SDL_UnlockMutex(IoMutex);
}

void XrPokeCpu(XrProcessor *proc) {
	SDL_SemPost(proc->LoopSemaphore);
}

void XrLockIoMutex(XrProcessor *proc) {
	SDL_LockMutex(IoMutex);
	XrIoMutexProcessor = proc;
}

void XrUnlockIoMutex() {
	XrIoMutexProcessor = 0;
	SDL_UnlockMutex(IoMutex);
}

void XrLockCache(XrProcessor *proc, uint32_t tag) {
	SDL_LockMutex((SDL_mutex *)(proc->CacheMutexes[(tag >> XR_DC_LINE_SIZE_LOG) & XR_CACHE_MUTEX_MASK]));
}

void XrUnlockCache(XrProcessor *proc, uint32_t tag) {
	SDL_UnlockMutex((SDL_mutex *)(proc->CacheMutexes[(tag >> XR_DC_LINE_SIZE_LOG) & XR_CACHE_MUTEX_MASK]));
}

void XrLockScache(uint32_t tag) {
	SDL_LockMutex(ScacheMutexes[(tag >> XR_DC_LINE_SIZE_LOG) & XR_CACHE_MUTEX_MASK]);
}

void XrUnlockScache(uint32_t tag) {
	SDL_UnlockMutex(ScacheMutexes[(tag >> XR_DC_LINE_SIZE_LOG) & XR_CACHE_MUTEX_MASK]);
}

#else

void LockIoMutex() {}
void UnlockIoMutex() {}
void XrPokeCpu(XrProcessor *proc) {}
void XrLockIoMutex(XrProcessor *proc) {}
void XrUnlockIoMutex() {}
void XrLockCache(XrProcessor *proc, uint32_t tag) {}
void XrUnlockCache(XrProcessor *proc, uint32_t tag) {}
void XrLockScache(uint32_t tag) {}
void XrUnlockScache(uint32_t tag) {}

uint32_t emscripten_last_tick = 0;

#endif

uint32_t SimulatorHz = 20000000;

bool RAMDumpOnExit = false;
bool KinnowDumpOnExit = false;

uint32_t XrProcessorCount = 1;

bool done = false;

bool Headless = false;

#ifdef EMSCRIPTEN
void MainLoop(void);
#endif

#define CPUSTEPMS 40

int CpuLoop(void *context) {
	XrProcessor *proc = (XrProcessor *)context;

	int last_tick = SDL_GetTicks();

	int cyclespertick = SimulatorHz/1000;
	int extracycles = SimulatorHz%1000; // squeeze in the sub-millisecond cycles

	while (1) {
		int this_tick = SDL_GetTicks();
		int dt = this_tick - last_tick;
		last_tick = this_tick;

		// printf("delta time=%d\n", dt);

		proc->Progress = XR_POLL_MAX;

		for (int i = 0; i < dt; i++) {
			int cyclesleft = cyclespertick;

			if (i == dt-1)
				cyclesleft += extracycles;

			XrExecute(proc, cyclesleft, 1);

			if (proc->Id == 0) {
				// The thread for CPU 0 also does the RTC intervals, once per
				// millisecond of CPU time. 

				RTCInterval(1);
			}
		}

		int final_tick = SDL_GetTicks();
		int this_tick_duration = final_tick - this_tick;

		// printf("duration=%d, delay=%d\n", this_tick_duration, CPUSTEPMS - this_tick_duration);

		if (this_tick_duration < CPUSTEPMS) {
			SDL_SemWaitTimeout(proc->LoopSemaphore, CPUSTEPMS - this_tick_duration);
		}
	}
}

void CpuCreate(int id) {
	XrProcessor *proc = malloc(sizeof(XrProcessor));

	if (!proc) {
		fprintf(stderr, "failed to allocate cpu %d", id);
		exit(1);
	}

	CpuTable[id] = proc;
	proc->Id = id;

	XrReset(proc);

#ifndef EMSCRIPTEN
	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		proc->CacheMutexes[i] = SDL_CreateMutex();

		if (!proc->CacheMutexes[i]) {
			fprintf(stderr, "Unable to allocate cache mutex: %s", SDL_GetError());
			exit(1);
		}
	}

	proc->LoopSemaphore = SDL_CreateSemaphore(0);

	if (!proc->LoopSemaphore) {
		fprintf(stderr, "Unable to allocate loop semaphore: %s", SDL_GetError());
		exit(1);
	}

	SDL_CreateThread(&CpuLoop, "CpuLoop", proc);
#endif
}

extern void TLBDump(void);

int main(int argc, char *argv[]) {
	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();

	uint32_t memsize = 4 * 1024 * 1024;

#ifndef EMSCRIPTEN
	IoMutex = SDL_CreateMutex();

	if (!IoMutex) {
		fprintf(stderr, "Unable to allocate IoMutex: %s", SDL_GetError());
		return 1;
	}

	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		ScacheMutexes[i] = SDL_CreateMutex();

		if (!ScacheMutexes[i]) {
			fprintf(stderr, "Unable to allocate ScacheMutex: %s", SDL_GetError());
			return 1;
		}
	}

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
			XrSimulateCaches = false;
		} else if (strcmp(argv[i], "-cachemiss") == 0) {
			XrSimulateCacheStalls = true;
		} else if (strcmp(argv[i], "-cacheprint") == 0) {
			XrPrintCache = true;
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

	if (XrProcessorCount <= 0 || XrProcessorCount > XR_PROC_MAX) {
		fprintf(stderr, "Bad processor count %d, should be between 1 and %d\n", XrProcessorCount, XR_PROC_MAX);
		return 1;
	}

	if (!XrSimulateCaches && XrProcessorCount > 1) {
		fprintf(stderr, "Can't simulate multiprocessor with disabled caches\n");
		return 1;
	}

#ifdef EMSCRIPTEN
	DKSAsynchronous = true;
	SerialAsynchronous = true;

	ROMLoadFile("bin/boot.bin");
	DKSAttachImage("bin/mintia.img");
	DKSAttachImage("bin/aisix.img");
#endif

	ScreenInit();
	ScreenDraw();

	done = false;

	for (int i = 0; i < XrProcessorCount; i++) {
		CpuCreate(i);
	}

#ifndef EMSCRIPTEN
	int tick_end = 0;
	int tick_start = 0;

	while (!done) {
		tick_start = SDL_GetTicks();

		ScreenDraw();
		done = ScreenProcessEvents();

		int tick_after_draw = SDL_GetTicks();

		LockIoMutex();
		DKSInterval(tick_after_draw - tick_end);
		SerialInterval(tick_after_draw - tick_end);
		UnlockIoMutex();

		tick_end = SDL_GetTicks();
		int delay = 1000/FPS - (tick_end - tick_start);

		if (delay > 0) {
			SDL_Delay(delay);
		}
	}
#else
	XrIoMutexProcessor = CpuTable[0];
	emscripten_last_tick = SDL_GetTicks();
	emscripten_set_main_loop(MainLoop, FPS, 0);
#endif

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

void EnqueueCallback(uint32_t interval, uint32_t (*callback)(uint32_t, void*), void *param) {
	SDL_AddTimer(interval, callback, param);
}

#ifdef EMSCRIPTEN
void MainLoop(void) {
	// Dumbed down CPU driving loop for emscripten since it sucks and doesn't
	// like SDL's threads.

	ScreenDraw();
	done = ScreenProcessEvents();

	XrProcessor *proc = CpuTable[0];

	int this_tick = SDL_GetTicks();
	int dt = this_tick - emscripten_last_tick;
	emscripten_last_tick = this_tick;

	int cyclespertick = SimulatorHz/1000;
	int extracycles = SimulatorHz%1000; // squeeze in the sub-millisecond cycles

	proc->Progress = XR_POLL_MAX;

	for (int i = 0; i < dt; i++) {
		int cyclesleft = cyclespertick;

		if (i == dt-1)
			cyclesleft += extracycles;

		XrExecute(proc, cyclesleft, 1);

		// In emscripten, the interval functions are also responsible for
		// driving async device activity.

		DKSInterval(1);
		RTCInterval(1);
		SerialInterval(1);
	}
}
#endif