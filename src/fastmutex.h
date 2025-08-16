#ifndef XR_MUTEX_H
#define XR_MUTEX_H

#include <stdatomic.h>
#include <errno.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

typedef struct _XrSemaphore {
#ifdef __APPLE__
    dispatch_semaphore_t    Semaphore;
#else
    sem_t                   Semaphore;
#endif
} XrSemaphore;


static inline void XrInitializeSemaphore(XrSemaphore *s, uint32_t value) {
#ifdef __APPLE__
    s->Semaphore = dispatch_semaphore_create(value);
#else
    sem_init(&s->Semaphore, 0, value);
#endif
}

static inline void XrWaitSemaphore(XrSemaphore *s) {
#ifdef __APPLE__
    dispatch_semaphore_wait(s->Semaphore, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
            r = sem_wait(&s->Semaphore);
    } while (r == -1 && errno == EINTR);
#endif
}

static inline void XrPostSemaphore(XrSemaphore *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->Semaphore);
#else
    sem_post(&s->Semaphore);
#endif
}

static inline void XrUninitializeSemaphore(XrSemaphore *s) {
#ifndef __APPLE__
	sem_destroy(&s->Semaphore);
#endif
}

typedef struct _XrMutex {
	XrSemaphore Semaphore;
	_Atomic int ContentionCounter;
} XrMutex;

static inline void XrInitializeMutex(XrMutex *mutex) {
	mutex->ContentionCounter = 0;
	XrInitializeSemaphore(&mutex->Semaphore, 0);
}

static inline void XrUninitializeMutex(XrMutex *mutex) {
	XrUninitializeSemaphore(&mutex->Semaphore);
}

static inline void XrLockMutex(XrMutex *mutex) {
	if (atomic_fetch_add_explicit(&mutex->ContentionCounter, 1, memory_order_acquire) != 0) {
		// Wait on the semaphore.

		XrWaitSemaphore(&mutex->Semaphore);
	}
}

static inline void XrUnlockMutex(XrMutex *mutex) {
	if (atomic_fetch_sub_explicit(&mutex->ContentionCounter, 1, memory_order_release) != 1) {
		// There are waiters, so post the semaphore to allow one in.

		XrPostSemaphore(&mutex->Semaphore);
	}
}

static inline int XrTryLockMutex(XrMutex *mutex) {
	while (1) {
		int counter = mutex->ContentionCounter;

		if (counter != 0) {
			return 0;
		}

		if (atomic_compare_exchange_strong_explicit(
			&mutex->ContentionCounter,
			&counter,
			1,
			memory_order_acquire,
			memory_order_relaxed
		)) {
			return 1;
		}
	}
}

#endif