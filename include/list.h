/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * List head file
 *
 * Author(s): wxlin <linweixuangz@126.com>
 *
 * $Id: list.h,v 1.1 2008-10-08 09:33:00 wxlin Exp $
 */
 
#ifndef _LIST_H_
#define _LIST_H_

#include "log.h"

#ifndef WIN32

#define LIST_INLINE inline

#define OFFSET_OF(type, member) ((size_t) &((type *)0)->member)
#define container_of(ptr, type, member) ({                \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
	(type *)( (char *)__mptr - OFFSET_OF(type,member) );})

static LIST_INLINE void prefetch(const void *x) {;}

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_NEW() (struct list_head*)malloc(sizeof(struct list_head))

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)


static LIST_INLINE void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static LIST_INLINE void __list_add(struct list_head *newi,struct list_head *prev,struct list_head *next)
{
	next->prev = newi;
	newi->next = next;
	newi->prev = prev;
	prev->next = newi;
}

static LIST_INLINE void list_add(struct list_head *newi, struct list_head *head)
{
	__list_add(newi, head, head->next);
}

static LIST_INLINE void list_add_tail(struct list_head *newi, struct list_head *head)
{
	__list_add(newi, head->prev, head);
}

static LIST_INLINE void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static LIST_INLINE void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static LIST_INLINE void list_replace(struct list_head *old,struct list_head *newi)
{
	newi->next = old->next;
	newi->next->prev = newi;
	newi->prev = old->prev;
	newi->prev->next = newi;
}

static LIST_INLINE void list_replace_init(struct list_head *old,struct list_head *newi)
{
	list_replace(old, newi);
	INIT_LIST_HEAD(old);
}

static LIST_INLINE void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static LIST_INLINE void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static LIST_INLINE void list_move_tail(struct list_head *list,struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static LIST_INLINE int list_is_last(const struct list_head *list,
							   const struct list_head *head)
{
	return list->next == head;
}

static LIST_INLINE int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static LIST_INLINE int list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

static LIST_INLINE void __list_splice(struct list_head *list,struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

static LIST_INLINE void list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head);
}

static LIST_INLINE void list_splice_init(struct list_head *list,struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; prefetch(pos->next), pos != (head); \
	pos = pos->next)

#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	prefetch(pos->member.next), &pos->member != (head); 	\
	pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	prefetch(pos->member.prev), &pos->member != (head); 	\
	pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))

#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member);	\
	prefetch(pos->member.next), &pos->member != (head);	\
	pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_from(pos, head, member) 			\
	for (; prefetch(pos->member.next), &pos->member != (head);	\
	pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
	n = list_entry(pos->member.next, typeof(*pos), member);	\
	&pos->member != (head); 					\
	pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member), 		\
	n = list_entry(pos->member.next, typeof(*pos), member);		\
	&pos->member != (head);						\
	pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = list_entry(pos->member.next, typeof(*pos), member);		\
	&pos->member != (head);						\
	pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
	n = list_entry(pos->member.prev, typeof(*pos), member);	\
	&pos->member != (head); 					\
	pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#define list_for_each_free(head) \
	{struct list_head *pos,*n; \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	pos = n, n = pos->next){ list_del(pos); free(pos);} \
	free(head);}

#endif
#endif /*_LIST_H_*/

