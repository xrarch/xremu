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
#include "lsic.h"

XrProcessor *CpuTable[XR_PROC_MAX];

#ifndef EMSCRIPTEN

SDL_mutex *ScacheMutexes[XR_CACHE_MUTEXES];

#else

uint32_t emscripten_last_tick = 0;

#endif

uint32_t SimulatorHz = 20000000;

bool RAMDumpOnExit = false;
bool KinnowDumpOnExit = false;

uint32_t XrProcessorCount = 1;
uint32_t CpuThreadCount = 0;

bool done = false;

bool Headless = false;

#ifdef EMSCRIPTEN
void MainLoop(void);
#endif

#define CPUSTEPMS 17 // 60Hz rounded up

int CpuLoop(void *context) {
	uintptr_t id = (uintptr_t)context;

	// Execute CPU time from each CPU in sequence, until all timeslices have run
	// down.

	int procid = 0;

	int cyclesperms = (SimulatorHz+999)/1000;
	int pausemargin = cyclesperms * 2;

	while (1) {
		// Wait until the next frame.

		SDL_SemWait(CpuTable[id]->LoopSemaphore);

		// Iterate over the CPUs in a cycle until all of the virtual CPU time
		// for this frame (~17ms worth) has been executed. Multiple threads may
		// execute this work loop simultaneously, to distribute guest workload
		// over host cores (the default, as of writing, is half as many host
		// threads as guest CPUs).
		//
		// We increment the "done" count when a CPU's timeslice has dropped to
		// zero and break out of the loop when all of the CPUs are done.
		//
		// We skip to the next CPU early, without incrementing the done count,
		// if a CPU executes the HLT instruction or many PAUSE instructions.
		//
		// If it executes HLT, it gives up the remainder of its virtual
		// millisecond because it has reached an idle point and has no work to
		// do. It will still be iterated over for each remaining millisecond in
		// its timeslice in case the timer interrupt needs to be handled (which
		// would un-halt it) or an IPI is received from another CPU.
		//
		// If it executes many PAUSE instructions, it does not give up the
		// remainder of its virtual millisecond; the remaining cycles are
		// credited to it for the next iteration. This is because it does have
		// work to do, it's just spin-waiting for another CPU to do something
		// (i.e. unlock a spinlock, acknowledge an IPI, etc...) before it can
		// proceed. We skip to the next CPU early, in the hopes that happens
		// and it can be freed from its spin-wait by the time the next iteration
		// rolls around.

		for (int done = 0; done < XrProcessorCount; procid = (procid + 1) % XrProcessorCount) {
			XrProcessor *proc = CpuTable[procid];

			// Trylock the CPU's run lock.

			if (SDL_TryLockMutex(proc->RunLock) == SDL_MUTEX_TIMEDOUT) {
				// Failed to lock it. This means another thread is executing
				// this CPU. Don't increment the done count because there's
				// an invariant that thread count <= cpu count, so we're
				// guaranteed to be able to grab one eventually if we just keep
				// looping.

				continue;
			}

			while (proc->Timeslice != 0) {
				if (RTCIntervalMS && proc->TimerInterruptCounter >= RTCIntervalMS) {
					// Interval timer ran down, send self the interrupt.
					// We do this from the context of each cpu thread so that we get
					// accurate amounts of CPU time between each tick.

					LsicInterruptTargeted(proc, 2);

					proc->TimerInterruptCounter = 0;
				}

				if (proc->Id == 0) {
					// The zeroth thread also does the RTC intervals, once per
					// millisecond of CPU time. 

					RTCUpdateRealTime();
				}

				int realcycles = XrExecute(proc, cyclesperms + proc->PauseReward, 1);

				if (realcycles > cyclesperms) {
					// Used some of the reward cycles, decrement them.

					proc->PauseReward -= realcycles - cyclesperms;
				}

				proc->Timeslice -= 1;
				proc->TimerInterruptCounter += 1;

				if (proc->PauseCalls >= XR_PAUSE_MAX || proc->Halted) {
					// Halted or paused. Advance to next CPU.

					if (proc->PauseCalls >= XR_PAUSE_MAX) {
						if (realcycles < cyclesperms) {
							// The CPU voluntarily paused. Reward it with extra
							// cycles for next time.

							proc->PauseReward += cyclesperms - realcycles;

							if (proc->PauseReward >= pausemargin) {
								// Never reward with more than 2ms worth of cycles.

								proc->PauseReward = pausemargin;
							}
						}
					}

					proc->PauseCalls = 0;
					break;
				}
			}

			if (proc->Timeslice == 0) {
				done++;
			}

			SDL_UnlockMutex(proc->RunLock);
		}
	}
}

void CpuInitialize(int id) {
	XrProcessor *proc = malloc(sizeof(XrProcessor));

	if (!proc) {
		fprintf(stderr, "failed to allocate cpu %d", id);
		exit(1);
	}

	CpuTable[id] = proc;
	proc->Id = id;
	proc->TimerInterruptCounter = 0;
	proc->PauseReward = 0;
	proc->Timeslice = 0;

	XrReset(proc);

#ifndef EMSCRIPTEN
	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		proc->CacheMutexes[i] = SDL_CreateMutex();

		if (!proc->CacheMutexes[i]) {
			fprintf(stderr, "Failed to create cache mutex\n");
			exit(1);
		}
	}

	for (int i = 0; i < XR_CACHE_MUTEXES; i++) {
		ScacheMutexes[i] = SDL_CreateMutex();

		if (!ScacheMutexes[i]) {
			fprintf(stderr, "Failed to create Scache mutex\n");
			exit(1);
		}
	}

	proc->LoopSemaphore = SDL_CreateSemaphore(0);

	if (!proc->LoopSemaphore) {
		fprintf(stderr, "Unable to allocate loop semaphore: %s", SDL_GetError());
		exit(1);
	}

	proc->InterruptLock = SDL_CreateMutex();

	if (!proc->InterruptLock) {
		fprintf(stderr, "Failed to create interrupt mutex\n");
		exit(1);
	}

	proc->RunLock = SDL_CreateMutex();

	if (!proc->RunLock) {
		fprintf(stderr, "Failed to create run mutex\n");
		exit(1);
	}
#endif
}

void CpuCreate(uintptr_t id) {
#ifndef EMSCRIPTEN
	SDL_CreateThread(&CpuLoop, "CpuLoop", (void *)id);
#endif
}

extern void TLBDump(void);
extern void DbgInit(void);

int main(int argc, char *argv[]) {
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitor");
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // 0: point, 1 = linear

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_EnableScreenSaver();

	uint32_t memsize = 4 * 1024 * 1024;

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

		} else if (strcmp(argv[i], "-threads") == 0) {
			if (i+1 < argc) {
				CpuThreadCount = atoi(argv[i+1]);
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

	DbgInit();

	ScreenInit();
	ScreenDraw();

	done = false;

	for (int i = 0; i < XrProcessorCount; i++) {
		CpuInitialize(i);
	}

	if (CpuThreadCount > XrProcessorCount || CpuThreadCount == 0) {
		CpuThreadCount = (XrProcessorCount + 1) / 2;
	}

	for (int i = 0; i < CpuThreadCount; i++) {
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

		DKSInterval(tick_after_draw - tick_end);
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

		for (int i = 0; i < XrProcessorCount; i++) {
			SDL_LockMutex(CpuTable[i]->RunLock);

			CpuTable[i]->Timeslice = CPUSTEPMS;
			CpuTable[i]->Progress = XR_POLL_MAX;
			CpuTable[i]->PauseCalls = 0;

			SDL_UnlockMutex(CpuTable[i]->RunLock);
		}

		for (int i = 0; i < CpuThreadCount; i++) {
			SDL_SemPost(CpuTable[i]->LoopSemaphore);
		}

		tick_end = SDL_GetTicks();
		int delay = 1000/FPS - (tick_end - tick_start);

		if (delay > 0) {
			SDL_Delay(delay);
		}
	}
#else
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
	// Dumbed down CPU driving loop for emscripten since it doesn't seem to play
	// well with SDL's thread APIs.

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
		if (RTCIntervalMS && proc->TimerInterruptCounter >= RTCIntervalMS) {
			// Interval timer ran down, send self the interrupt.
			LsicInterruptTargeted(proc, 2);

			proc->TimerInterruptCounter = 0;
		}

		int cyclesleft = cyclespertick;

		if (i == dt-1)
			cyclesleft += extracycles;

		XrExecute(proc, cyclesleft, 1);

		// For emscripten, the interval functions are also responsible for
		// driving async device activity.

		DKSInterval(1);
		SerialInterval(1);

		RTCUpdateRealTime();

		proc->TimerInterruptCounter += 1;
	}
}

#endif