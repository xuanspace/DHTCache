/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 * 
 * Reliable UDP Interface
 * 
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
 *
 * $Id: rudp.c,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */

#include <errno.h>
#include "log.h"
#include "buffer.h"
#include "network.h"
#include "message.h"
#include "upcall.h"
#include "udp.h"

#define SEQ_CACHE_SIZE 100
#define bufip inet_ntoa(buf->desc->to.sin_addr)
#define TRACE(...) if(srv->flags & UDP_TRACE) __log.debug(__VA_ARGS__)

void* udp_schedule(void* param);
int udp_map_free(struct udp_server* srv);

struct udp_server* udp_create(int flags)
{
    struct udp_server* srv = NULL;
    srv = malloc(sizeof(struct udp_server));
    if(srv != NULL){
        /* init the epoll server*/
        memset(srv,0,sizeof(struct udp_server));        
		srv->ipmap = hash_new(10);
        srv->rqueue = queue_create(1000);
		srv->flags = flags ;
	}
	return srv;
}

int udp_init(struct udp_server* srv)
{
    if(!srv) return -1;

    /* init server socket*/
    srv->sock = srv->net->sock;

    /* init server lock */
    if(pthread_mutex_init(&srv->lock, NULL) != 0)
        return -1;
	return 0;
}

int udp_start(struct udp_server* srv)
{
    if(!srv) return -1;
    srv->running = 1;

    /* create udp scheduler thread */
    if(pthread_create(&srv->scheduler,NULL,
        udp_schedule,(void*)srv) != 0)
        return -1;

	return 0;
}

int udp_stop(struct udp_server* srv)
{
    if(!srv) return -1;

    /* set server stop flag*/
    srv->running = 0;

    /* stop input queue*/
    queue_term(srv->rqueue);

    if(pthread_join(srv->scheduler, NULL) !=0)
        return -1;
    
	return 0;
}

void udp_destroy(struct udp_server* srv)
{
    if(!srv) return;

    /* release the peers*/
    udp_map_free(srv);

    /* release hash map*/
    hash_destroy(srv->ipmap);

    /* release receive queue*/
    queue_destroy(srv->rqueue);

    /* release server mutex */
    pthread_mutex_destroy(&srv->lock);

    free(srv);
}

struct udp_peer* udp_peer_new()
{
    int size;
    struct udp_peer* peer = NULL;

    /* create peer udp info,map with ip*/
    peer = malloc(sizeof(struct udp_peer));
    if(peer){
        memset(peer,0,sizeof(struct udp_peer));
        size = sizeof(uint16)*SEQ_CACHE_SIZE;
        peer->state = UDP_INIT;
        peer->seq_map = hash_new(10);
        peer->seq_his = malloc(size);
        memset(peer->seq_his,1,size);
    }
    return peer;
}

void udp_peer_free(struct udp_peer* peer)
{
    if(peer != NULL){
        if(peer->seq_map)
            hash_destroy(peer->seq_map);
        if(peer->seq_his)
            free(peer->seq_his);
        free(peer);
    }
}

int udp_map_free(struct udp_server* srv)
{
    HashPair *pair;
    struct udp_peer* peer;

    pair = hash_first(srv->ipmap);
    while (pair) {
        peer = (struct udp_peer*)pair->value;
        pair = hash_next(srv->ipmap);
        udp_peer_free(peer);
    }
    return 0;
}

struct udp_peer* udp_peer_lookup(struct udp_server* srv,uint32 ip)
{
    struct udp_peer* peer = NULL;

    /* get the peer window info*/
    peer = (struct udp_peer*)hash_get(srv->ipmap,ip);
    if(peer == NULL){
        peer = udp_peer_new(0);
        peer->id = srv->ids++;
        hash_set(srv->ipmap,ip,peer);
    }
    return peer;
}

int udp_addr_print(struct udp_server* srv,struct sockaddr *paddr)
{
    struct sockaddr_in *addr = (struct sockaddr_in*)paddr;
    TRACE("Network:        *sockaddr_in{ sin_family = %d, sin_addr.s_addr = %d(%s), sin_port = %d(%d)}\n",
        addr->sin_family,
        addr->sin_addr.s_addr, inet_ntoa(addr->sin_addr),
        addr->sin_port, ntohs(addr->sin_port)
        );
    return 0;
}

int udp_buf_print(struct udp_server* srv,struct Buffer* buf,unsigned int flag)
{
    udp_addr_print(srv,&buf->desc->addr);
    TRACE("Network:        *buffer{ seq #%d, len = %d, data = 0x%X}\n",
        buf->pkt->seq,
        (flag == 1) ? HEADER_SIZE(Packet) : buf->desc->size, 
        buf->desc->data);
    return 0;
}

int udp_filter(struct udp_server* srv,struct Buffer* buf)
{
    int ret = -1;
    int slot = -1;    
    struct udp_peer* peer;
    uint16 seqno = buf->pkt->seq;

    pthread_mutex_lock(&srv->lock);
    peer = udp_peer_lookup(srv,buf->dst);
    if(peer != NULL){
        if (buf->pkt->flag & SYN_RST) {
            memset(peer->seq_his,0,sizeof(uint16)*SEQ_CACHE_SIZE);
            ret = 0; /*reset sequence*/
        }else{
            slot = seqno%SEQ_CACHE_SIZE;
            if(seqno != peer->seq_his[slot]){
                peer->seq_his[slot] = seqno;
                ret = 0; /*new sequence*/
            }
        }
    }else{ /* not found the peer */
        ret = 0; /*we let it in as new*/
    }

    pthread_mutex_unlock(&srv->lock);
    return ret;
}

int udp_ack(struct udp_server* srv,struct Buffer* buf)
{
    int ret = -1;
    Packet ackd;

    /* check if need acknowledgment*/
    if (buf->pkt->flag & ACK_PLEASE) 
    {
        /* make acknowledgment packet*/
        ackd.flag = 0;
        ackd.seq = buf->pkt->seq;
        ackd.flag |= ACK_YES; /*ack flag*/
        ret = sendto(srv->sock, &ackd,
            HEADER_SIZE(Packet), 0, 
            &buf->desc->addr,
            sizeof(buf->desc->addr));
        if (ret < 0){
            DBG("Network: ->failed to send %s ACK packet,sock %d error %d(%s).\n",
                bufip,srv->sock,errno,strerror(errno));
        }else{
            TRACE("Network: ->send %s ack packet #%d.\n",bufip,buf->pkt->seq);
        }

        /*log out packet buffer*/
        udp_buf_print(srv,buf,1);
    }

    return ret;
}

int udp_acked(struct udp_server* srv,struct Buffer* buf)
{
    uint32 sid;
    struct udp_peer* peer;

    pthread_mutex_lock(&srv->lock);
    peer = udp_peer_lookup(srv,buf->dst);
    if(peer != NULL){
        sid = (peer->id << 16) + buf->pkt->seq;
        hash_remove(peer->seq_map,sid);
    }
    pthread_mutex_unlock(&srv->lock);

    return 0;
}

int udp_syn(struct udp_server* srv,uint32 ip)
{
    int ret;
    Packet syn;

    /* build the synchronize sequence request */	
    syn.seq = 0; /* start from zero*/
    syn.flag = SYN_RST | ACK_PLEASE;
    syn.win = 0;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(srv->net->port);

    TRACE("Network: send SYN %d...\n",ip);
    ret = sendto(srv->sock,&syn,
        HEADER_SIZE(Packet), 0, 
        (struct sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        DBG("Network: ->send SYN packet,errno %d(%s).\n",errno,strerror(errno));
    }else{
        TRACE("Network: ->send SYN packet ok.\n");
    }

    /*log out packet buffer*/
    udp_addr_print(srv,(struct sockaddr*)&addr);
    return ret;
}

uint16 udp_window(struct udp_server* srv)
{
    return 0;
}

int udp_sendbuf(struct udp_server* srv,Buffer* buf)
{
    uint32 sid;
    int ret,retry=0;
    int count = 0;    
    struct udp_peer* peer;

    /* get peer udp server status*/
    pthread_mutex_lock(&srv->lock);
    peer = udp_peer_lookup(srv,buf->dst);
    TRACE("Network: ->send look up %d.\n",buf->dst);

    /* if init status,set SYN flag*/
    if(peer->state == UDP_INIT){
        peer->state = UDP_SYN_SENT;
        buf->pkt->flag |= SYN_RST;
    }

    /* if peer udp server status*/
    if(peer->state == UDP_CLOSE){
        TRACE("Network: ->detect %s closed, discard send pkt.\n",bufip);
		pthread_mutex_unlock(&srv->lock);
        buffer_free(buf);
        return 0;
    }
      
    /* fill buffer packet members*/
    buf->pkt->seq = peer->iss++;
    buf->pkt->win = udp_window(srv);
    sid = (peer->id << 16) + buf->pkt->seq;
    hash_set(peer->seq_map,sid,buf);
    TRACE("Network: ->send %s req packet #%d, sid%d.\n",bufip,buf->pkt->seq,sid);
    pthread_mutex_unlock(&srv->lock);

    /* send out packet buffer*/
    resend: retry++;
    ret = sendto(srv->sock,
        buf->desc->data,
        buf->desc->size,0,
        &buf->desc->addr,
        sizeof(buf->desc->addr));
    if (ret < 0) {
        DBG("Network: ->sendto %s,sock %d errno %d(%s).\n",
            bufip,srv->sock,errno,strerror(errno));        
    }

    /*log out packet buffer*/
    udp_buf_print(srv,buf,0);

    /*wait the ack packet*/
    count = 0;
    while (count++ < 50){
        usleep(20000); /*wait seq ack*/
        if(hash_get(peer->seq_map,sid))
            continue;

        TRACE("Network: ->sent %s ok  packet #%d, sid%d.\n",bufip,buf->pkt->seq,sid);        
        return ret;/*packet acked*/
    }

    /*resend the packet buffer*/
    if(retry < 7)
        goto resend;

    TRACE("Network: ->send %s seq %d retry failure.\n",bufip,buf->pkt->seq,sid);
    hash_remove(peer->seq_map,sid);
    return -1;
}

void udp_upcall(struct udp_server* srv,struct Buffer* buf)
{
    buffer.pop(buf, HEADER_SIZE(PacketDesc));
    buffer.pop(buf, HEADER_SIZE(Packet));    
    __network_onreceived(srv->net, &buf->desc->from, buf);
}

int udp_receive(struct udp_server* srv,struct Buffer* buf)
{
    /* handle ack and ack reply*/
    if (buf->pkt->flag & ACK_YES) {
        TRACE("Network: <-recv %s ack packet #%d.\n",bufip,buf->pkt->seq);
        /* free ack message buffer*/
        udp_acked(srv,buf);
        buffer.free(buf);
        return 0;
    }

    /* if broadcast not filtered*/
    if (buf->pkt->flag & BO_CAST){
        TRACE("Network: <-recv bocast packet #%d.\n",bufip,buf->pkt->seq);
		udp_upcall(srv,buf);
        buffer.free(buf);
        return 0;
    }

    /* duplicate sequence filter*/    
    if (udp_filter(srv,buf) != 0) {
        TRACE("Network: <-recv %s duplicate packet #%d.\n",bufip,buf->pkt->seq);
        /* resend acknowledgment ACK_YES */
        udp_ack(srv,buf);
        buffer.free(buf);
        return 0; /*discard,exit*/
    }

    /* normal packet buffer*/    
    TRACE("Network: <-recv %s req packet #%d.\n",bufip,buf->pkt->seq);
    udp_ack(srv,buf);
    queue_push(srv->rqueue,buf);
    return 0;
}

int udp_input(struct Buffer* buf)
{
    struct udp_server* srv;
    srv = (struct udp_server*)buf->owner;
    return udp_receive(srv,buf);
}

int udp_output(struct Buffer* buf)
{
    int ret;
    struct udp_server* srv;
    srv = (struct udp_server*)buf->owner;
    ret = udp_sendbuf(srv,buf);
    buffer.free(buf);
    return ret;
}

void* udp_schedule(void* param)
{
    struct Buffer* buf;
    struct udp_server* srv;
    srv = (struct udp_server*)param;

    /* dequeue and upcall buffer */
    while (srv->running){
        /* pop send buffer */
        buf = NULL;
        queue_pop(srv->rqueue,(void**)&buf);
        if(buf != NULL){            
            udp_upcall(srv,buf);
            buffer.free(buf);
        }
    }
    return 0;
}
