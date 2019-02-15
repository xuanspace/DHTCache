/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * UDP Transport interfaces
 * 
* Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: udp.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */
#ifndef _UDP_H_
#define _UDP_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "host.h"
#include "hashmap.h"
#include "buffer.h"
#include "queue.h"
#include "priority.h"
enum UDP_FLAGS{
    UDP_DEFAULT,
    UDP_DEBUG,
    UDP_TRACE
};

enum UDP_STATE{
    UDP_INIT,                   /* deafault state when network up*/
    UDP_CREATE,                 /* udp socket alread created*/
    UDP_SYN_SENT,               /* initial sequence number sent */
    UDP_ESTABLISNED,            /* peer pair connection created */
    UDP_CLOSE                   /* peer udp have been closed*/
};

struct udp_peer {
    int flags;					/* flag the send & recv window*/
    int state;					/* status for peer udp host*/
    uint32 id;                  /* peer udp server id number*/
    uint16 iss;                 /* initial send sequence number*/
    uint16 ack;                 /* ack send sequence number*/
    uint16 wnd;            		/* sequence receive window size*/
    Hashmap *seq_map;			/* send sequence number map*/
    uint16 *seq_his;			/* sent sequence history vector*/
};

struct udp_server {
	int flags;					/* the udp server self flags */	
	int running;				/* udp server running flag */
	int sock;					/* udp server socket handle */
	struct Network* net;		/* transport network layer */
	Hashmap* ipmap;				/* peer udp server peer ip map */
    unsigned int ids;			/* identity udp peer sequence */
    struct queue_t* rqueue;     /* received buffer queue */
    pthread_t scheduler;        /* recv buffer schedule thread */
    pthread_mutex_t lock;       /* the whole udp server locker */
	void* owner;                /* the udp server owner pointer*/
};

struct udp_server* udp_create(int flags);
int udp_init(struct udp_server* srv);
int udp_start(struct udp_server* srv);
int udp_stop(struct udp_server* srv);
void udp_destroy(struct udp_server* srv);
int udp_input(struct Buffer* buf);
int udp_output(struct Buffer* buf);

#endif /* _TRANSORT_H_ */
