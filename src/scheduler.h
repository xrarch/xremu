#ifndef XR_SCHEDULER_H
#define XR_SCHEDULER_H

#include "xrdefs.h"
#include "fastmutex.h"
#include "queue.h"

#define XR_DEVICE_QUEUE_SLOTS 10

typedef struct _XrDeviceQueue XrDeviceQueue;
typedef struct _XrDeviceQueueEntry XrDeviceQueueEntry;

typedef void (*XrExpireF)(XrDeviceQueue *queue, void *context);

struct _XrDeviceQueueEntry {
	XrExpireF Func;
	void *Context;
	uint64_t Expiry;
};

struct _XrDeviceQueue {
	XrDeviceQueueEntry Slots[XR_DEVICE_QUEUE_SLOTS];
	uint64_t Ticks;
	int EntryCount;
};

typedef struct _XrSchedulable XrSchedulable;

typedef void (*XrSchedulableF)(XrSchedulable *schedulable);
typedef void (*XrStartTimesliceF)(XrSchedulable *schedulable, int dt);

typedef struct _XrSchedulingThread XrSchedulingThread;

struct _XrSchedulable {
	ListEntry WorkEntry;
	XrSchedulable *Next;
	XrSchedulingThread *PreferredThread;
	XrSchedulableF Func;
	XrStartTimesliceF StartTimeslice;
	void *Context;
	int Timeslice;
	int Enqueued;
};

static inline void XrInitializeSchedulable(XrSchedulable *schedulable, XrSchedulableF func, XrStartTimesliceF starttimeslice, void *context) {
	schedulable->Timeslice = 0;
	schedulable->Func = func;
	schedulable->StartTimeslice = starttimeslice;
	schedulable->Context = context;
	schedulable->Next = 0;
	schedulable->PreferredThread = 0;
	schedulable->Enqueued = 0;
}

extern void XrInitializeScheduler(int threads);

extern void XrScheduleAllNextFrameWork(int dt);

extern void XrScheduleWorkForAny(XrSchedulable *work);

extern void XrScheduleWorkForNextFrame(XrSchedulable *work, int front);

extern void XrScheduleWorkForMe(XrSchedulable *after, XrSchedulable *work);

extern void XrStartScheduler(void);

extern void *XrSchedulerLoop(void *context);

static inline void XrInitializeDeviceQueue(XrDeviceQueue *queue) {
	queue->Ticks = 0;
	queue->EntryCount = 0;

	for (int i = 0; i < XR_DEVICE_QUEUE_SLOTS; i++) {
		queue->Slots[i].Func = NULL;
	}
}

static inline int XrTickDeviceQueue(XrDeviceQueue *queue, int ticks) {
	queue->Ticks += ticks;

	if (!queue->EntryCount) {
		return 0;
	}

	XrDeviceQueueEntry expired[XR_DEVICE_QUEUE_SLOTS];
	int expiredcount = 0;

	for (int i = 0; i < XR_DEVICE_QUEUE_SLOTS; i++) {
		// There are so few slots (ten) that we just scan an array to
		// find any expired entries.

		XrDeviceQueueEntry *entry = &queue->Slots[i];

		if ((entry->Func != NULL) && (queue->Ticks >= entry->Expiry)) {
			// Have to expire this entry.

			// Copy it to the deferred entry array.

			XrDeviceQueueEntry *copy = &expired[expiredcount++];
			copy->Func = entry->Func;
			copy->Context = entry->Context;

			// Mark entry free.

			entry->Func = NULL;
			queue->EntryCount--;
		}
	}

	// Defer calling the functions until after we've scanned all the slots.

	for (int i = 0; i < expiredcount; i++) {
		expired[i].Func(queue, expired[i].Context);
	}

	return expiredcount != 0;
}

static inline void XrInsertDeviceQueue(XrDeviceQueue *queue, void *context, XrExpireF func, int ms) {
	// Insert an entry into the device queue for later expiration.

	for (int i = 0; i < XR_DEVICE_QUEUE_SLOTS; i++) {
		if (queue->Slots[i].Func == NULL) {
			// Use this entry.

			queue->Slots[i].Func = func;
			queue->Slots[i].Context = context;
			queue->Slots[i].Expiry = queue->Ticks + ms;
			queue->EntryCount++;

			return;
		}
	}

	// Shouldn't be reachable since we size device queues to fit the maximum
	// number of XR/station devices that use them at once.

	abort();
}

#endif // XR_SCHEDULER_H