#ifndef XR_SCHEDULER_H
#define XR_SCHEDULER_H

#include "xrdefs.h"
#include "fastmutex.h"
#include "queue.h"

typedef struct _XrSchedulable XrSchedulable;

typedef void (*XrSchedulableF)(XrSchedulable *schedulable);
typedef void (*XrStartTimesliceF)(XrSchedulable *schedulable, int dt);

struct _XrSchedulable {
	ListEntry WorkEntry;
#ifndef SINGLE_THREAD_MP
	XrSchedulable *Next;
	void *PreferredThread;
#endif
	XrSchedulableF Func;
	XrStartTimesliceF StartTimeslice;
	void *Context;
	int Timeslice;
};

static inline void XrInitializeSchedulable(XrSchedulable *schedulable, XrSchedulableF func, XrStartTimesliceF starttimeslice, void *context) {
	schedulable->Timeslice = 0;
	schedulable->Func = func;
	schedulable->StartTimeslice = starttimeslice;
	schedulable->Context = context;
	schedulable->Next = 0;
}

extern void XrInitializeScheduler(int threads);

extern void XrScheduleAllNextFrameWork(int dt);

extern void XrScheduleWork(XrSchedulable *work);

extern void XrScheduleWorkForNextFrame(XrSchedulable *work, int front);

extern void XrScheduleWorkBorrow(XrSchedulable *after, XrSchedulable *work);

extern void XrStartScheduler(void);

#endif // XR_SCHEDULER_H