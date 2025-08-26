#include "scheduler.h"
#include "xr.h"
#include "pthread.h"

int XrSchedulableCount = 0;
int XrSchedulableProcessorIndex = 0;

XrSchedulable *XrSchedulableTable[XR_SCHEDULABLE_MAX];

int XrSchedulingThreadCount = 0;

typedef struct _XrSchedulingThread {
	pthread_t Pthread;
	XrSemaphore LoopSemaphore;
} XrSchedulingThread;

// There can't be more scheduling threads than processors so that should be the
// maximum size of the table.

XrSchedulingThread XrSchedulingThreadTable[XR_PROC_MAX];

void *XrSchedulerLoop(void *context) {
	uintptr_t id = (uintptr_t)context;

	// Execute CPU time from each CPU in sequence, until all timeslices have run
	// down.

	XrSchedulingThread *thread = &XrSchedulingThreadTable[id];

	int startschedid = XrSchedulableProcessorIndex + (id * (XrProcessorCount / XrSchedulingThreadCount));

	while (1) {
		// Wait until the next frame.

		XrWaitSemaphore(&thread->LoopSemaphore);

		// Start the processor ID at the preferred ID. This will make it so the
		// same threads preferentially execute the same couple of processors,
		// which will improve cache affinity.

		int schedid = startschedid;

		// Iterate over the CPUs in a cycle until all of the virtual CPU time
		// for this frame, of which each CPU gets ~17ms worth, has been
		// executed. For example, at 20MHz each CPU will get 20MHz * 17ms =
		// 340,000 simulated cycles each frame. The CPU execution is aligned to
		// frames in order to give meaning to the vblank interrupt and make user
		// input feel smooth.
		//
		// Multiple threads may execute this work loop simultaneously, to
		// distribute guest workload over host cores (the default, as of
		// writing, is half as many host threads as guest CPUs).
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

		for (int done = 0; done < XrSchedulableCount; schedid = (schedid + 1) % XrSchedulableCount) {
			XrSchedulable *schedulable = XrSchedulableTable[schedid];

			// Trylock the CPU's run lock. Cache it first because it might
			// change.

retry:;

			XrMutex *runlock = schedulable->RunLock;

			if (!XrTryLockMutex(runlock)) {
				// Failed to lock it. This means another thread is executing
				// this CPU. Don't increment the done count because there's
				// an invariant that thread count <= cpu count, so we're
				// guaranteed to be able to grab one eventually if we just keep
				// looping.

				continue;
			}

			if (schedulable->Timeslice <= 0) {
				XrUnlockMutex(runlock);
				done++;
				continue;
			}

directnext:

			done = 0;

			if (runlock != &schedulable->InherentRunLock) {
				XrLockMutex(&schedulable->InherentRunLock);

				if (runlock != schedulable->RunLock) {
					// The runlock changed, so try again.

					XrUnlockMutex(&schedulable->InherentRunLock);
					XrUnlockMutex(runlock);

					goto retry;
				}
			}

			schedulable->Func(schedulable);

			if (runlock != &schedulable->InherentRunLock) {
				XrUnlockMutex(&schedulable->InherentRunLock);
			}

			if (schedulable->Timeslice <= 0) {
				done++;
			}

			if (schedulable->Next) {
				// There's a preferred next schedulable to run.
				// We can do this without releasing the current runlock because
				// its runlock has been set to the same as ours. This is useful
				// for atomically scheduling a processor together with a disk
				// that it is waiting for, which greatly improves the latency
				// simulation when there are multiple scheduling threads.

				XrSchedulable *next = schedulable->Next;
				schedulable->Next = 0;

				if (runlock == next->RunLock && next->Timeslice > 0) {
					schedulable = next;
					goto directnext;
				}
			}

			XrUnlockMutex(runlock);
		}
	}
}

void XrInitializeScheduler(int threads) {
	XrSchedulingThreadCount = threads;

	for (uintptr_t id = 0; id < threads; id++) {
		XrSchedulingThread *thread = &XrSchedulingThreadTable[id];

		XrInitializeSemaphore(&thread->LoopSemaphore, 0);

		pthread_create(&thread->Pthread, NULL, &XrSchedulerLoop, (void *)id);
	}
}

void XrScheduleItems(int dt) {
	for (int i = 0; i < XrSchedulableCount; i++) {
		XrSchedulableTable[i]->StartTimeslice(XrSchedulableTable[i], dt);
	}

	for (int i = 0; i < XrSchedulingThreadCount; i++) {
		XrPostSemaphore(&XrSchedulingThreadTable[i].LoopSemaphore);
	}
}