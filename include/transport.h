/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Transport interfaces
 * 
* Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: queue.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */
#ifndef _TRANSORT_H_
#define _TRANSORT_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "host.h"
#include "hashmap.h"
#include "buffer.h"
#include "priority.h"

#define T_SUCCESS 			0
#define T_FAIL     			1
#define T_EAGAIN 			EAGAIN
#define T_EINTR 			EINTR

#define HZ                  100
#define RTO_MAX	            ((unsigned)(120*HZ))
#define RTO_MIN	            ((unsigned)(HZ/5))
#define RTT_TIMEOUT         ((unsigned)(3*HZ))	/* RFC 1122 initial RTO value*/
#define SEQ_CACHE_SIZE      1000

#define MAX_SEND_QUEUE		64
#define MAX_RECV_QUEUE		2046
#define MAX_SEQ_HISTORY		1000

struct Network;
struct timer_queue;
struct send_queue;
struct recv_queue;

enum SEQ_STATUS{
    SEQ_LOST,
    SEQ_DUPLICATE,
    SEQ_ERROR
};

enum SEQ_FLAGS{
    SEQ_DEFAULT		= 0x0,
    SEQ_SEND        = 0x2,
    SEQ_RECV        = 0x4
};

enum SEQLOG_FLAGS{
    SEQLOG_SEND,   
    SEQLOG_ACK,    
    SEQLOG_RECV,    
    SEQLOG_UPCALL
};

enum BUF_FLAGS{
    BUF_DEFAULT,
    BUF_DELAYED
};

enum BE_STATE{
    BE_INIT,
    BE_CREATE,
    BE_SYN_SEND,
    BE_ESTABLISNED,
    BE_CLOSE,
    BE_DEAD,
    BE_LOST,
    BE_CONGEST
};

enum BE_CSTATE{
    UDP_NORMAL,
    UDP_LOST,
    UDP_CONGEST,
    UDP_RECOVERT
};

struct seq_entry {
    uint16 seqno;               /* sequence number from network*/
    struct timeval stamp;       /* sequence number timestamp*/
};

struct cwnd_info {
    int flags;					/* flag the send & recv window*/
    int state;					/* status for receive window*/
    uint32 id;                  /* receive congest window id */
    uint16 iss;                 /* initial send sequence number*/
    uint16 win_size;            /* sequence window size*/
    uint16 adv_size;            /* advertise window size*/
    long srtt;		            /* smoothed round trip time << 3*/
    long rto;		            /* retransmission time-out*/    
    uint16 mdev;		        /* medium deviation */
    uint16 mdev_max;	        /* maximal mdev for the last rtt period	*/
    uint16 rttvar;		        /* smoothed mdev_max */
    struct timeval rtt;         /* network round-trip time */
    time_t alive;				/* window alive timestamp*/
    uint32 drop;			    /* ack drop counter*/
    volatile uint32 hold;		/* hold sequence counter*/
    struct timeval stamp;       /* timestamp for window*/
    struct list_head* delay;	/* delayed send buffer list*/
    struct seq_entry *seq_vec;	/* sent sequence history vector*/
    struct seq_entry *seq_his;	/* recv sequence history vector*/
    struct timer_queue *tq;	    /* sequence timers queue*/
    pthread_mutex_t lock;       /* the window locker*/
};

struct send_ops{
    struct send_queue* (*create)(struct Network* net,int flags);
    int (*init)(struct send_queue* sq);
    int (*start)(struct send_queue* sq);
    int (*stop)(struct send_queue* sq);
    void (*destroy)(struct send_queue* sq);
    int (*priority)(Buffer* buf);
    int (*window)(struct send_queue* sq,uint32 ip);
    int (*output)(Buffer* buf);
    void* (*xmit)(void* param);
};

struct send_queue{
    unsigned int flags;         /* flag the send & recv queue*/
    int state;					/* status for send queue*/
    volatile int running;		/* send queue running flag */
    volatile unsigned long count;/* queue elements counter*/
    struct Network* net;		/* transport network layer*/
    int sock;                   /* transport socket handle*/
    struct timer_queue *tqueue;	/* sequence timers queue*/
    Hashmap* ipmap;				/* congest window ip map*/
    struct send_ops *ops;		/* queue operation call*/

    /* Congest control */
    uint32 snd_wnd;             /* send window size*/
    uint32 snd_ssthresh;        /* send window size threshold*/
    uint32 prior_ssthresh;      /* ssthresh saved at recovery start	*/
    uint32 snd_cwnd;			/* sending congestion window*/
    uint32 snd_cwnd_cnt;		/* linear increase counter*/
    uint32 snd_cwnd_clamp;		/* do not allow snd_cwnd to grow above this */
    uint32 snd_tick_nsec;
    uint32 max_ssthresh;        /* growth limited to max_ssthresh*/
    uint32 mss_cache;	        /* cached effective mss, not including SACKS */
    uint16 sequence;
    uint32 tick_count;
    uint32 tick_slice;
    uint32 tick;

    /* RTT measurement */
    uint32 packets_out;	        /* Packets which are "in flight*/
    uint32 left_out;	        /* Packets which leaved network	*/
    uint32 retrans_out;	        /* Retransmitted packets out*/

    /* delayed send variables*/
    struct list_head *delay_queue; /* delayed send buffer queue*/
    unsigned int delay_count;	/* delayed buffer count*/	
    unsigned int delay_limit;	/* max delayed limitation*/	
	unsigned long replicas;	    /* replica number*/
	long delay;					/* delay send usec*/
    long send_delay;			/* delay send usec*/
	long bandwidth;				/* network if band width */
    unsigned long drop;	        /* drop buffer counter*/
	unsigned long duplicate;	/* duplicate counter */
	unsigned long dup_rate;		/* duplicate rate */
	unsigned long retransmit;   /* retransmit counter*/
	unsigned long resnd_rate;   /* retransmit rate*/
    unsigned long lost;         /* queue send bytes counter*/
    unsigned long bytes;        /* queue send bytes counter*/
    unsigned long send_rate;    /* network send bytes rate*/
    unsigned long lost_rate;    /* network packet lost rate*/    
	unsigned long tx_cnt;       /* network send counter*/
	unsigned long tx_max;       /* network send counter*/
	unsigned long tx_rate;      /* network send times rate*/    

    /* send queue variables*/
    struct list_head** list;    /* priority queue list*/
    volatile unsigned int hold;	/* hold the space in queue*/
    unsigned long size;		    /* send queue capacity size*/
    unsigned long limit;		/* send queue limitation*/
    unsigned int ids;			/* identity sequence*/
    unsigned int full_waiters;  /* send queue input waiter*/
    unsigned int empty_waiters; /* send queue output waiter*/
    pthread_cond_t empty;		/* queue condition wait*/
    pthread_cond_t full;		/* queue condition wait*/
    pthread_cond_t xmit;		/* queue condition wait*/
    pthread_t scheduler;		/* queue entry handler*/
    pthread_t mornitor;		    /* queue entry handler*/
    pthread_mutex_t lock;       /* the sequence queue locker*/
    void* owner;
};

struct recv_ops{
    struct recv_queue* (*create)(struct Network* net,int flags);
    int (*init)(struct recv_queue* rq);
    int (*start)(struct recv_queue* rq);
    int (*stop)(struct recv_queue* rq);
    void (*destroy)(struct recv_queue* rq);
    int (*priority)(Buffer* buf);
    int (*input)(struct Buffer* buf);
    void* (*schedule)(void* param);
    void (*upcall)(struct Buffer*);
};

struct recv_queue{
    unsigned int flags;         /* flag the send & recv queue*/
    volatile unsigned long count;/* queue elements counter */
    struct Network* net;		/* transport network layer*/
    int sock;                   /* transport socket handle*/
    struct timer_queue *tqueue;	/* sequence timers queue */
    Hashmap*  ipmap;			/* recv ip map to peers */
    struct recv_ops *ops;		/* recv operation call set*/

    /* send queue variables*/
    struct list_head** rqueue;  /* priority dispatch queue*/
    volatile unsigned int hold;	/* hold the space in queue*/
    unsigned long size;		    /* send queue capacity size*/
    unsigned long rcv_bytes;    /* queue send bytes counter*/
    unsigned long rcv_rate;     /* network send rate*/
	unsigned long rx_cnt;       /* network receive counter*/
	unsigned long rx_rate;      /* network send rate*/
	unsigned long bytes;        /* queue recv bytes counter*/    

    volatile int running;		/* recv queue running flag */
    unsigned long limit;		/* recv queue limitation*/
    unsigned int full_waiters;  /* recv queue input waiter*/
    unsigned int empty_waiters; /* recv queue output waiter*/
    pthread_cond_t empty;		/* queue empty wait cond*/
    pthread_cond_t full;		/* queue full wait cond*/
    pthread_t scheduler;		/* recv queue buffer handler*/
    pthread_mutex_t lock;       /* the sequence queue locker*/
    void* owner;
};

struct transport{
    unsigned int ip;
    short int port;
    int sock;                   
    struct send_queue *sq;
    struct recv_queue *rq;
    struct Network* net;
    pthread_mutex_t lock;
};

typedef struct seq_entry SeqEntry;
typedef struct cwnd_info WndInfo;
typedef struct send_queue SendQueue;
typedef struct recv_queue RecvQueue;

int be_input(struct Buffer* buf);
int be_output(struct Buffer* buf);

struct cwnd_info* be_rwin_info_create(int flags);
struct send_queue* be_send_queue_create(struct Network* net,int flags);
int be_send_queue_init(struct send_queue* queue);
int be_send_queue_start(struct send_queue* queue);
int be_send_queue_stop(struct send_queue* queue);
void be_send_queue_destroy(struct send_queue* queue);

struct recv_queue* be_recv_queue_create(struct Network* net,int flags);
int be_recv_queue_init(struct recv_queue* rq);
int be_recv_queue_start(struct recv_queue* rq);
int be_recv_queue_stop(struct recv_queue* rq);
void be_recv_queue_destroy(struct recv_queue* rq);
inline int be_rwnd_free_space(struct recv_queue* rq);

#endif /* _TRANSORT_H_ */
