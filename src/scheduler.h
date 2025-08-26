#ifndef XR_SCHEDULER_H
#define XR_SCHEDULER_H

#include "xrdefs.h"
#include "fastmutex.h"
#include "queue.h"

#define XR_SCHEDULABLE_MAX (XR_PROC_MAX + 16)

extern int XrSchedulableCount;
extern int XrSchedulableProcessorIndex;

typedef struct _XrSchedulable XrSchedulable;

extern XrSchedulable *XrSchedulableTable[XR_SCHEDULABLE_MAX];

typedef void (*XrSchedulableF)(XrSchedulable *schedulable);
typedef void (*XrStartTimesliceF)(XrSchedulable *schedulable, int dt);

struct _XrSchedulable {
#ifndef SINGLE_THREAD_MP
	XrMutex *RunLock;
	XrMutex InherentRunLock;
	XrSchedulable *Next;
#endif
	XrSchedulableF Func;
	XrStartTimesliceF StartTimeslice;
	void *Context;
	int Timeslice;
};

static inline void XrInitializeSchedulable(XrSchedulable *schedulable, XrSchedulableF func, XrStartTimesliceF starttimeslice, void *context) {
	XrInitializeMutex(&schedulable->InherentRunLock);
	schedulable->Timeslice = 0;
	schedulable->Func = func;
	schedulable->StartTimeslice = starttimeslice;
	schedulable->Context = context;
	schedulable->RunLock = &schedulable->InherentRunLock;
	schedulable->Next = 0;

	XrSchedulableTable[XrSchedulableCount++] = schedulable;
}

extern void XrInitializeScheduler(int threads);

extern void XrScheduleItems(int dt);

#endif // XR_SCHEDULER_H