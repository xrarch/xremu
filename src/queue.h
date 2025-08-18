#ifndef XR_QUEUE_H
#define	XR_QUEUE_H

#include "xrdefs.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct _ListEntry ListEntry;

struct _ListEntry {
	ListEntry *Next;
	ListEntry *Prev;
};

static XR_ALWAYS_INLINE void InitializeList(ListEntry *head) {
	head->Next = head;
	head->Prev = head;
}

static XR_ALWAYS_INLINE bool EmptyList(ListEntry *head) {
	return head->Next == head;
}

static XR_ALWAYS_INLINE void InsertAtTailList(ListEntry *head, ListEntry *entry) {
	ListEntry *last = head->Prev;

	entry->Prev = last;
	entry->Next = head;
	last->Next = entry;
	head->Prev = entry;
}

static XR_ALWAYS_INLINE void InsertAtHeadList(ListEntry *head, ListEntry *entry) {
	ListEntry *first = head->Next;

	entry->Next = first;
	entry->Prev = head;
	first->Prev = entry;
	head->Next = entry;
}

static XR_ALWAYS_INLINE void RemoveEntryList(ListEntry *entry) {
	ListEntry *prev = entry->Prev;
	ListEntry *next = entry->Next;

	prev->Next = next;
	next->Prev = prev;
}

#define ContainerOf(address, type, field) \
	((type *)((size_t)(address) - offsetof(type, field)))

#endif // XR_QUEUE_H
