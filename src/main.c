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

XrNumaNode XrNumaNodes[XR_NODE_MAX];

bool RAMDumpOnExit = false;
bool KinnowDumpOnExit = false;

bool done = false;

bool Headless = false;

extern void TLBDump(void);
extern void DbgInit(void);

int TickEnd = 0;
int TickStart = 0;
int TickAfterDraw = 0;

void MainLoop(void) {
#ifndef EMSCRIPTEN
	while (!done) {
#endif
		TickStart = SDL_GetTicks();

		ScreenDraw();
		done = ScreenProcessEvents();

		int TickAfterDraw = SDL_GetTicks();

		SerialInterval(TickAfterDraw - TickEnd);

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

		XrScheduleAllNextFrameWork(TickAfterDraw - TickEnd);

#ifdef EMSCRIPTEN
		XrSchedulerLoop(0);
#endif

		TickEnd = SDL_GetTicks();

#ifndef EMSCRIPTEN
		int delay = 1000/FPS - (TickEnd - TickStart);

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
#endif

	// TLBDump();
}

int main(int argc, char *argv[]) {
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitor");
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // 0: point, 1 = linear

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();

	int threads = 0;
	int easyproccount = 1;
	uint32_t easymemcount = 4 * 1024 * 1024;
	bool explicitnodes = false;

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
				easymemcount = atoi(argv[i+1]);
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
				easyproccount = atoi(argv[i+1]);
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

		} else if (strcmp(argv[i], "-node") == 0) {
			if (i+3 < argc) {
				int nodeid = atoi(argv[i+1]);
				if (nodeid >= XR_NODE_MAX) {
					fprintf(stderr, "node id must be at most %d\n", XR_NODE_MAX - 1);
					return 1;
				}

				int nodeprocs = atoi(argv[i+2]);
				if (nodeprocs > XR_PROC_PER_NODE_MAX) {
					fprintf(stderr, "at most %d procs are allowed per node\n", XR_PROC_PER_NODE_MAX);
					return 1;
				}

				int noderam = atoi(argv[i+3]);
				if (noderam > RAM_PER_NODE) {
					fprintf(stderr, "at most %d bytes allowed per node\n", RAM_PER_NODE);
					return 1;
				}

				XrNumaNodes[nodeid].RamSize = noderam;
				XrNumaNodes[nodeid].ProcessorCount = nodeprocs;
				explicitnodes = true;

				i += 3;
			} else {
				fprintf(stderr, "node id, cpu count, and memory count must be specified\n");
				return 1;
			}

		} else {
			fprintf(stderr, "don't recognize option %s\n", argv[i]);
			return 1;
		}
	}
#endif // !EMSCRIPTEN

#ifdef EMSCRIPTEN
	easyproccount = 2;
#endif

	if (!explicitnodes) {
		// The NUMA nodes were not explicitly specified, so we should make some.
		// We fill nodes up with processors starting from node 0.

		if (easyproccount <= 0 || easyproccount > XR_PROC_MAX) {
			fprintf(stderr, "Bad processor count %d, should be between 1 and %d\n", easyproccount, XR_PROC_MAX);
			return 1;
		}

		if (easymemcount > RAMMAXIMUM) {
			fprintf(stderr, "Too much RAM: maximum is %d bytes (%d bytes given)\n", RAMMAXIMUM, easymemcount);
			return 1;
		}

		int nodecount = 0;

		for (int i = 0; i < XR_NODE_MAX; i++) {
			if (easyproccount < XR_PROC_PER_NODE_MAX) {
				break;
			}

			XrNumaNodes[i].ProcessorCount = XR_PROC_PER_NODE_MAX;
			easyproccount -= XR_PROC_PER_NODE_MAX;
			nodecount++;
		}

		if (easyproccount) {
			// Add the remaining partial count to the last node.
			XrNumaNodes[nodecount].ProcessorCount = easyproccount;
			nodecount++;
		}

		// Now divide the RAM among the nodes evenly.

		int pernoderam = easymemcount / nodecount;

		// Eliminate the remainder

		easymemcount = nodecount * pernoderam;

		if (pernoderam > RAM_PER_NODE) {
			pernoderam = RAM_PER_NODE;
		}

		for (int i = 0; i < nodecount; i++) {
			XrNumaNodes[i].RamSize = pernoderam;
			easymemcount -= pernoderam;
		}

		if (easymemcount) {
			// There was leftover RAM. Try to create memory-only nodes with it.

			for (int i = nodecount; i < XR_NODE_MAX && easymemcount != 0; i++) {
				pernoderam = easymemcount;

				if (pernoderam > RAM_PER_NODE) {
					pernoderam = RAM_PER_NODE;
				}

				XrNumaNodes[i].RamSize = pernoderam;

				easymemcount -= pernoderam;
			}
		}
	}

	for (int i = 0; i < XR_NODE_MAX; i++) {
		XrProcessorCount += XrNumaNodes[i].ProcessorCount;
	}

#ifndef SINGLE_THREAD_MP
	if (threads > XrProcessorCount || threads == 0) {
		threads = (XrProcessorCount + 1) / 2;
	}
#else
	if (threads != 0) {
		fprintf(stderr, "Warning: Built as SINGLE_THREAD_MP, forcing to 1 thread\n");
	}

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

	if (EBusInit()) {
		fprintf(stderr, "failed to initialize ebus\n");
		return 1;
	}

#ifdef EMSCRIPTEN
	DKSAsynchronous = true;
	SerialAsynchronous = true;

	ROMLoadFile("embin/boot.bin");
	DKSAttachImage("embin/mintia.img");
	DKSAttachImage("embin/aisix.img");
	DKSAttachImage("embin/mintia2.img");
#endif

	DbgInit();

	ScreenInit();
	ScreenDraw();

	done = false;

	XrInitializeProcessors();

	XrStartScheduler();

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(MainLoop, FPS, 0);
#else
	MainLoop();
#endif

	return 0;
}