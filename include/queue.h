/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Queue interfaces
 * 
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
 *
 * $Id: queue.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define Q_SUCCESS 	0
#define Q_STOP     	1
#define Q_EAGAIN 	EAGAIN
#define Q_EINTR 	EINTR

struct queue_t {
    unsigned int        flags; 		    /* queue init flags*/
    void              	**data;         /* queue element array*/
    int                 *fds;    	    /* file fd slot array*/
    unsigned int        count; 		    /* element counter*/
    unsigned int        in;    		    /* next empty location */
    unsigned int        out;   		    /* next filled location */
    unsigned int        size;		    /* max size of queue */
    unsigned int        full_waiters;   /* queue full waiter*/
    unsigned int        empty_waiters;  /* queue empty waiter*/
    pthread_mutex_t		qlock;          /* queue object lock*/
    pthread_cond_t  	not_empty;      /* queue full condition*/
    pthread_cond_t  	not_full;       /* queue empty condition*/
    int                 terminated;     /* queue exit flag*/
};

typedef struct queue_t Queue;

#ifdef __cplusplus
extern "C" {
#endif

#define queue_full(queue) ((queue)->count == (queue)->size)
#define queue_empty(queue) ((queue)->count == 0)

struct queue_t* queue_create(unsigned int capacity);
int queue_push(struct queue_t *queue, void *data);
int queue_pushwait(struct queue_t *queue, void *data,long sec);
int queue_pop(struct queue_t *queue, void **data);
int queue_trypush(struct queue_t *queue, void *data);
int queue_trypop(struct queue_t *queue, void **data);
unsigned int queue_size(struct queue_t *queue);
int queue_signal_all(struct queue_t *queue);
int queue_term(struct queue_t *queue);
int queue_destroy(struct queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* _QUEUE_H_ */
