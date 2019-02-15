/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* timer heap functions
*
* Author(s): wxlin  <weixuan.lin@sierraatlantic.com>
*
* $Id:heap.h,v 1.4 2008-12-30 08:14:55 wxlin Exp $
*
*/

#ifndef _HEAP_H_
#define _HEAP_H_

/*
used timer struct define

typedef struct timer_entry{
	int heap_id;
	struct timeval expire;
} timer_entry;

typedef struct timer_queue{
    struct timer_entry** timers;
    unsigned int count,
	unsigned int size;
} timer_queue;
*/

static inline void heap_elem_init(struct timer_entry* e);
static inline int heap_elem_greater(struct timer_entry *timer, struct timer_entry *b);
static inline int heap_empty(struct timer_queue* tq);
static inline unsigned heap_size(struct timer_queue* tq);
static inline struct timer_entry*  heap_top(struct timer_queue* tq);
static inline int heap_reserve(struct timer_queue* tq, unsigned count);
static inline int heap_push(struct timer_queue* tq, struct timer_entry* e);
static inline struct timer_entry* heap_pop(struct timer_queue* tq);
static inline int heap_erase(struct timer_queue* tq, struct timer_entry* e);
static inline void heap_shift_up_(struct timer_queue* tq, unsigned hole_index, struct timer_entry* e);
static inline void heap_shift_down_(struct timer_queue* tq, unsigned hole_index, struct timer_entry* e);

static inline int 
heap_elem_greater(struct timer_entry *a, struct timer_entry *b)
{
    return tv_gt(&a->expire, &b->expire);
}

static inline void 
heap_elem_init(struct timer_entry* e) 
{ 
	e->heap_id = -1; 
}

static inline int 
heap_empty(struct timer_queue* tq) 
{ 
	return 0u == tq->count; 
}

static inline unsigned 
heap_size(struct timer_queue* tq) 
{ 
	return tq->count; 
}

static inline struct timer_entry* 
heap_top(struct timer_queue* tq) 
{ 
	return tq->count ? *tq->timers : 0; 
}

static inline int 
heap_push(struct timer_queue* tq, struct timer_entry* e)
{
    if(heap_reserve(tq, tq->count + 1))
        return -1;
    heap_shift_up_(tq, tq->count++, e);
    return 0;
}

static inline struct timer_entry* 
heap_pop(struct timer_queue* tq)
{
    if(tq->count)
    {
        struct timer_entry* e = *tq->timers;
        heap_shift_down_(tq, 0u, tq->timers[--tq->count]);
        e->heap_id = -1;
        return e;
    }
    return 0;
}

static inline int 
heap_erase(struct timer_queue* tq, struct timer_entry* e)
{
    if(((unsigned int)-1) != e->heap_id)
    {
        heap_shift_down_(tq, e->heap_id, tq->timers[--tq->count]);
        e->heap_id = -1;
        return 0;
    }
    return -1;
}

static inline int 
heap_reserve(struct timer_queue* tq, unsigned count)
{
    if(tq->size < count)
    {
        struct timer_entry** p;
        unsigned size = tq->size ? tq->size * 2 : 8;
        if(size < count)
            size = count;
        if(!(p = (struct timer_entry**)realloc(tq->timers, size * sizeof *p)))
            return -1;
        tq->timers = p;
        tq->size = size;
    }
    return 0;
}

static inline void 
heap_shift_up_(struct timer_queue* tq, unsigned hole_index, struct timer_entry* e)
{
    unsigned parent = (hole_index - 1) / 2;
    while(hole_index && heap_elem_greater(tq->timers[parent], e))
    {
        (tq->timers[hole_index] = tq->timers[parent])->heap_id = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (tq->timers[hole_index] = e)->heap_id = hole_index;
}

static inline void 
heap_shift_down_(struct timer_queue* tq, unsigned hole_index, struct timer_entry* e)
{
    unsigned child = 2 * (hole_index + 1);
    while(child <= tq->count)
	{
        child -= child == tq->count || heap_elem_greater(tq->timers[child], tq->timers[child - 1]);
        if(!(heap_elem_greater(e, tq->timers[child])))
            break;
        (tq->timers[hole_index] = tq->timers[child])->heap_id = hole_index;
        hole_index = child;
        child = 2 * (hole_index + 1);
	}
    heap_shift_up_(tq, hole_index,  e);
}

#endif /* _HEAP_H_ */
