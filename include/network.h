/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Reliable UDP Network Layer
 *
 * $Id: network.h,v 1.3 2008-12-12 03:27:45 hcai Exp $
 *
 * AUTHOR(S): Alex Wei      sunding.wei@sierraatlantic.com
 *
 */

#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "global.h"
#include "buffer.h"
#include "host.h"
#include "hashmap.h"
#include "fifo.h"
#include "list.h"
#include "misc.h"
#include "upcall.h"
#include "stat_cnt.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#define NETWORK_RETRY           5
#define NETWORK_TIMEOUT         20       /* Milliseconds */

#define PUSH_PACKET(BUF) \
    (Packet*) buffer.push(BUF, HEADER_SIZE(Packet));
#define PUSH_DESC(BUF) \
    (PacketDesc*) buffer.push(BUF, HEADER_SIZE(PacketDesc));
#define GET_HEAD(BUF) \
    (struct MsgHead*)buf->pkt->data;

#define MAKE_PACKET(BUF) \
    (Packet*)BUF->desc->data;
#define MAKE_DESC(BUF,SIZE) \
    (PacketDesc*) buffer.push(BUF, SIZE);
#define MAKE_HEAD(BUF) \
    (struct MsgHead*)BUF->pkt->data;

enum NETWORK_FLAGS{
    NET_DEFAULT     = 0x00,
    NET_SEQLOG      = 0x02,
    NET_USETCP      = 0x04,
    NET_USEUDP      = 0x08
};

enum PACKET_FLAGS{
    ACK_NO          = 0x000,  /* I don't need ACK */
    ACK_PLEASE      = 0x002,  /* ACK please if you received */
    ACK_YES         = 0x004,  /* Yes, I received */
    SYN_RST         = 0x008,  /* Synchronize sequence numbers */
    ACK_SYN         = 0x010,  /* Initial send sequence number */
    BO_CAST         = 0x020,  /* Broadcast packet*/
    ACK_BLOCK       = 0x040,  /* Acknowledgments with block message*/
    ACK_TEST        = 0x080,  /* Acknowledgments test*/
    URG_STOP        = 0x100,  /* Network urgent stop*/
    URG_RAVEL       = 0x200,  /* Network urgent recovery*/
    SRV_INNER       = 0x400,  /* Identify inner connetion*/
    SRV_OUTER       = 0x800,  /* Identify outer connetion*/
    DBG_TRACE       = 0x1000  /* packet debug trace*/
};

struct PacketDesc{
    union {
        Host from;
        Host to;
        struct sockaddr addr;
    };
    uint32  size;
    char    data[1];
} ;

struct Packet{
    uint16  seq;
    uint16  flag;       /* ACK,SYN,FIN*/
    uint16  win;        /* window size*/
    uint32  size;       /* data size */
    char    data[1];
}__attribute__ ((packed));

struct MsgHead{
    uint16    type;
    uint16    seq;
}__attribute__ ((packed));

/* forward declaration */
struct Network;
struct udp_server;
struct epoll_server;
struct send_queue;
struct recv_queue;
struct send_ops;
struct recv_ops;

struct Network 
{
    /* interfaces func */
    bool (*initialize)(struct Network *net);
    int  (*start)(struct Network *net);
    void (*stop)(struct Network *net);
    void (*destroy)(struct Network *net);
    uint32 (*next_seq)(struct Network * net);
    uint16 (*get_listen_port)(struct Network *net);
    void (*set_msg_cb)(struct Network *net, int msg_type, void*, onsent_t, onreceived_t);
    int (*send)(struct Network* net, Host* to, Buffer* buf, bool reliable);
    int (*udp_output)(struct Buffer *buf);
    int (*udp_input)(struct Buffer *buf);
    int (*tcp_input)(struct Buffer *buf);	
    int (*tcp_output)(struct Buffer *buf);	    
    /* On node failure detected callback */
    int (*onnodefailure)(struct Network* net, Host* from);


    /* local host ip:port */
    union {
        Host host;                      /* struct sockaddr_in */
        struct sockaddr addr;           /* 14 bytes of address*/
    };
    
    /* network variables*/
    int         flag;                   /* network self flags*/
    bool        run;                    /* network running state */
    SOCKET_T    sock;                   /* udp socket fd handle*/
    pthread_t   lthread;                /* listen thread handle*/
    uint32      myAddr;                 /* first ip address of local*/	
    unsigned short port;                /* network listen port*/
    int         protocol;               /* network using protocol*/	

    /* underlying instances*/
    struct udp_server* udp_server;      /* udp socket server*/
    struct epoll_server* tcp_server;    /* tcp socket server*/
    Hashmap*   callback_map;            /* message callbacks */

    /* stat counters */
    stat_counter* udp_recv;
    stat_counter* udp_send;
    stat_counter* udp_fail;
    stat_counter* udp_timeout;
};

typedef struct Packet Packet;
typedef struct PacketDesc PacketDesc;
typedef struct MsgHead MsgHead;
typedef struct Network Network;

#ifdef __cplusplus
extern "C" {
#endif

struct Network* network_new(int port,int protocol);
bool     network_initialize(struct Network *net);
int      network_start(struct Network *net);
int      network_send(struct Network *net, Host *dest, Buffer *buf, bool reliable);
void     network_stop(struct Network *net);
void     network_destroy(struct Network *net);
uint32   network_addr(struct Network * net); 
uint16   network_listen_port(struct Network *net);

/* convert uint32 ip to dotted string format */
char*    ip_htoa(uint32 ip, char* strbuf, size_t bufsize);
uint32   getlocaluip();

#ifdef __cplusplus
}
#endif

#endif /* __NETWORK_H__ */
