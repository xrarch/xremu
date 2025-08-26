#include "scheduler.h"
#include "xr.h"
#include "pthread.h"
#include <stdio.h>

ListEntry XrSchedulerWorkList;
XrMutex XrSchedulerWorkListMutex;

ListEntry XrSchedulerNextFrameList;
XrMutex XrSchedulerNextFrameListMutex;

XrSemaphore XrSchedulerSemaphore;

int XrSchedulingThreadCount = 0;

typedef struct _XrSchedulingThread {
	pthread_t Pthread;
} XrSchedulingThread;

// There can't be more scheduling threads than processors so that should be the
// maximum size of the table.

XrSchedulingThread XrSchedulingThreadTable[XR_PROC_MAX];

static inline XrSchedulable *XrPopSchedulerWork(XrSchedulingThread *thread) {
	while (1) {
		// Wait until there is work.

		XrWaitSemaphore(&XrSchedulerSemaphore);

		XrLockMutex(&XrSchedulerWorkListMutex);

		ListEntry *listentry = XrSchedulerWorkList.Next;

		if (listentry == &XrSchedulerWorkList) {
			// It's empty, so nevermind.

			XrUnlockMutex(&XrSchedulerWorkListMutex);

			continue;
		}

		XrSchedulable *work = ContainerOf(listentry, XrSchedulable, WorkEntry);

		RemoveEntryList(listentry);

		XrUnlockMutex(&XrSchedulerWorkListMutex);

		return work;
	}
}

void XrScheduleWork(XrSchedulable *work) {
	XrLockMutex(&XrSchedulerWorkListMutex);

	InsertAtTailList(&XrSchedulerWorkList, &work->WorkEntry);

	XrUnlockMutex(&XrSchedulerWorkListMutex);

	XrPostSemaphore(&XrSchedulerSemaphore);
}

void XrScheduleWorkForNextFrame(XrSchedulable *work, int front) {
	XrLockMutex(&XrSchedulerNextFrameListMutex);

	if (front) {
		InsertAtHeadList(&XrSchedulerNextFrameList, &work->WorkEntry);
	} else {
		InsertAtTailList(&XrSchedulerNextFrameList, &work->WorkEntry);
	}

	XrUnlockMutex(&XrSchedulerNextFrameListMutex);
}

void XrScheduleWorkBorrow(XrSchedulable *after, XrSchedulable *work) {
	work->Next = after->Next;
	after->Next = work;
}

void *XrSchedulerLoop(void *context) {
	uintptr_t id = (uintptr_t)context;

	// Execute CPU time from each CPU in sequence, until all timeslices have run
	// down.

	XrSchedulingThread *thread = &XrSchedulingThreadTable[id];

	XrSchedulable *work = 0;

	while (1) {
		if (!work) {
			work = XrPopSchedulerWork(thread);
		}

		work->Func(work);

		XrSchedulable *next = work->Next;
		work->Next = 0;
		work = next;
	}
}

void XrInitializeScheduler(int threads) {
	InitializeList(&XrSchedulerWorkList);
	InitializeList(&XrSchedulerNextFrameList);

	XrInitializeSemaphore(&XrSchedulerSemaphore, 0);

	XrInitializeMutex(&XrSchedulerWorkListMutex);
	XrInitializeMutex(&XrSchedulerNextFrameListMutex);

	XrSchedulingThreadCount = threads;
}

void XrStartScheduler(void) {
	for (uintptr_t id = 0; id < XrSchedulingThreadCount; id++) {
		XrSchedulingThread *thread = &XrSchedulingThreadTable[id];

		pthread_create(&thread->Pthread, NULL, &XrSchedulerLoop, (void *)id);
	}
}

void XrScheduleAllNextFrameWork(int dt) {
	// Put all per-frame work on the work list.

	ListEntry list;
	InitializeList(&list);

	// Move the next frame list to our local list and empty it.

	XrLockMutex(&XrSchedulerNextFrameListMutex);

	if (XrSchedulerNextFrameList.Next != &XrSchedulerNextFrameList) {
		list.Next = XrSchedulerNextFrameList.Next;
		list.Prev = XrSchedulerNextFrameList.Prev;

		list.Next->Prev = &list;
		list.Prev->Next = &list;

		InitializeList(&XrSchedulerNextFrameList);
	}

	XrUnlockMutex(&XrSchedulerNextFrameListMutex);

	// Call the start timeslice callback.

	ListEntry *listentry = list.Next;

	while (listentry != &list) {
		XrSchedulable *work = ContainerOf(listentry, XrSchedulable, WorkEntry);

		work->StartTimeslice(work, dt);

		listentry = listentry->Next;
	}

	// Now enqueue.

	listentry = list.Next;

	while (listentry != &list) {
		ListEntry *next = listentry->Next;

		XrSchedulable *work = ContainerOf(listentry, XrSchedulable, WorkEntry);

		XrScheduleWork(work);

		listentry = next;
	}
}