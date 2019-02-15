/**
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* Reliable UDP Network Layer
*
* $Id: network.c,v 1.7 2009-01-14 07:19:38 hcai Exp $
*
* Author: Alex Wei
*
*/

#include "global.h"
#include "network.h"
#include "host.h"
#include "timer.h"
#include "log.h"
#include "message.h"
#include "upcall.h"
#include "udp.h"
#include "epoll.h"

#define INIT_MSG_TYPE 1000

Network* 
network_new(int port,int protocol)
{
    Network *net = NULL;

    /* alloc network instance */
    net = NEW(Network);
    memset(net,0,sizeof(Network));
    net->flag = NET_SEQLOG;

    /* use the tcp protocol*/    
    net->port = port;
    net->protocol = 1;
    net->callback_map = hash_new(97);
    host_set(&net->host, NULL, port);
    net->run = true;
    
    /* create TCP server*/
    net->tcp_server = eserver_create(port);
    net->tcp_server->net = net;

    /* stat counter */
    net->udp_recv = NULL;
    net->udp_send = NULL;
    net->udp_fail = NULL;
    net->udp_timeout = NULL;

    /* network func setting*/
    net->initialize = network_initialize;
    net->start = network_start;
    net->send = network_send;
    net->stop = network_stop;
    net->destroy = network_destroy;
    net->get_listen_port = network_listen_port;
    net->set_msg_cb = network_set_msg_callback;
    net->tcp_output = epoll_output;

    return net;
}

bool
network_initialize(Network * net)
{
    int ret;

    /* init epoll tcp server */
    ret = eserver_init(net->tcp_server);
    if (ret != 0) {
        ERR("Network - Init epoll tcp error.\n");
        return ret;
    }

    /* get the local address */
    net->myAddr = network_addr(net);

    return true;
}


/**
* Return local first IP address
* @author  Alex Wei
*/
uint32
network_addr(Network * net)
{
    uint32 ip = LOOPBACK;
    int i = 1;

    struct ifreq ifr;
    struct sockaddr_in *sin;

    /* Linux */
    while (i < 10) {
        sin = (struct sockaddr_in *) &ifr.ifr_addr;
        ifr.ifr_ifindex = i++;
        if (ioctl(net->sock, SIOCGIFNAME, &ifr) < 0)
            break;

        /* now ifr.ifr_name is set */
        if (ioctl(net->sock, SIOCGIFADDR, &ifr) < 0)
            continue;
        else {
            ip = ntohl(sin->sin_addr.s_addr);
            if (ip == LOOPBACK)
                continue;
            break;
        }
    }

    return ip;
}

int
network_send(Network* net, Host* to, Buffer* buf, bool reliable)
{
    uint32 ip;
    uint32 size; 
    struct sockaddr_in* sin;

    /* parameters check*/
    if (net == NULL || to == NULL || buf == NULL)
        return -1;

    /* self broadcast check*/
    sin = (struct sockaddr_in*) to;
    ip = ntohl(sin->sin_addr.s_addr);
    if (ip == BROADCAST) {
        /*DBG("Network - broadcast.\n");*/
        reliable = false;
    }

    /* build the buffer head*/
    size = buffer.size(buf);
    buf->pkt = PUSH_PACKET(buf);
    buf->hdr = GET_HEAD(buf);
    buf->pkt->flag = 0;
    buf->pkt->size = size;
    buf->pkt->flag |= reliable ? ACK_PLEASE : ACK_NO;

    size = buffer.size(buf);
    buf->desc = PUSH_DESC(buf);
    buf->desc->size = size;
    buf->desc->to = *to;
    buf->dlen = size;
    buf->data = (char*)buf->pkt;
    
    /* set message destination*/
    if(buf->hdr->type < INIT_MSG_TYPE)
        buf->dst = ip;
    else
        buf->type = EPOLL_FD_BUF;
    
    /* send message buffer*/
    buf->owner = net->tcp_server;
    return net->tcp_output(buf);
}

int
network_start(Network * net)
{
    int ret;
    if(!net) return -1;

    /* start TCP epoll server */
    ret = eserver_start(net->tcp_server);
    if (ret != 0) {
        ERR("Network - Start epoll tcp error.\n");
        return ret;
    }

    /* return */
    return ret;
}

void
network_stop(Network * net)
{
    /* check if network running */
    if (!net || !net->run)
        return;

    /* set network stop flag */
    LOG("Network: exiting...\n");
    net->run = false;

    /* stop tcp epoll server */
    eserver_stop(net->tcp_server);

    LOG("Network: exited.\n");
}


void
network_destroy(Network * net)
{
    if(!net) return;

    /* stop reliable udp server*/
    udp_destroy(net->udp_server);

    /* stop tcp epoll server*/
    eserver_destroy(net->tcp_server);

    /* release upcall dispatch*/
    upcall_destroy(net);

    /* close the udp socket*/
    close(net->sock);

    free(net);
}

uint16 network_listen_port(Network *net)
{
    return host_port(&net->host);
}

char* ip_htoa(uint32 ip, char* strbuf, size_t bufsize)
{
    snprintf(strbuf,
        bufsize,
        "%u.%u.%u.%u",
        *(((byte *) &ip) + 3),
        *(((byte *) &ip) + 2),
        *(((byte *) &ip) + 1),
        *((byte *) &ip));
    return strbuf;
}

#define IFRSIZE(size)   ((int)((size)*sizeof(struct ifreq)))
uint32 getlocaluip()
{
    struct ifreq*      ifr;
    struct ifconf      ifc;
    int                sockfd;
    int                size;

    if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
        DBG( "get local ip: cannot open socket.\n");
        return 0;
    }

    size = 1;
    ifc.ifc_len = IFRSIZE(size);
    ifc.ifc_req = NULL;

    uint32 ip = 0;

    do {
        ++size;
        /* realloc buffer size until no overflow occurs  */
        if (NULL == (ifc.ifc_req = (struct ifreq*)realloc((void *)ifc.ifc_req, IFRSIZE(size)))) {
            DBG( "get local ip: out of memory.\n");
            return 0;
        }
        ifc.ifc_len = IFRSIZE(size);
        if (ioctl(sockfd, SIOCGIFCONF, &ifc)) {
            perror("ioctl SIOCFIFCONF");
            DBG( "get local ip: ioctl fail.\n");
            free(ifc.ifc_req);
            return 0;
        }
    } while  (IFRSIZE(size) <= ifc.ifc_len);


    ifr = ifc.ifc_req;
    for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr) {

        if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data) {
            continue;  /* duplicate, skip it */
        }

        if (ioctl(sockfd, SIOCGIFFLAGS, ifr)) {
            continue;  /* failed to get flags, skip it */
        }

        struct sockaddr_in* ifraddr = (struct sockaddr_in*) &ifr->ifr_ifru.ifru_addr;
        ip = ntohl(ifraddr->sin_addr.s_addr);

        if (ip == LOOPBACK)
            continue;
        else
            break;
    }

    free(ifc.ifc_req);
    close(sockfd);
    return ip;
}

