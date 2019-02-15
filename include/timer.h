/**
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* timer queue head file
*
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
*
* $Id: timer.h,v 1.4 2008-12-30 08:14:55 wxlin Exp $
*
*/

#ifndef _TIMER_H_
#define _TIMER_H_

#include "list.h"
#include "hashmap.h"
#include "timeval.h"

#define  MAX_TIMER_QUEUE 64
#define  MAX_CALLBACK_QUEUE 4000

struct Mempool;
struct timer_entry;
struct timer_queue;

typedef int (*timer_callback_fn)(struct timer_entry* timer);
typedef int (*timer_callback_fn1)(void *param);
typedef int (*timer_callback_func)(unsigned int tid, void *param);
typedef void (*timer_param_free_func)(void *ptr,int flag);

enum TIMER_STATUS{
    TIMER_INIT,    
    TIMER_WATTING,
    TIMER_SCHEDULING,
    TIMER_EXPIRED,
    TIMER_ERROR,
    TIMER_INTERRUPT,
    TIMER_CANCLED,
    TIMER_CBWAITING,
    TIMER_CBDOING
};

enum TIMER_FLAGS{
    TIMER_ONCE		= 0x000,
    TIMER_REPEAT    = 0x002,
    TIMER_USEFLAG   = 0x004,
    TIMER_USETID	= 0x008,
    TIMER_USEPARAM  = 0x010,
    TIMER_USE16BID  = 0x020,
    TIMER_CUSTOM    = 0x040,
    TIMER_NEXT      = 0x080,
    TIMER_STARTAT   = 0x100,
    TIMER_USEC      = 0x200,
    TIMER_FIRSTRUN  = 0x400,
    TIMER_DELAYINC  = 0x800,
    TIMER_STOP      = 0x1000,
    TIMER_DELAY     = 0x2000
};

struct timer_entry {
    struct list_head node;
    void *param;
    unsigned short flags;
    int heap_id;
    unsigned int timer_id;
    unsigned short status;
    unsigned short repeat;
    struct timeval start;
    struct timeval setup;
    struct timeval expire;
    timer_callback_func callback;
    timer_param_free_func free;
};

struct callback_queue{
    struct list_head *list;
    volatile int flags;
    volatile int running;
    unsigned long max;
    volatile unsigned int count;
	unsigned int full_waiters;
	unsigned int empty_waiters;
    pthread_mutex_t lock;
    pthread_cond_t empty;
	pthread_cond_t full;
    pthread_t thread;
    struct timer_queue* owner;
};

struct timer_queue {
    unsigned int flags;
    unsigned int state;
    struct timer_entry **timers;
    struct callback_queue* cblist;
    volatile unsigned long ids;
    volatile unsigned int count;
    volatile unsigned int size;
    volatile int running;
	unsigned int full_waiters;
	unsigned int empty_waiters;
    unsigned long max;
    Hashmap*  map;
    struct Mempool* tpool;
    pthread_mutex_t lock;
    pthread_cond_t empty;
	pthread_cond_t full;
    pthread_cond_t cond;
    pthread_t scheduler;
    void* owner;
};

typedef struct timer_entry TimerEntry;
typedef struct callback_queue CallbackQueue;
typedef struct timer_queue TimerQueue;

extern TimerQueue* timer_queue_create(int flags);
extern int timer_queue_init(TimerQueue* tqueue);
extern int timer_queue_start(TimerQueue* tqueue);
extern int timer_queue_stop(TimerQueue* tqueue);
extern void timer_queue_destroy(TimerQueue* tqueue);

extern struct timer_entry* new_timer(struct timer_queue* tq);
extern unsigned int add_timer(struct timer_queue* tq,struct timeval* expire,unsigned int flags,timer_callback_func func, void *param);
extern unsigned int push_timer(struct timer_queue* tq,struct timer_entry *timer);
extern int insert_timer(struct timer_queue* tq,struct timer_entry *timer);
extern int remove_timer(struct timer_queue* tqueue,struct timer_entry *timer);
extern int cancel_timer(struct timer_queue*tqueue,unsigned int tid);
extern long del_timer(struct timer_queue*tqueue,unsigned int tid,int flag);

#endif  /*_TIMER_H_*/
