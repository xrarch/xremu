#ifndef _QUEUE_H_
#define	_QUEUE_H_

#include <stddef.h>

typedef struct _ListEntry ListEntry;

struct _ListEntry {
	ListEntry *Next;
	ListEntry *Prev;
};

static inline void InitializeList(ListEntry *head) {
	head->Next = head;
	head->Prev = head;
}

static inline bool EmptyList(ListEntry *head) {
	return head->Next == head;
}

static inline void InsertAtTailList(ListEntry *head, ListEntry *entry) {
	ListEntry *last = head->Prev;

	entry->Prev = last;
	entry->Next = head;
	last->Next = entry;
	head->Prev = entry;
}

static inline void InsertAtHeadList(ListEntry *head, ListEntry *entry) {
	ListEntry *first = head->Next;

	entry->Next = first;
	entry->Prev = head;
	first->Prev = entry;
	head->Next = entry;
}

static inline void RemoveEntryList(ListEntry *entry) {
	ListEntry *prev = entry->Prev;
	ListEntry *next = entry->Next;

	prev->Next = next;
	next->Prev = prev;
}

#define ContainerOf(address, type, field) \
	((type *)((size_t)(address) - offsetof(type, field)))

#endif /* !_QUEUE_H_ */
