/*
 * DHT Cache Module
 * Copyright (c), 2008, GuangFu, 
 * 
* epoll server head file
*
* Author(s): wxlin  <weixuan.lin@sierraatlantic.com>
*
* $Id:epoll.h,v 1.4 2008-12-30 08:14:55 wxlin Exp $
*
*/
#ifndef _EPOLL_H_
#define _EPOLL_H_

#include "host.h"
#include "list.h"
#include "hashmap.h"
#include "buffer.h"
#include "queue.h"
#include "threadpool.h"
#include <sys/epoll.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define EPOLL_MAX_POOL_THREADS 10

struct Network;
struct epoll_server;
struct epoll_client;
struct epoll_ipbuf;

enum epoll_type
{
    EPOLL_ACCEPT,                   /* accept connection type*/    
    EPOLL_CONNECT,                  /* active connection type*/    
    EPOLL_INNER_CONN,               /* server connection type*/    
    EPOLL_OUTER_CONN,               /* non server connection*/
    EPOLL_DEFAUL_TYPE,              /* full epoll server type*/
    EPOLL_NONSRV_TYPE               /* connection server type*/    
};

enum packet_flags
{
    EPOLL_SOURCE        = 0x00,     /* packet source*/    
    EPOLL_REQUEST       = 0x02,     /* packet request*/    
    EPOLL_REPLY         = 0x04,     /* packet reply*/    
    EPOLL_RESULT        = 0x08,     /* packet result*/
    EPOLL_FORWARD       = 0x10,     /* packet forward*/
    EPOLL_PROBE         = 0x20,     /* packet test*/
};

enum write_flags
{
    WEVNET_UNSET        = 0x000,    /* unset write event*/
    WEVENT_REQUESTED    = 0x002	    /* alread set write event*/
};

enum buf_flags
{
    EPOLL_WRITE_FAIT    = -1,		/* write failure*/
    EPOLL_IP_BUF        = 0x00,     /* buffer from server*/    
    EPOLL_FD_BUF        = 0x02,		/* buffer from client*/
    EPOLL_ID_BUF        = 0x04,		/* identity buffer */
    EPOLL_PROBE_BUF     = 0x08,		/* probe connection*/
    EPOLL_WRITE_OK      = 0x10,		/* write buffer ok*/    
};

enum epoll_flags
{
    EPOLL_READ_TPOOL    = 0x00,		/* use read thread pool*/
    EPOLL_WRITE_TPOOL   = 0x02,		/* use write thread pool*/
    EPOLL_UPCALL_TPOOL  = 0x04,		/* use upcall thread pool*/
    EPOLL_SINGLE_THRW   = 0x08,		/* use single thread rw*/    
    EPOLL_FE_CONGEST    = 0x10,		/* use fe congest control*/
    EPOLL_BE_CONGEST    = 0x20,		/* use be congest control*/
    EPOLL_READ_BALANCE  = 0x40,		/* use epoll read balance*/
    EPOLL_WRITE_DIRECT  = 0x80,		/* use epoll write direct*/
    EPOLL_WRITE_LOOP    = 0x100,	/* use epoll write loop*/
    EPOLL_CONGEST_AVOID = 0x200,	/* use congestion avoid*/
    EPOLL_DBG_TRACE     = 0x400		/* use debug trace*/
};

enum client_flags
{
    EPOLL_DEFAULT       = 0x00,		/* default connection*/
    EPOLL_FIRST         = 0x01,		/* first accept data*/
    EPOLL_IDENTIFIED    = 0x02,		/* identified connection*/
    EPOLL_MAPIP         = 0x04,		/* indicate ip mapped*/
    EPOLL_TIMEOUT       = 0x08,		/* write client timeout*/
    EPOLL_RBROKEN       = 0x10,		/* read client broken*/
    EPOLL_WBROKEN       = 0x20,		/* write client broken*/
    EPOLL_HALFLIFE      = 0x40,     /* timeout but alive*/
    EPOLL_PKILLED       = 0x80,     /* connection player killing*/
    EPOLL_PROBING       = 0x100     /* detect server dead*/
};

enum epoll_status
{
    EPOLL_CONNECTED     = 0x00,		/* default is avaiable*/
    EPOLL_EAGAIN        = 0x02,		/* no more data read */
    EPOLL_NOMEM         = 0x04,		/* no memory for epoll*/
    EPOLL_READING       = 0x08,		/* epoll event reading*/
    EPOLL_WRITING       = 0x10,		/* epoll event writing*/
    EPOLL_READNEXT      = 0x20,		/* epoll next event*/
    EPOLL_WRITNEXT      = 0x40,		/* epoll next writing*/
    EPOLL_SCHEDULING    = 0x80,		/* epoll event schduling*/
    EPOLL_CLOSED        = 0x100,	/* event socket closed*/
    EPOLL_ERROR         = 0x200,    /* event socket error*/
    EPOLL_URGENT        = 0x400,    /* server urgent status*/
    EPOLL_DEADTH        = 0x800,    /* detect server dead*/    
};

struct epoll_task
{
    int fd;							/* event socket file handle*/
    struct epoll_client* client;    /* event fd's client object*/
    struct Buffer* wbuf;			/* socket write buffer ptr*/
};

struct epoll_ops
{
    int (*add)(struct epoll_server* srv,int fd,unsigned int ip);
    int (*del)(struct epoll_server* srv,int fd);
    int (*dispatch)(struct epoll_server* srv,int fd);
    void* (*loop)(void *args);
    int (*read)(struct epoll_server* srv,int fd,Buffer* buf);
    struct epoll_server* (*create)();
    int (*init)(struct epoll_server* srv);
    int (*start)(struct epoll_server* srv);
    int (*stop)(struct epoll_server* srv);
    void (*destroy)(struct epoll_server* srv);
    int (*recv)(struct epoll_server* srv,struct epoll_client *client);
    int (*send)(struct epoll_server* srv,Buffer* buf);
    int (*input)(struct epoll_server* srv,Buffer* buf);
    int (*output)(Buffer* buf);
    void (*fail)(struct Network* self, Host* from);
    void (*callback)(struct Network* self, Host* to, uint16 id, bool stat, Buffer *buf, bool* keep);
};

struct epoll_server
{
    int flags;                      /* epoll server init flags*/
    int state;                      /* epoll server status*/
    int running;                    /* server running flag*/
    short int type;                 /* identify server type*/
    int port;                       /* listening socket port*/
    int listen_fd;                  /* listening socket handle*/
    int epoll_fd;                   /* epoll socket handle*/
    int nevents;                    /* epoll max listen events*/
    int timeout;                    /* epoll listen wait timeout*/
    int delay;                      /* epoll send delay*/
    unsigned int seq;               /* epoll buffer sequence*/
    struct epoll_event* events;     /* epoll event array*/
    struct queue_t* rtask;          /* read event task queue*/
    struct queue_t* wtask;          /* write event task queue*/
    struct queue_t* rqueue;         /* input buffer queue*/
    Hashmap* ipmap;                 /* connection ip map*/
    Hashmap* fdmap;                 /* epoll socket handle map*/
    Hashmap* idmap;                 /* connection ip map*/
    pthread_t acceptor;             /* socket data reader thread*/
    pthread_t reader;               /* socket data reader thread*/
    pthread_t epoller;              /* epoll loop thread*/
    pthread_t writer;               /* socket data writer thread*/
    pthread_t sender;               /* socket data sender thread*/
    pthread_t scheduler;            /* upcall schedule thread*/
    pthread_t wpoller;              /* wpoll loop thread*/
    pthread_t recycler;             /* buffer free thread*/
    pthread_t prober;               /* probe host thread*/
    FIFO *free_queue;		        /* buffer free queue*/
    SEM_T free_sem;		            /* buffer free sem*/
    FIFO *probe_queue;		        /* probe buffer queue*/
    SEM_T probe_sem;		        /* probe buffer sem*/
    struct epoll_ops* ops;          /* server operation set*/
    volatile unsigned int rbuf_cnt; /* receive buffer counter*/
    unsigned int rbuf_limit;        /* limitation of read buffer*/
    volatile unsigned int wbuf_cnt; /* send buffer counter*/
    unsigned int wbuf_limit;        /* limitation of send buffer*/
    unsigned int full_waiters;      /* buffer full waiter*/
    unsigned int empty_waiters;     /* buffer empty waiter*/
    pthread_cond_t wcond;	        /* write queue condition*/
    pthread_cond_t wfull;	        /* write full condition*/
    pthread_mutex_t rlock;          /* event read lock*/
    pthread_mutex_t wlock;          /* event write lock*/
    pthread_mutex_t lock;           /* server object lock*/
    unsigned int reader_n;          /* n threads do read*/
    unsigned int writer_n;          /* n threads do write*/
    unsigned int scheduler_n;       /* n threads do schedule*/
    unsigned int thrmax;            /* threads max in pool*/
    ThreadPool threadpool;          /* thread pool for rw*/
    struct list_head** list;        /* priority queue list*/
    struct list_head *black_list;   /* dead nodes blacklist*/
    unsigned int bklist_cnt;        /* dead nodes counter*/
    unsigned long wtick;            /* write tick counter*/
    struct Network *net;	        /* outside network layer*/
    struct list_head *iplink;       /* ip buffer link head*/
    struct epoll_ipbuf *ipcur;      /* current ip link ptr*/
};

struct epoll_client
{
    struct list_head link;          /* client list link node*/
    int fd;                         /* client socket handle*/
    volatile short int flag;        /* client current flag*/
    short int ctype;                /* connect type flag*/
    short int type;                 /* accept/connect type*/
    unsigned int ip;                /* client ip address*/
    unsigned short port;            /* client socket port*/
    unsigned int win;               /* peer client window*/
    volatile uint32 event;          /* client epoll event*/
    volatile short int state;       /* epoll client status*/
    volatile short int rstate;      /* client read status*/
    volatile short int wstate;      /* client write status*/
    volatile short int wflag;       /* client write flag*/
    volatile unsigned int write;    /* write request count*/
    struct Buffer* rbuf;            /* socket read buffer*/
    struct Buffer* wbuf;            /* socket write buffer*/    
    volatile int refcnt;            /* client reference count*/
    unsigned int evcount;           /* epoll event counter*/
    struct timeval stamp;           /* epoll client timestamp*/
    volatile long wttl;             /* write start timestamp*/
    long deadline;                  /* client deadline time*/
    int delay;                      /* epoll send delay count*/
};

struct epoll_ipbuf
{
    struct epoll_ipbuf *next;	   /* next ip node link*/
    struct epoll_ipbuf *prev;      /* prev ip node link*/
    struct list_head** list;       /* target ip's buffers*/
    unsigned int ip;               /* target scoket address*/
    unsigned int count;		       /* buffer list counter */
};

struct epoll_packet
{
    uint16  seq;                    /* packet sequence num*/
    uint16  flag;                   /* ACK,SYN,FIN etc.*/
    uint16  win;                    /* receive window size*/
    uint32  size;                   /* packet data size */
    uint16  type;                   /* packet identify type */
    uint16  sn;                     /* type sequence num*/
    uint32  src;                    /* packet source ip addr*/
    uint32  dst;                    /* packet destine ip addr*/
    uint32  fd;                     /* packet source socket */
    uint16  op;                     /* packet operation code*/
}__attribute__ ((packed));

struct epoll_server* eserver_create();
int eserver_init(struct epoll_server* srv);
int eserver_start(struct epoll_server* srv);
int eserver_stop(struct epoll_server* srv);
void eserver_destroy(struct epoll_server* srv);
int epoll_output(Buffer* buf);
int epoll_congest(struct epoll_server* srv,uint32 ip);
int epoll_ping(struct epoll_server* srv,uint32 ip);
int epoll_pong(struct epoll_server* srv,uint32 ip);
int epoll_alive(struct epoll_server* srv,uint32 ip,uint16 wnd);

#endif /* EPOLL_H_ */

