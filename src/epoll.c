/*
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* tcp epoll interface
*
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
*
* $Id: epoll.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
*
*/

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include "log.h"
#include "buffer.h"
#include "network.h"
#include "epoll.h"
#include "list.h"
#include "timeval.h"
#include "message.h"
#include "priority.h"

#define SO_NODELAY          0x200
#define TCP_CORK 		    3
#define SOCKET_ERROR 	    -1
#define EREAD               1
#define EWRITE              2
#define WPOLL_TICK			1000
#define WPOLL_TIMEOUT       30
#define WPOLL_TIMEDELTA     5
#define BLACK_TIMEOUT       300
#define INIT_MSG_TYPE       1000
#define CONGEST_KEEPALIVE   1000
#define CONGEST_THRESHOLD   50
#define CONGEST_DELAY       10240
#define UPCALL_THREAD       0xDEADBECA
#define TRACE(...)          if(srv->flags & EPOLL_DBG_TRACE) __log.debug(__VA_ARGS__)

__thread int __thrid;
extern struct epoll_ops _eserver_ops;
void* epoll_read_thr(void *args);
void* epoll_write_thr(void *args);
void* epoll_schedule(void* param);
void* epoll_wpoll(void* arg);
void* epoll_recycle(void *args);
void* epoll_prober(void *args);
int epoll_pkill(struct epoll_server* srv,int fd);
int epoll_clarify(struct epoll_server* srv,uint32 ip);
int epoll_reader(struct epoll_server* srv,int fd);
int epoll_writer(struct epoll_server* srv,int fd);
int epoll_read(struct epoll_server* srv,int fd,Buffer* buf);
int epoll_write(struct epoll_server* srv,int fd,Buffer* buf);
int epoll_del(struct epoll_server* srv,int fd);
int epoll_wpush(Buffer* buf);
int epoll_wsignal(struct epoll_server* srv);
int epoll_wcycle(struct epoll_server* srv,Buffer* buf);
void epoll_wfree(struct epoll_server* srv,struct Buffer* buf,int wstate);
int epoll_probe(struct epoll_server* srv,uint32 ip);
struct Buffer* epoll_buff();
int epoll_packet(struct Buffer* buf);

inline int epoll_breakpoint()
{
    /* breakpoint test for epoll*/
    DBG("EPoll: encounter a breakpoint,error\n");
    return 0;
}

inline void epoll_tlset(int id)
{
    __thrid = id;
}

inline int epoll_tlsid()
{
    return __thrid;
}

Buffer* buffer_realloc(Buffer* buf,int size)
{
    int new_size = size;
    Buffer* rbuf = NULL;
    new_size += HEADER_SIZE(PacketDesc);
    rbuf = buffer.alloc(new_size);
    if(rbuf){
        /* buffer form cache maybe larger new_size,
        we use actual capacity of buffer*/
        new_size = buffer_capacity(rbuf);
        rbuf->desc = MAKE_DESC(rbuf,new_size);
        rbuf->pkt = MAKE_PACKET(rbuf);
        rbuf->hdr = MAKE_HEAD(rbuf);
        rbuf->data = (char*)rbuf->pkt;
        rbuf->flag = buf->flag;
        memcpy(rbuf->pkt,buf->pkt,buf->dlen);
        rbuf->dlen = buf->dlen;
        rbuf->data += rbuf->dlen;
    }
    buffer_free(buf);
    return rbuf;
}

inline 
void buffer_add(Buffer* buf,Buffer* head)
{
    buf->next = head;
    buf->prev = head->prev;
    head->prev->next = buf;
    head->prev = buf;
}

inline 
void buffer_add_tail(Buffer* buf,Buffer* head)
{
    buf->next = head->prev->next;
    buf->prev = head->prev;
    head->prev->next = buf;
    head->prev = buf;
}

inline 
void buffer_remove(Buffer* buf)
{
    if (buf->next != buf) {
        buf->next->prev = buf->prev;
        buf->prev->next = buf->next;
        buf->prev = buf->next = NULL;
    }
}

inline 
int buffer_release(Buffer* buf)
{
    int count = 0;
    Buffer *next,*head = buf;
    do {
        next = buf->next;
        buffer_free(buf);
        buf = next;
        count++;
    } while (next != head);
    return count;
}

Buffer* buffer_next(Buffer* buf)
{
    if (buf->prev == buf && 
        buf->next == buf) {
            return NULL; /* last one */
    } else {
        return buf->next;
    }
}

int setblocking(int sock)
{
    int opts, nonb = 0;
    nonb |= O_NONBLOCK;

    opts = fcntl(sock,F_GETFL);
    if (opts < 0) {
        DBG("EPoll: get nonblocking fcntl error %d.\n",opts);
        return opts;
    }

    if (fcntl(sock, F_SETFL, opts &~ nonb) <0 ){
        DBG("EPoll: set blocking fcntl error %d.\n",opts);
    }

    return opts;
}

int setnonblocking(int sock)
{
    int opts;

    opts = fcntl(sock,F_GETFL);
    if (opts < 0) {
        DBG("EPoll: get nonblocking fcntl error %d.\n",opts);
        return opts;
    }

    opts = opts|O_NONBLOCK;
    if (fcntl(sock,F_SETFL,opts)<0) {
        DBG("EPoll: set nonblocking fcntl error %d.\n",opts);
    }

    return opts;
}

int socket_init()
{
    int ret;
    int sock;
    int optval;
    socklen_t optlen;

    /* create port socket*/
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        DBG("EPoll: cannot open socket, error %d: %s.\n", errno, strerror(errno));
        return -1;
    }

    /* Set socket send buffer size	*/
    optval = 1024*1024;
    optlen = sizeof(int);
    ret = setsockopt( sock, SOL_SOCKET, SO_SNDBUF,&optval, optlen);
    if(ret == SOCKET_ERROR){
        DBG("EPoll: Can't set the socket send buffer size, error %d: %s.\n", errno, strerror(errno));
        goto erro;
    }

    /* Set socket receive buffer size */
    ret = setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &optval, optlen);
    if(ret == SOCKET_ERROR){
        DBG("EPoll: Can't set the socket receive buffer size, error %d: %s.\n", errno, strerror(errno));
        goto erro;
    }else{
        ;//DBG("EPoll: Set socket buffer size %dk.\n",optval/1024);
    }

    optval = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    if(ret == SOCKET_ERROR){
        DBG("EPoll: Unable to set addr reuse, error %d: %s.\n", errno, strerror(errno));
        goto erro;
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen); 
    if(ret == SOCKET_ERROR){
        DBG("EPoll: Unable to set keepalive, error %d: %s.\n", errno, strerror(errno));
        goto erro;
    }
    /*
    ret = setsockopt(sock, SOL_SOCKET, SO_LINGER, &optval, optlen); 
    if(ret == SOCKET_ERROR){
    DBG("EPoll: Unable to set linger, error %d: %s.\n", errno, strerror(errno));
    goto erro;
    }
    */
    /* init socket successfully */
    return sock;

erro:
    /* close socket,if failure*/
    if(sock){
        close(sock);
    }

    return -1;
}

int setsndtimeout(int sock)
{
    int ret;
    struct timeval timeo = {3, 0};
    socklen_t len = sizeof(timeo);

    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);
    if(ret == SOCKET_ERROR){
        DBG("EPoll: Unable to SO_SNDTIMEO, error %d: %s.\n", errno, strerror(errno));
    }
    return ret;
}

int socket_bind(int sock,int port,int max)
{
    int ret;
    struct sockaddr_in addr;

    /* prepare socket address*/
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* bind socket address*/
    ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        DBG("EPoll: Bind scoket address error %d!\n",ret);
        return ret;
    }

    /* start listen socket */
    if ( (ret = listen(sock, max)) < 0) {
        DBG("EPoll: Listen the socket error %d!\n",ret);
        return ret;
    }

    /* bind socket successfully */
    return ret;
}

int socket_connect(int fd,unsigned int ip,int port)
{
    int ret=-1,retry=0;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);

    while (retry++ < 1){
        DBG("EPoll: Connecting %s:%d...\n",inet_ntoa(addr.sin_addr),port);
        ret = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
        if(ret == SOCKET_ERROR){
            DBG("EPoll: Can't connect peer %s:%d, error %d: %s.\n",
                inet_ntoa(addr.sin_addr), port,
                errno, strerror(errno));
            /* usleep(300); */
        }else{
            DBG("EPoll: Connect %s fd %d ok.\n",inet_ntoa(addr.sin_addr),fd);
            break;
        }
    }
    return ret;
}

int socket_connectex(int fd,unsigned int ip,int port,int sec)
{
    int ret = -1;
    fd_set set;
    int optval;
    socklen_t optlen;
    struct timeval tv;
    struct sockaddr_in addr;

    /* set socket address*/
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);

    /* set nonblocking socket*/
    setnonblocking(fd);
    DBG("EPoll: Connecting %s:%d...\n",inet_ntoa(addr.sin_addr),port);
    ret = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
    if (ret == 0) {
        DBG("EPoll: Connecting %s:%d ok\n", inet_ntoa(addr.sin_addr),port);
        return ret;
    }
    
    /* socket connect failure*/
    if (errno != EINPROGRESS) {
        DBG("EPoll: Connecting %s:%d return EINPROGRESS\n", inet_ntoa(addr.sin_addr),port);
        return ret;
    }

    /* set wait timeout value*/
    FD_ZERO(&set);
    FD_SET(fd, &set);
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    /* start wait sock connection*/
    if(select(fd+1, NULL, &set, NULL, &tv) > 0){
        optlen = sizeof(int);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&optval), &optlen);
        if(optval == 0) 
            ret = 0;
    }

    return ret;
}

int socket_read(int fd,void *buffer,unsigned int length)
{
    unsigned int nleft;
    int nread;
    char *ptr;
    ptr = (char *)buffer;
    nleft = length;

    while(nleft > 0)
    {
        if((nread = read(fd, ptr, nleft)) < 0){
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0){
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return length - nleft;
}

int socket_write(int fd,char *buffer,unsigned int length)
{
    unsigned int nleft;
    int nwritten;
    const char *ptr;
    ptr = (const char *)buffer;
    nleft = length;

    while(nleft > 0){
        if((nwritten = send(fd, ptr, nleft,0)) <= 0){
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return length;
}

inline 
int epoll_lock(struct epoll_server* srv)
{
    int ret;
    ret = pthread_mutex_lock(&srv->lock);
    if(ret != 0)
        epoll_breakpoint();
    return ret;
}

inline 
int epoll_unlock(struct epoll_server* srv)
{
    return pthread_mutex_unlock(&srv->lock);
}

inline
void epoll_ref(struct epoll_client* client)
{
    client->refcnt++;
}

void epoll_unref(struct epoll_server* srv,struct epoll_client* client)
{
    client->refcnt--;

    /* death probe for reconnection*/
    if(client->state == EPOLL_CLOSED &&
        client->ctype == EPOLL_INNER_CONN)
    {
        /* whether the ip already in list*/
        bool exist = false;
        struct epoll_client* node;
        list_for_each_entry(node,srv->black_list,link){
            if(node->ip == client->ip){
                exist = true;
                break;
            }
        }

        if(exist == false){
            /* whether the ip already in probing*/
            if(!(client->flag & EPOLL_PROBING)){
                client->flag |= EPOLL_PROBING;
                /* add closed client to blacklist*/            
                struct timeval now;
                struct epoll_client* host;
                gettimeofday(&now,NULL);
                tv_setv(&client->stamp,&now);
                client->deadline = now.tv_sec;
                list_add_tail((struct list_head*)client,srv->black_list);
                srv->bklist_cnt++;
                client->refcnt++;
                /* check client whether mapping*/
                host = hash_get(srv->ipmap,client->ip);
                if(host == client)
                hash_remove(srv->ipmap,client->ip);
                epoll_probe(srv,client->ip);
            }
        }
    }

    /* no more one using,unmap fd/ip*/
    if (client->refcnt == 0 &&
        client->state == EPOLL_CLOSED) 
    {
        /* remove client fd map*/
        struct epoll_client* node;
        hash_remove(srv->fdmap,client->fd);
        if(client->flag & EPOLL_MAPIP){
            node = hash_get(srv->ipmap,client->ip);
            if(node == client)
            hash_remove(srv->ipmap,client->ip);
        }

        /* unregister socket events*/
        if(client->event != 0)
            epoll_del(srv,client->fd);

        /* close socket handle*/
        char ipstr[20];
        ip_htoa(client->ip, ipstr, sizeof(ipstr));
        DBG("EPoll: close fd %d, ip %s,%x\n",client->fd,ipstr,client);
        if(client->flag & EPOLL_PKILLED){
            setblocking(client->fd);
            //epoll_pkill(srv,client->fd);
        }

        close(client->fd);
        client->fd = -1; 
        free(client);
    }
}

/*
*  When the server connect each other same time, must release one connection, 
*  here we adopt ip value comparison, if accpet ip < localip chose accept 
*  connection,and wait peer send close connection request.
*/
int epoll_ipmap(struct epoll_server* srv,int fd,unsigned int ip)
{
    int ret;
    uint32 localip;
    struct epoll_client *client;

    /* exist active connection*/
    bool ismap = true;
    client = hash_get(srv->ipmap,ip);
    if (client != NULL) {
        localip = getlocaluip();
        if(localip > ip){
            /* keep accept connection,close previous
               exist duplicate reconnection*/
            if (client->ctype == EPOLL_CONNECT)
                client->flag |= EPOLL_PKILLED;

            /*avoid remove ip map,when unref*/
            client->flag &= ~EPOLL_MAPIP;
            client->state = EPOLL_CLOSED;

            /*avoid close call fail callback*/
            client->ctype = EPOLL_OUTER_CONN;
            if (client->refcnt == 0){
                client->refcnt++;
                epoll_unref(srv,client);
            }

            /* map the fd client to ip*/
            epoll_clarify(srv,ip);
            hash_remove(srv->ipmap,ip);
            client = hash_get(srv->fdmap,fd);
            if(client != NULL){
                client->flag |= EPOLL_MAPIP;
                client->ctype = EPOLL_INNER_CONN;
                hash_set(srv->ipmap,ip,client);
                DBG("EPoll: map fd %d to ip %d\n",fd,ip);
            }
        }
        else if(localip < ip){
            /*keep previous connection,and wait the 
              peer close the accept connection*/
            ismap = false; /*not map fd's ip*/
            epoll_clarify(srv,ip);
            client = hash_get(srv->fdmap,fd);
            if(client != NULL){
                client->flag &= ~EPOLL_MAPIP;
                client->ctype = EPOLL_OUTER_CONN;
            }
            DBG("EPoll: not map fd %d to ip %d\n",fd,ip);
        }
        else{ /*local connect self*/
            ismap = false; /*not map ip*/
            DBG("EPoll: local ip connection,error!\n");
            client = hash_get(srv->fdmap,fd);
            if(client != NULL){
                client->flag &= ~EPOLL_MAPIP;
                client->state = EPOLL_CLOSED;
                client->ctype = EPOLL_OUTER_CONN;
                if (client->refcnt == 0){
                    client->refcnt++;
                    epoll_unref(srv,client);
                }
            }
        }
    }
    else{
        DBG("EPoll: map fd %d to ip %d\n",fd,ip);
        client = hash_get(srv->fdmap,fd);
        hash_set(srv->ipmap,ip,client);
        client->flag |= EPOLL_MAPIP;
        client->ctype = EPOLL_INNER_CONN;
    }

    return ret;
}

int epoll_add(struct epoll_server* srv,int fd,unsigned int ip)
{
    int ret;
    struct epoll_client *client;
    struct epoll_event event = {0, {0}};    

    /* register socket events*/
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    ret = epoll_ctl(srv->epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if (ret == -1) {
        DBG("EPoll: epoll_ctl add error!\n");
    }

    /* create accept connection,only map fd*/
    client = malloc(sizeof(struct epoll_client));
    if (client != NULL) {
        memset(client,0,sizeof(struct epoll_client));
        client->fd = fd;
        client->ip = ip;
        client->port = srv->port;
        client->type = EPOLL_ACCEPT;
        hash_set(srv->fdmap,fd,client);
        client->event = event.events;
        /*default unknow identity,unmap ip*/
        //client->flag |= EPOLL_MAPIP;
        client->ctype = EPOLL_OUTER_CONN;
        DBG("EPoll: map fd %d to client %x.\n",fd,client);
    }else{
        ret = -1; /* no memory*/
        DBG("EPoll: fail to alloc client,error!\n");
    }

    return ret;
}

int epoll_del(struct epoll_server* srv,int fd)
{
    int ret;
    struct epoll_client *client;
    struct epoll_event event = {0, {0}};

    /* unregister socket events*/
    event.data.fd = fd;
    event.events = 0;
    ret = epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL,fd,&event);
    if (ret == -1) {
        DBG("EPoll: epoll_ctl del error!\n");
        epoll_breakpoint();
    }

    /* flag client is closed*/
    client = hash_get(srv->fdmap,fd);
    if (client != NULL) {
        client->event = 0;
        if(client->flag & EPOLL_MAPIP){
            struct epoll_client* node;
            node = hash_get(srv->ipmap,client->ip);
            if(node == client)
            hash_remove(srv->ipmap,client->ip);
        }        
        client->state = EPOLL_CLOSED;
    }

    return ret;
}

int epoll_wmod(struct epoll_server* srv,struct epoll_client *client)
{
    int ret;

    struct epoll_event event = {0, {0}};
    event.data.fd = client->fd;
    event.events = EPOLLIN | EPOLLET;
    client->event = event.events;

    ret = epoll_ctl(srv->epoll_fd,EPOLL_CTL_MOD,client->fd,&event);
    if(ret != 0) {
        DBG("EPoll: epoll_ctl mod write error!\n");
        epoll_breakpoint();
    }

    return ret;
}

int epoll_rmod(struct epoll_server* srv,struct epoll_client *client)
{
    int ret;

    struct epoll_event event = {0, {0}};
    event.data.fd = client->fd;
    event.events = EPOLLIN|EPOLLOUT|EPOLLET;
    client->event = event.events;

    ret = epoll_ctl(srv->epoll_fd,EPOLL_CTL_MOD,client->fd,&event);
    if (ret !=0 ) {
        DBG("EPoll: epoll_ctl mod write error!\n");
        epoll_breakpoint();
    }

    return ret;
}

int epoll_config(struct epoll_server* srv)
{
    int nfiles;
    struct rlimit rl;

    /* disable environment variable is set */
    if (getenv("EVENT_NOEPOLL")) {
        WARN("EPoll: getenv EVENT_NOEPOLL is set.\n");
        return -1;
    }

    /* get epoll fd limitation*/
    nfiles = 0; /*default zero*/
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 &&
        rl.rlim_cur != RLIM_INFINITY) {
            nfiles = rl.rlim_cur - 1;
    }

    /* init the epoll max event*/
    srv->nevents = nfiles;

    /* init the epoll timeout*/
    srv->timeout = 500;

    /* init read buf cache num*/
    srv->rbuf_limit = 100;

    /* init write buf cache num*/
    srv->wbuf_limit = 500;

    /* init n threads do read*/
    srv->reader_n = 1;

    /* init n threads do write*/
    srv->writer_n = 1;

    /* init n threads do schedule*/
    srv->scheduler_n = 2;  

    /* init delay counter*/
    srv->delay = 1;

    /* init blacklist counter*/
    srv->bklist_cnt = 0;

    /* init threadpool threads*/
    srv->thrmax = EPOLL_MAX_POOL_THREADS;

    /* init upcall threads*/
    srv->flags = EPOLL_UPCALL_TPOOL;

    /* init read balance*/
    srv->flags |= EPOLL_READ_BALANCE;

    /* init read balance*/
    srv->flags |= EPOLL_WRITE_DIRECT;
    
    /* init write behavior*/
    srv->flags |= EPOLL_WRITE_LOOP;

    /* init server type*/
    srv->type = EPOLL_DEFAUL_TYPE;

    return nfiles;
}

int epoll_init(struct epoll_server* srv)
{
    int ret,epfd;
    struct epoll_event event = {0, {0}};

    /* create epoll handle*/
    epfd = epoll_create(srv->nevents);
    if (epfd == -1) {
        if (errno != ENOSYS)
            DBG("EPoll: epoll_create error!\n");
        return (epfd);
    }

    /* register listen event */
    event.data.fd = srv->listen_fd;
    event.events = EPOLLIN | EPOLLET;

    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,srv->listen_fd,&event);
    if ( ret == -1){ /* fail to add listen fd*/
        close(epfd);
        return ret;
    }

    /* return epoll handle*/
    srv->epoll_fd = epfd;
    return epfd;
}

int epoll_dispatch(struct epoll_server* srv,int fd)
{
    int ret = 0;
    struct epoll_task* task;

    /* create new read task*/
    task = malloc(sizeof(struct epoll_task));
    if (task == NULL) {
        DBG("EPoll: fail to malloc read task,error!\n");
        epoll_breakpoint();
        return -1;
    }

    /* add new read task*/
    task->fd = fd;
    ret = queue_push(srv->rtask,task);
    return ret;
}

int epoll_activate(struct epoll_server* srv,int fd)
{
    int ret = 0;
    struct epoll_task* task;
    struct epoll_client* client;

    /* switch to read mod*/
    epoll_lock(srv);
    client = hash_get(srv->fdmap,fd);
    if (client != NULL){
        client->wttl = time(NULL)-client->wttl;
        DBG("EPoll: write request time %d\n",client->wttl);
        epoll_wmod(srv,client);
    }else{
        DBG("EPoll: fail to find fd client,error!\n");
        epoll_breakpoint();
    }
    epoll_unlock(srv);

    /* create new read task*/
    task = malloc(sizeof(struct epoll_task));
    if (task == NULL) {
        DBG("EPoll: fail to malloc write task,error!\n");
        epoll_breakpoint();
        return -1;
    }

    /* add new read task*/
    task->fd = fd;
    ret = queue_push(srv->wtask,task);

    return ret;
}

void* epoll_loop(void *args)
{
    int i,fd,nfds;
    unsigned int ip;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct epoll_server* srv;

    srv = (struct epoll_server*)args;
    addrlen = sizeof(struct sockaddr);

    while(srv->running)
    {
        nfds = epoll_wait(srv->epoll_fd,srv->events,srv->nevents,srv->timeout);
        if (nfds == -1) {
            if (errno != EINTR) {
                DBG("EPoll: epoll_wait EINTR,error!\n");
                return 0; /*interrupt*/
            } else {
                //DBG("EPoll: epoll_wait!\n");
                if(!srv->running)
                    break;
                continue;
            }
        }

        for (i=0; i<nfds; i++)
        {
            if(srv->events[i].data.fd == srv->listen_fd)
            {
                fd = accept(srv->listen_fd,(struct sockaddr*)&addr, &addrlen);
                if (fd < 0) {
                    DBG("EPoll: accept client error!\n");
                }
                else{
                    ip = ntohl(addr.sin_addr.s_addr);
                    DBG("EPoll: accept %s fd %d\n",inet_ntoa(addr.sin_addr),fd);

                    /* register socket events*/
                    if(ip){
                        /* set fd non blocking*/
                        setnonblocking(fd);

                        /* add epoll fd listen*/
                        epoll_lock(srv);
                        epoll_add(srv,fd,ip);
                        epoll_unlock(srv);

                    }else{
                        close(fd);
                    }
                }
            }
            else
            {
                if(srv->events[i].events & EPOLLIN)
                {
                    /* check readable fd handle*/
                    if ((fd = srv->events[i].data.fd) < 0)
                        continue;

                    /* use single thread read/write*/
                    if(srv->reader_n == 0)
                        epoll_reader(srv,fd);

                    /* multi write,activate write task*/
                    else
                        epoll_dispatch(srv,fd);
                }
                if(srv->events[i].events & EPOLLOUT)
                {
                    /* check writable fd handle*/
                    if ((fd = srv->events[i].data.fd) < 0)
                        continue;

                    /* use single thread read/write*/
                    if(srv->writer_n == 0)
                        epoll_writer(srv,fd);

                    /* multi write,activate write task*/
                    else
                        epoll_activate(srv,fd);
                }
            }
        }
    }
    return (0);
}

void* epoll_read_thr(void *args)
{
    int fd;
    struct epoll_task *task;
    struct epoll_server* srv;
    srv = (struct epoll_server*)args;

    while(srv->running)
    {
        /* pop read event task */
        task = NULL;
        queue_pop(srv->rtask,(void**)&task);
        if(srv->rtask->terminated)
            break;

        /* check task pop*/
        if(task == NULL)
            continue;

        /* get read event fd */
        fd = task->fd;
        free(task);

        /* call event reader */
        epoll_reader(srv,fd);
    }
    return 0;
}

void* epoll_write_thr(void *args)
{
    int fd;
    struct epoll_task *task;
    struct epoll_server* srv;  
    srv = (struct epoll_server*)args;

    while(srv->running)
    {
        /* pop write event fd*/
        task = NULL;
        queue_pop(srv->wtask,(void**)&task);
        if(srv->wtask->terminated)
            break;

        /* check task pop*/
        if(task == NULL)
            continue;

        /* get write event fd*/
        fd = task->fd;
        free(task);

        /* call event writer*/
        epoll_writer(srv,fd);
    }
    return 0;
}

int epoll_read(struct epoll_server* srv,int fd,Buffer* buf)
{
    char *ptr;
    int n,len,nleft;

    ptr = buf->data;
    nleft = buf->dlen;
    len = buf->dlen;

    while(nleft > 0)
    {
        n = read(fd, ptr, nleft);
        if (n  < 0) {
            /* interrupt signal*/
            if(errno == EINTR){ /*4*/
                continue;
            }
            /* no more data */
            if(errno == EAGAIN){ /*11*/
                buf->flag = EAGAIN;
                /*DBG("EPoll: recv EAGAIN\n");*/
                break;
            }
            /* socket error*/       
            DBG("EPoll: recv %d error %d,%s\n",fd,errno,strerror(errno));
            return -1;           
        } 
        else if(n == 0){
            /* socket closed*/
            DBG("EPoll: read %d ,socket closed.\n",fd);
            return -1;
        } 
        else{
            /* read success*/
            nleft -= n;
            ptr += n;
        }
    }

    return len - nleft;
}

int epoll_identify(struct epoll_server* srv,struct epoll_client *client,Buffer* buf)
{
    int ret = 0;

    /* check the connect magic*/
    if(!(client->flag & EPOLL_FIRST)){ /*first time*/
        epoll_lock(srv);
        client->flag |= EPOLL_FIRST;
        if(buf->pkt->seq == 0xBEFE){
            if(buf->pkt->flag & SRV_INNER){
                if(client->type == EPOLL_ACCEPT){
                    epoll_ipmap(srv,client->fd,client->ip);
                }
            }
            buf->type = EPOLL_ID_BUF;
        }
        else{
            /* illegal connection,close it*/
            client->state = EPOLL_CLOSED;
            DBG("EPoll: illegal connection fd %d,ip%d\n",client->fd,client->ip);
            ret = -1; 
        }
        epoll_unlock(srv);
    }

    if(buf->pkt->flag & URG_STOP){
        DBG("EPoll: receive pk close infor\n");
        client->flag |= EPOLL_OUTER_CONN;
        buf->type = EPOLL_ID_BUF;
    }

    return ret;
}


int packet_recv(struct epoll_server* srv,struct epoll_client *client)
{    
    Buffer* buf;
    int desc_size,head_size,total_size;
    int n,len,size,fd,capacity;

    buf = client->rbuf;
    fd = client->fd;
    desc_size = HEADER_SIZE(PacketDesc);
    head_size = size = HEADER_SIZE(Packet);

    /* receive packet head*/
    if(buf->dlen < size){
        len = buf->dlen;
        size -= len;
        buf->dlen = size;
        n = epoll_read(srv,fd,buf);
        if (n == -1) {  /*read error*/
            client->flag |= EPOLL_RBROKEN;
            client->rstate |= EPOLL_ERROR;
            return n;
        }
        /*socket fd no more data?*/         
        if(buf->flag == EAGAIN){
            //DBG("EPoll: recv again %d\n",client->write);
            client->rstate |= EPOLL_EAGAIN;
            buf->dlen = len+n;
            buf->data += n;
            return n;
        }
        if(n != size){ /*not enough,this not exist*/
            buf->dlen = len+n;
            buf->data += n;
            return n;
        }
        buf->dlen = len+n;
        buf->data += n;
    }

    TRACE("EPoll: recv fd %d buf%X{seq%d,flag %d,win %d,size %d}\n",
        fd,buf,buf->pkt->seq,buf->pkt->flag,buf->pkt->win,buf->pkt->size);

    /* verify client and buffer*/
    if(epoll_identify(srv,client,buf) !=0)
        return -1;

    /* check buffer capacity */
    capacity = buffer_capacity(buf)-desc_size;
    total_size = head_size+buf->pkt->size;
    if(total_size > 104857600){
        TRACE("EPoll: recv %d pkt size %d may error\n",fd,buf->pkt->size);
        epoll_breakpoint();
    }

    if(total_size > capacity){
        client->rstate &= ~EPOLL_NOMEM;
        buf = buffer_realloc(buf,total_size);
        if(buf == NULL){
            client->rstate |= EPOLL_NOMEM;
            DBG("EPoll: fail to realloc mem error\n");
            return -1;
        }
        client->rbuf = buf;
    }

    /* receive packet body*/
    if(buf->dlen < total_size){
        len = buf->dlen;
        size = total_size-buf->dlen;
        buf->dlen = size;
        n = epoll_read(srv,fd,buf);
        if (n == -1){  /*read error*/
            client->flag |= EPOLL_RBROKEN;
            client->rstate |= EPOLL_ERROR;
            return n;
        }
        /*socket fd no more data?*/         
        if(buf->flag == EAGAIN){ 
            //DBG("EPoll: recv again %d\n",client->write);
            client->rstate |= EPOLL_EAGAIN;
            buf->dlen = len+n;
            buf->data += n;
            return n;
        }
        if(n != size){/*not enough,error*/
            buf->dlen = len+n;
            buf->data += n;
            return n;
        }
        buf->dlen = len+n;
        buf->data = (char*)buf->pkt;
        //DBG("EPoll: recv dlen %d\n",buf->dlen);
    }

    return buf->dlen;
}

int epoll_recv(struct epoll_server* srv,struct epoll_client *client)
{
    int n,size;
    struct Buffer *buf;

    /* check previous buf*/
    if (client->rbuf == NULL) {
        /* alloc receive buffer*/
        client->rstate &= ~EPOLL_NOMEM;
        buf = buffer.alloc(BIG_ENOUGH);
        if(buf == NULL){
            client->rstate |= EPOLL_NOMEM;
            DBG("EPoll: receiver fail to alloc mem,error\n");
            return -1;
        }

        /* build buffer members,here we use actual capacity 
        of buffer,let desc place at top of buffer*/
        size = buffer_capacity(buf);
        buf->desc = MAKE_DESC(buf,size);
        buf->pkt = MAKE_PACKET(buf);
        buf->hdr = MAKE_HEAD(buf);
        buf->data = (char*)buf->pkt;
        buf->flag = 0;
        buf->dlen = 0;
        client->rbuf = buf;        
    }

    /* receive packet head data*/
    client->rbuf->flag = 0;
    client->rstate &= ~EPOLL_ERROR;
    client->rstate &= ~EPOLL_EAGAIN;
    n = packet_recv(srv,client);
    return n;
}

int epoll_reader(struct epoll_server* srv,int fd)
{
    int n;
    struct Buffer* buf;
    struct sockaddr_in* addr;
    struct epoll_client *client;

    /* get fd client ptr */
    epoll_lock(srv);
    client = hash_get(srv->fdmap,fd);
    if (client != NULL){
        epoll_ref(client);
        client->rstate |= EPOLL_READING;
    }
    epoll_unlock(srv);
    if (client == NULL){
        DBG("EPoll: recv not found fd %d client,error!\n",fd);
        epoll_breakpoint();
        return -1;
    }

    do{
        /* receive client data*/
        n = epoll_recv(srv,client);
        TRACE("EPoll: recv fd %d %d bytes\n",fd,n);
        /* error,closed,nomem*/
        if (n == -1){
            /* delete epoll event*/
            if(client->rbuf){
                buffer_free(client->rbuf);
                client->rbuf = NULL;
            }
            epoll_lock(srv);
            epoll_del(srv,fd);
            epoll_unref(srv,client);
            epoll_unlock(srv);
            DBG("EPoll: close client connect fd %d!\n",fd);
            return -1; /*exit*/
        }

        /* handle receive buffer*/
        if (client->rbuf->flag != EAGAIN) 
        {
            /*deliver received buffer*/
            buf = client->rbuf;
            client->rbuf = NULL;

            /*update peer window size*/
            client->win = buf->pkt->win;
            gettimeofday(&client->stamp,NULL);
            if(client->win > CONGEST_THRESHOLD)
                client->delay++;
            else
                client->delay = 1;

            TRACE("EPoll: recv a packet %d client %x,dlen %d\n",
                fd,client,buf->dlen);

            /* build from address*/
            addr = &buf->desc->from;
            addr->sin_addr.s_addr = htonl(client->ip);
            addr->sin_port = htons(client->port);
            buf->dst = client->ip;
            buf->src = fd;
            buf->owner = srv;
            srv->ops->input(srv,buf);

            /* if use read balance policy,we return*/
            if(srv->flags & EPOLL_READ_BALANCE){
                epoll_dispatch(srv,fd);
                break;
            }
        }

        /* check receive status*/
    }while(!(client->rstate & EPOLL_EAGAIN));

    /* next to write event*/
    epoll_lock(srv);
    client->rstate &= ~EPOLL_READING;
    epoll_unref(srv,client);
    epoll_unlock(srv);

    return 0;
}

int epoll_writer(struct epoll_server* srv,int fd)
{
    int wtime;
    int n,write;
    struct Buffer* buf;
    struct epoll_client *client;

    /* get fd client ptr*/       
    buf = NULL;
    epoll_lock(srv);
    client = hash_get(srv->fdmap,fd);
    if (client != NULL) {
        epoll_ref(client);
        if (client->wbuf) {
            buf = client->wbuf;
            client->wbuf = NULL;
            buf->prev = buf->next = buf;
            client->wstate |= EPOLL_WRITING;
            write = 1;
        }else{
            DBG("EPoll: write %d wbuf is null,error!\n",fd);
            epoll_unref(srv,client);
        }
    }
    epoll_unlock(srv);

    /* check client & buf ptr*/
    if(client == NULL || buf == NULL){
        if(client) 
            client->wflag = WEVNET_UNSET;
        else
            DBG("EPoll: writer not found fd %d client,error!\n",fd);
        return -1;
    }

    /* write(send) buffer out*/ 
    wtime = time(NULL);
    DBG("EPoll: client fd%d write -> in (%d),t%d\n",client->fd,buf->dlen,0);
    if(buf->dlen > 2*1024*1024){
        DBG("EPoll: send pkt size %d may error\n",buf->pkt->size);
        epoll_breakpoint();
    }

    buf->flag = 0;
    buf->pkt->win = srv->wbuf_cnt;
    client->wstate = 0;
    n = epoll_write(srv,fd,buf);

    /* check the result of write*/
    epoll_lock(srv);
    if(n == -1){
        DBG("EPoll: send fd %d,error\n",client->fd);
        client->flag |= EPOLL_WBROKEN;
        client->wstate |= EPOLL_ERROR;
        client->state = EPOLL_CLOSED;
        /* reset the buffer data and dlen*/
        buf->data = (char*)buf->pkt;
        buf->dlen = buffer.size(buf);
        epoll_breakpoint();

        /* Note:
         * for reconnection requirement,we delay buffer free till 
         * detect peer dead,so push back the buffer to server's  
         * buffer queue. comment out epoll_wfree(srv,buf);
         */
        DBG("EPoll: recycle buffer,delay free,error\n");
        epoll_wcycle(srv,buf);
    }
    else{
        /* write socket fd again*/
        if(buf->flag == EAGAIN){
            buf->dlen -= n;
            buf->data += n;
            buf->priority = PRIORITY_HIGHEST;
            client->wstate |= EPOLL_EAGAIN;
            epoll_wcycle(srv,buf);
        }
        else{
            /* write successfully,signal queue*/
            //client->wttl = 0;
            epoll_wfree(srv,buf,EPOLL_WRITE_OK);
            buf = NULL;
            DBG("EPoll: write buffer ok\n");
            epoll_wsignal(srv);
        }
    }

    wtime = time(NULL) - wtime;
    DBG("EPoll: client fd%d write -> out(%d),t%d\n",client->fd,buf ? buf->dlen: 0,wtime);
    client->wflag = WEVNET_UNSET;
    client->wstate &=~EPOLL_WRITING;
    epoll_unref(srv,client);
    epoll_unlock(srv);

    return n;
}

int epoll_write(struct epoll_server* srv,int fd,Buffer* buf)
{
    char *ptr;
    int n,len,nleft;

    ptr = buf->data;
    nleft = buf->dlen;
    len = buf->dlen;

    while(nleft > 0){
        n = write(fd, ptr, nleft);
        if ( n < 0) {
            /* interrupt signal*/
            if(errno == EINTR){
                DBG("EPOLL: send EINTR\n");
                continue;
            }
            /* fd write full*/
            if(errno == EAGAIN){
                /*DBG("EPOLL: send EAGAIN\n");*/
                buf->flag = EAGAIN;
                break;
            }
            /* other errors*/
            DBG("EPOLL: send fd %d error %d,%s\n",fd,errno,strerror(errno));
            return -1;
        }
        else if (n == 0){
            /* socket closed*/
            DBG("EPOLL: send fd %d close\n",fd);
            return -1;
        } else {
            /* write success*/
            nleft -= n;
            ptr += n;
        }
    }

    return len - nleft;
}

int epoll_writebuf(struct epoll_server* srv,struct epoll_client *client,struct Buffer* buf)
{
    int n,fd;

    fd = client->fd;
    buf->pkt->win = srv->wbuf_cnt;
    buf->flag = 0;
    client->wstate = 0;

    n = epoll_write(srv,fd,buf);
    if(n == -1){ /*peer closed*/
        client->flag |= EPOLL_WBROKEN;
        client->wstate |= EPOLL_ERROR;
        client->state = EPOLL_CLOSED;

        /*reset buffer ptr and len*/
        buf->data = (char*)buf->pkt;
        buf->dlen = buf->desc->size;
        epoll_wcycle(srv,buf);
        epoll_breakpoint();
    }
    else{
        /* write socket fd again*/
        if(buf->flag == EAGAIN){
            buf->dlen -= n;
            buf->data += n;
            buf->priority = PRIORITY_HIGHEST;
            client->wstate |= EPOLL_EAGAIN;
            epoll_wcycle(srv,buf);
        }
        else{
            /* write successfully*/
            client->wttl = 0;
            epoll_wfree(srv,buf,EPOLL_WRITE_OK);
            client->wbuf = NULL;
            epoll_wsignal(srv);
        }
    }
    return n;
}

int epoll_hello(struct epoll_server* srv,int fd,short int type)
{
    int size;
    struct Buffer* buf;

    /* build hello packet buffer*/
    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    /* set server inner hello flag*/
    buf->dlen = sizeof(struct epoll_packet);
    if(type == EPOLL_INNER_CONN)
        buf->pkt->flag |= SRV_INNER;
    else
        buf->pkt->flag |= SRV_OUTER;

    buf->pkt->seq = 0xBEFE; /*magic*/
    buf->pkt->win = srv->wbuf_cnt;
    buf->pkt->size = buf->dlen - HEADER_SIZE(Packet);
    buf->desc->size = buf->dlen;
    DBG("EPoll: send identity to fd %d peer.\n",fd);

    /* send hello packet buffer*/
    return socket_write(fd,buf->data,buf->dlen);
}

int epoll_pkill(struct epoll_server* srv,int fd)
{
    int size;
    struct Buffer* buf;

    /* build pk information packet*/
    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    /* set pk packet head*/	
    buf->dlen = sizeof(struct epoll_packet);
    buf->pkt->flag |= URG_STOP;
    buf->pkt->win = srv->wbuf_cnt;
    buf->pkt->size = buf->dlen - HEADER_SIZE(Packet);
    buf->desc->size = buf->dlen;
    buf->owner = srv;
    DBG("EPoll: send fd killed infor.\n",fd);

    /* send pk information packet*/
    return socket_write(fd,buf->data,buf->dlen);
}

struct epoll_client* 
epoll_connect(struct epoll_server* srv,unsigned int ip,int port)
{
    int ret,fd;
    struct epoll_client* conn;
    struct epoll_event event = {0, {0}};

    /* create epoll_client of connect*/
    conn = malloc(sizeof(struct epoll_client));
    if(conn == NULL)
        return conn;

    /* set socket ip and port*/
    memset(conn,0,sizeof(struct epoll_client));
    conn->ip = ip;
    conn->port = port;
    conn->state = EPOLL_CLOSED;
    conn->type = EPOLL_CONNECT;

    /* identify the server type*/
    if(srv->type == EPOLL_DEFAUL_TYPE)
        conn->ctype = EPOLL_INNER_CONN;
    else
        conn->ctype = EPOLL_OUTER_CONN;

    /* create clinet connect socket*/
    fd = socket_init();
    if(fd < 0){
        free(conn);
        return NULL;
    }

    /* connect to the server*/
    conn->fd = fd;
    ret = socket_connect(fd,ip,port);
    if(ret < 0){
        free(conn);
        close(fd);
        return NULL;
    }

    /* send identification information*/
    if(epoll_hello(srv,fd,conn->ctype) <= 0){
        DBG("EPoll: say hello error!\n");
        epoll_breakpoint();
    }

    /* hash ip/fd epoll_client*/
    conn->state = EPOLL_CONNECTED;
    conn->wstate = EPOLL_CONNECTED;
    conn->flag |= EPOLL_MAPIP;
    conn->flag |= EPOLL_FIRST;
    hash_set(srv->ipmap,ip,conn);
    hash_set(srv->fdmap,fd,conn);
    DBG("EPoll: map the fd %d to %d,%x!\n",fd,ip,conn);

    /* init epoll event*/
    setnonblocking(fd);
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    conn->event = event.events;
    ret = epoll_ctl(srv->epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if ( ret == -1){ /* fail to add client fd*/
        DBG("EPoll: epoll_ctl add client error!\n");
        epoll_breakpoint();
    }

    /* return client conn*/
    return conn;
}

void epoll_close(struct epoll_server* srv)
{
    if (srv->listen_fd >= 0)
        close(srv->listen_fd);
    if (srv->epoll_fd >= 0)    
        close(srv->epoll_fd);
    if (srv->events != NULL)
        free(srv->events);
}

void epoll_free_fdmap(struct epoll_server* srv)
{
    HashPair* hash;
    struct epoll_client* client;

    hash = hash_first(srv->fdmap);
    while (hash) {
        client = (struct epoll_client*)hash->value;
        if (client) {
            if (client->rbuf)
                buffer_free(client->rbuf);        
            if (client->wbuf)
                buffer_free(client->wbuf);
            hash_remove(srv->fdmap,client->fd);
            free(client);
        }
        hash = hash_next(srv->fdmap);
    }
}

void epoll_free_idmap(struct epoll_server* srv)
{
    int i;
    HashPair* hash;
    struct epoll_ipbuf* ipbuf;

    hash = hash_first(srv->idmap);
    while (hash) {
        ipbuf = (struct epoll_ipbuf*)hash->value;
        if (ipbuf) {
            if (ipbuf->list){
                for(i=0;i < MAX_PRIORITY_LEVEL;i++)
                    list_for_each_free(ipbuf->list[i]);
                free(ipbuf->list);
            }
            hash_remove(srv->idmap,ipbuf->ip);
            free(ipbuf);
        }
        hash = hash_next(srv->idmap);
    }
}

struct epoll_server* eserver_create(int port)
{
    int size;

    /* create epoll server */
    struct epoll_server* srv = NULL;
    srv = malloc(sizeof(struct epoll_server));
    if(srv != NULL)
    {
        /* init the epoll server*/
        memset(srv,0,sizeof(struct epoll_server));        
        srv->port = port;

        /* config epoll resource*/
        if(!epoll_config(srv)){
            free(srv);
            return 0;
        }

        /* alloc the event array*/
        size = sizeof(struct epoll_event)*srv->nevents;
        srv->events = (struct epoll_event*)malloc(size);

        /* alloc fd and ip map*/
        srv->ipmap = hash_new(10);
        srv->fdmap = hash_new(10);
        srv->idmap = hash_new(10);

        /* alloc read/write queue*/
        srv->rtask = queue_create(srv->nevents);
        srv->wtask = queue_create(srv->nevents);
        srv->rqueue = queue_create(srv->rbuf_limit);
        srv->rtask->flags = 1;

        /*alloc buf priority queue*/
        srv->iplink = LIST_HEAD_NEW();
        INIT_LIST_HEAD(srv->iplink);
        srv->ipcur = (struct epoll_ipbuf*)srv->iplink;

        /* alloc blacklist list*/
        srv->black_list = LIST_HEAD_NEW();
        INIT_LIST_HEAD(srv->black_list);
        
        /* alloc buffer free queue/sem*/
        srv->free_queue = fifo_new();
        srv->free_sem = SEM_NEW(0);

        /* alloc probe queue/sem*/
        srv->probe_queue = fifo_new();
        srv->probe_sem = SEM_NEW(0);

        /* alloc thread pool*/
        srv->threadpool = alloc_threadpool();
    }
    return srv;
}

int eserver_init(struct epoll_server* srv)
{
    int ret = 0;

    /* init listen socket*/
    srv->listen_fd = socket_init();
    if(srv->listen_fd < 0)
        return -1;

    /* set fd non blocking*/
    setnonblocking(srv->listen_fd);

    /* bind and listen sock
    if(socket_bind(srv->listen_fd,
        srv->port,srv->nevents) <0)
        return -1;*/

    /* init epoll handle*/
    if(epoll_init(srv) < 0)
        return -1;

    /* init server lock */
    if(pthread_mutex_init(&srv->lock, NULL) != 0)
        return -1;

    /* init write lock */
    if(pthread_mutex_init(&srv->wlock, NULL) != 0)
        return -1;

    /* init server write condition */
    if(pthread_cond_init(&srv->wcond, NULL) != 0)
        return -1;

    /* init server write condition */
    if(pthread_cond_init(&srv->wfull, NULL) != 0)
        return -1;

    /* server operations*/
    srv->ops = &_eserver_ops;

    /* success return */
    return ret;
}

int eserver_start(struct epoll_server* srv)
{
    int i;

    /* start run epoll server*/
    srv->running = 1;

    /* bind and listen sock*/
    if(srv->type == EPOLL_DEFAUL_TYPE){
        if(socket_bind(srv->listen_fd,
            srv->port,srv->nevents) <0)
            return -1;
    }

    /* create buffer recycle thread*/
    if(pthread_create(&srv->recycler,NULL,
        epoll_recycle,(void*)srv) != 0)
        return -1;

    /* create prober thread*/
    if(pthread_create(&srv->prober,NULL,
        epoll_prober,(void*)srv) != 0)
        return -1;

    /* create epoll thread*/
    if(pthread_create(&srv->epoller,NULL,
        epoll_loop,(void*)srv) != 0)
        return -1;

    /* create write poll thread*/
    if(pthread_create(&srv->wpoller,NULL,
        epoll_wpoll,(void*)srv) != 0)
        return -1;

    /* create epoll read thread
    if(pthread_create(&srv->acceptor,NULL,
        epoll_accept,(void*)srv) != 0)
        return -1;*/

    /* create epoll read thread*/
    for(i=0;i<srv->reader_n;i++)
        if(pthread_create(&srv->reader,NULL,
            epoll_read_thr,(void*)srv) != 0)
            return -1;

    /* create epoll writer thread*/
    for(i=0;i<srv->writer_n;i++)
        if(pthread_create(&srv->writer,NULL,
            epoll_write_thr,(void*)srv) != 0)
            return -1; 

    /* create epoll upcall thread */
    if(srv->flags & EPOLL_UPCALL_TPOOL){
        for(i=0;i<srv->scheduler_n;i++)
            if(pthread_create(&srv->scheduler,NULL,
                epoll_schedule,(void*)srv) != 0)
                return -1;
    }

    return 0;
}

int eserver_stop(struct epoll_server* srv)
{
    /* set server stop flag*/
    srv->running = 0;

    /* close listen/epoll */
    epoll_close(srv);

    /* stop read task*/
    queue_term(srv->rtask);

    /* stop write task*/
    queue_term(srv->wtask);

    /* stop input queue*/
    queue_term(srv->rqueue);

    /* stop write condition*/
    if(pthread_cond_broadcast(&srv->wcond) != 0)
        return -1;

    if(pthread_cond_broadcast(&srv->wfull) != 0)
        return -1;

    /* waiting accept exist
    if(pthread_join(srv->acceptor, NULL) !=0)
        return -1;*/

    /* waiting loop exist*/
    if(pthread_join(srv->epoller, NULL) !=0)
        return -1;

    /* waiting reader exist*/
    if(srv->reader_n){
        if(pthread_join(srv->reader, NULL) !=0)
            return -1;
    }

    /* waiting writer exist*/
    if(srv->writer_n){
        if(pthread_join(srv->writer, NULL) !=0)
            return -1;
    }

    if(srv->flags & EPOLL_UPCALL_TPOOL){
        if(pthread_join(srv->scheduler, NULL) !=0)
            return -1;
    }

    /*stop buffer recycle thread*/
    UP(srv->free_sem);
    if(pthread_join(srv->recycler, NULL) !=0)
        return -1;

    UP(srv->probe_sem);
    if(pthread_join(srv->prober, NULL) !=0)
        return -1;

    return 0;
}

void eserver_destroy(struct epoll_server* srv)
{
    /* release task queue*/
    queue_destroy(srv->rtask);
    queue_destroy(srv->wtask);
    queue_destroy(srv->rqueue);

    /* release clients map*/
    epoll_free_fdmap(srv);
    epoll_free_idmap(srv);

    /* release hash map*/
    hash_destroy(srv->ipmap);
    hash_destroy(srv->fdmap);
    hash_destroy(srv->idmap);

    /* release write condition*/
    pthread_cond_destroy(&srv->wcond);

    /* release write condition*/
    pthread_cond_destroy(&srv->wfull);

    /* release server mutex */
    pthread_mutex_destroy(&srv->lock);

    /* release write lock */
    pthread_mutex_destroy(&srv->wlock);

    /* free threadpool threads*/
    destroy_threadpool(srv->threadpool);

    /* release buffer iplink*/
    free(srv->iplink);

    /* release black list*/
    list_for_each_free(srv->black_list);

    /* release buffer recycler*/
    fifo_destroy(srv->free_queue);
    SEM_DESTROY(srv->free_sem);

    /* release probe queue*/
    fifo_destroy(srv->probe_queue);
    SEM_DESTROY(srv->probe_sem);

    /* release server instance*/
    free(srv);
}

inline
int epoll_verify(struct epoll_server* srv,struct Buffer* buf)
{
    int isidentity = -1;

    /*identify client packet?*/
    if(buf->type == EPOLL_ID_BUF){
        isidentity = 0; /*free the buf*/
        DBG("EPoll: verify fd %d connection.\n",buf->src);
    }

    return isidentity;
}

int epoll_upcall(struct epoll_server* srv,Buffer* buf)
{
    /* if identification packet,not go on*/
    if(epoll_verify(srv,buf) == 0){
        buffer_free(buf);
        return 0;
    }

    TRACE("EPoll: recv %x{seq%d,flag %d,win %d, size %d}\n", buf,
        buf->pkt->seq,buf->pkt->flag,buf->pkt->win,buf->pkt->size);

    /* if inner epoll packet,inner handle*/
    if(epoll_packet(buf) == 0){
        buffer_free(buf);
    }
    else{
        buffer.pop(buf, HEADER_SIZE(PacketDesc));
        buffer.pop(buf, HEADER_SIZE(Packet));

        /* call upcall handle function*/
        __network_onreceived(srv->net, &buf->desc->from, buf);
    }
    return 0;
}

int epoll_input(struct epoll_server* srv,Buffer* buf)
{
    int ret;

    /* if use upcall thread poll,enqueue*/
    if(srv->flags & EPOLL_UPCALL_TPOOL){
        do{ 
            ret = queue_pushwait(srv->rqueue,buf,5);
            if(ret == ETIMEDOUT)
                WARN("EPoll: push recv buffer upcall timeout,(sendq %d).\n",srv->wbuf_cnt);
        }while(ret == ETIMEDOUT);
    }
    else{
        /* directly upcall packet*/
        ret = epoll_upcall(srv,buf);
    }

    return ret;
}

void* epoll_schedule(void* param)
{
    struct Buffer* buf;
    struct epoll_server* srv;
    srv = (struct epoll_server*)param;
    epoll_tlset(UPCALL_THREAD);

    /* dequeue and upcall buffer */
    while (srv->running){
        /* pop received buffer */
        buf = NULL;
        queue_pop(srv->rqueue,(void**)&buf);
        if(buf != NULL){
            epoll_upcall(srv,buf);
        }
    }
    return 0;
}

int epoll_wdiscard(struct epoll_server* srv)
{
    int count = 0;
    int priority;
    struct list_head *p,*n;
    struct list_head *b,*m;
    struct Buffer *buf;

    struct epoll_ipbuf* ipbuf;
    DBG("EPoll: discard buffers...\n");
    list_for_each_safe(p,n,srv->iplink){
        ipbuf = (struct epoll_ipbuf*)p;
        /* discard fe buffer avoid congestion*/
        for(priority=PRIORITY_HIGHEST-1;priority>=0;priority--){
            list_for_each_safe(b,m,ipbuf->list[priority]){
                buf = (struct Buffer *)b;
                /*only discard buffer from fe
                if(buf->hdr->type == M_BlockPut ||
                    buf->hdr->type == M_BlockGet )*/
                {
                    list_del(b);
                    /*epoll_wfree(srv,buf,EPOLL_WRITE_FAIT);*/
                    buffer_free(buf);
                    count++;
                }
            }
        }
        ipbuf->count = 0;
        list_del(p);
    }

    if(count){ /* signal enqueue*/
        DBG("EPoll: discard %d buffers.\n",count);
        srv->wbuf_cnt -= count;
        if (srv->full_waiters)
            pthread_cond_signal(&srv->wcond);
    }
    srv->ipcur = (struct epoll_ipbuf*)srv->iplink;
    
    return count;
}

int epoll_wbuf_push(struct epoll_server* srv,struct Buffer* buf,bool cycle)
{	
    struct epoll_ipbuf* ipbuf = NULL;

    ipbuf = hash_get(srv->idmap,buf->dst);
    if(ipbuf == NULL){
        int size,priority;
        ipbuf = malloc(sizeof(struct epoll_ipbuf));
        if(ipbuf == NULL) epoll_breakpoint();
        memset(ipbuf,0,sizeof(struct epoll_ipbuf));

        /*alloc priority queue head*/
        size = sizeof(struct list_head*)*MAX_PRIORITY_LEVEL;
        ipbuf->list = (struct list_head**)malloc(size);
        for(priority=0;priority < MAX_PRIORITY_LEVEL;priority++){
            ipbuf->list[priority] = LIST_HEAD_NEW();
            INIT_LIST_HEAD(ipbuf->list[priority]);
        }
        ipbuf->ip = buf->dst;
        hash_set(srv->idmap,buf->dst,ipbuf);
    }

    if(ipbuf->count == 0)
        list_add_tail((struct list_head*)ipbuf,srv->iplink);

    if(cycle == true)
        list_add((struct list_head*)buf,ipbuf->list[buf->priority]);
    else
        list_add_tail((struct list_head*)buf,ipbuf->list[buf->priority]);

    ipbuf->count++;
    return 0;
}

Buffer* epoll_wbuf_pop(struct epoll_server* srv)
{
    int priority;
    struct Buffer* buf = NULL;
    struct epoll_ipbuf *ipbuf = NULL;

    while(!list_empty(srv->iplink)){
        if(((struct list_head*)srv->ipcur) == srv->iplink)
            ipbuf = srv->ipcur->next;
        else
            ipbuf = srv->ipcur;		
        if (ipbuf->count == 0){
            srv->ipcur = ipbuf->next;
            list_del((struct list_head*)ipbuf);
        }
        else{
            for(priority=MAX_PRIORITY_LEVEL-1;priority>=0;priority--){
                if (!list_empty(ipbuf->list[priority])){
                    buf = (struct Buffer*)(ipbuf->list[priority]->next);
                    list_del((struct list_head*)buf);
                    srv->ipcur = ipbuf->next;
                    ipbuf->count--;
                    if(ipbuf->count == 0)
                    list_del((struct list_head*)ipbuf);
                    return buf;
                }
            }
        }
    }
    return buf;
}

int epoll_wpush(Buffer* buf)
{
    int result = -1;
    struct timespec timeout;
    struct epoll_server* srv;
    srv = (struct epoll_server*)buf->owner;

    /* check buffer length*/
    if (!buf->dlen) {
        DBG("EPoll: the buffer length %d error.\n",buf->dlen);
        return result;
    }

    if(buf->priority < 0 || buf->priority > MAX_PRIORITY_LEVEL){
        buf->priority = PRIORITY_NORMAL;
        DBG("EPoll: out of priority,error.\n");
    }

    /* check if cycle call*/
    if(epoll_tlsid() != UPCALL_THREAD){
        epoll_congest(srv,buf->dst);
    }

    /* if write limit,wait free*/
    buf->refcnt.counter = 0;
    if (pthread_mutex_lock(&srv->wlock) == 0) 
    {
        while (srv->wbuf_cnt >= srv->wbuf_limit){
            timeout.tv_sec = 3;  /* 3 second timeout*/
            timeout.tv_nsec = 0; /*sec=1000000000nsec*/
            srv->full_waiters++;
            result = pthread_cond_timedwait(&srv->wcond,&srv->wlock,&timeout);
            //result = pthread_cond_wait(&srv->wcond,&srv->wlock);
            srv->full_waiters--;

            /* server need exit */
            if (srv->running == 0){
                goto exit;  
            }
            /*wait timed expire */
            if(result == ETIMEDOUT){
                /*avoid congestion,discard buffer*/
                if(srv->flags & EPOLL_CONGEST_AVOID)
                    epoll_wdiscard(srv);
                result = -1;
                goto exit; /* cond waiting expire*/
            }
            if(result != 0){ 
                goto exit;  /* condition error*/
            }
        }

        /* enqueue a buffer */
        epoll_wbuf_push(srv,buf,false);
        srv->wbuf_cnt++;
        if (srv->empty_waiters) {
            pthread_cond_signal(&srv->wfull);
        }
        /* enqueue success*/
        result = 0;

exit:   /* exit buffer enqueue*/
        pthread_mutex_unlock(&srv->wlock);
    }

    /* if error,free buffer*/
    if (result == -1) {
        /*buffer.free(buf);*/
        DBG("EPoll: push send buffer failure\n");
    }

    return result;
}

Buffer* epoll_wpop(struct epoll_server* srv)
{
    int status = 0;
    Buffer* buf = NULL;

    // use the server lock in write poll
    if (pthread_mutex_lock(&srv->wlock) == 0)
    {
        while (srv->wbuf_cnt <= 0) {
            srv->empty_waiters++;
            status = pthread_cond_wait(&srv->wfull, &srv->wlock);
            srv->empty_waiters--;
            if (srv->running == 0)
                goto exit; /* server stop*/
            if(status != 0){
                goto exit; /* wait error*/
            }
        }

        /* dequeue a buffer */
        buf = epoll_wbuf_pop(srv);

        /* notice: 
        * here only pick a buffer item,still hold the count
        * for recycle the buffer if not be handle and back.
        */
exit:
        /* exit buffer enqueue*/
        pthread_mutex_unlock(&srv->wlock);
    }

    return buf;
}

int epoll_wsignal(struct epoll_server* srv)
{
    int ret = -1;
    if(pthread_mutex_lock(&srv->wlock) == 0){
        srv->wbuf_cnt--;
        if (srv->full_waiters)
            ret = pthread_cond_signal(&srv->wcond);
        pthread_mutex_unlock(&srv->wlock);
    }
    return ret;
}

int epoll_wcycle(struct epoll_server* srv,Buffer* buf)
{
    int result = 0;    
    pthread_mutex_lock(&srv->wlock);
    buf->refcnt.counter++;
    epoll_wbuf_push(srv,buf,true);
    pthread_mutex_unlock(&srv->wlock);
    return result;
}

int epoll_wtimeout(struct epoll_server* srv,struct epoll_client *client)
{
    int result = -1;
    long wtime;
    struct timeval now;

    /*check client if write timeout*/
    gettimeofday(&now,NULL);
    wtime = now.tv_sec - client->wttl;
    tv_sub(&now,&client->stamp);

    /*if over 2 second send ping*/
    if(wtime >= 3){
        if((srv->wtick%100) == 0)
            epoll_ping(srv,client->ip);
    }

    /*check client last timestamp*/
    if(wtime >= WPOLL_TIMEOUT){
        client->state = EPOLL_CLOSED;
        result = WPOLL_TIMEOUT;
        /* ping still work,peer halflife*/
        if(now.tv_sec < WPOLL_TIMEOUT-WPOLL_TIMEDELTA){
            client->flag |= EPOLL_HALFLIFE;
        }
    }

    return result;
}

void* epoll_recycle(void *args)
{
    struct Buffer* buf;
    struct epoll_server* srv;
    srv = (struct epoll_server*)args;

    bool state = false;
    bool keep = false;

    while(srv->running)
    {
        DOWN(srv->free_sem);
        buf = (struct Buffer*)fifo_pop(srv->free_queue);
        if (!buf) continue;

        state = false; 
        keep = false;
        if(buf->flag == EPOLL_WRITE_OK)
            state = true;

        /* build from address*/
        Host node;
        node.sin_port = htons(srv->port);
        node.sin_addr.s_addr = htonl(buf->dst);
        node.sin_family = AF_INET;
        buf->owner = srv;

        /* callback to up layer */        
        if(srv->ops->callback && (buf->type == EPOLL_IP_BUF)){
            //DBG("EPoll: buffer callback #%d %d\n",srv->wbuf_cnt,buf->dst);
            buffer.pop(buf, HEADER_SIZE(PacketDesc));
            buffer.pop(buf, HEADER_SIZE(Packet));
            srv->ops->callback(srv->net,
                &node,buf->hdr->seq,state,buf,&keep);
            if (keep == false)
                buffer_free(buf);
        }else{
            buffer_free(buf);
        }
    }

    return 0;
}

void epoll_wfree(struct epoll_server* srv,struct Buffer* buf,int wstate)
{
    buf->owner = srv;
    buf->flag = EPOLL_WRITE_FAIT;
    if(wstate == EPOLL_WRITE_OK)
    buf->flag = EPOLL_WRITE_OK;
        
    fifo_push(srv->free_queue, buf); 
    UP(srv->free_sem);
}

int epoll_retry(struct epoll_server* srv,struct epoll_client *client)
{
    int retry = -1;

    /* client write timeout,but peer still alive*/
    if(client->flag & EPOLL_HALFLIFE){
        return (retry = 1); /*not need retry*/
    }

    /* client write timeout*/
    if(client->flag & EPOLL_TIMEOUT){
        return (retry = -1); /*not need retry*/
    }

    /* client read broken error*/
    if(client->flag & EPOLL_RBROKEN){
        return (retry = 1); /*need retry*/
    }

    /* client write broken error*/
    if(client->flag & EPOLL_WBROKEN){
        return (retry = 1); /*need retry*/
    }

    return retry;
}

int epoll_clarify(struct epoll_server* srv,uint32 ip)
{
    struct epoll_client *p,*n;
    if(srv->bklist_cnt){
        list_for_each_entry_safe(p,n,srv->black_list,link){
            if(p->ip == ip){
                srv->bklist_cnt--;
                list_del((struct list_head*)p);
                epoll_unref(srv,p);
                return 0; /*alive*/
            }
        }
    }
    return -1;
}

int epoll_deadlist(struct epoll_server* srv,uint32 ip)
{
    int retry,interval;
    struct timeval now;
    struct epoll_client *p,*n;
    struct epoll_client *client;

   /* 
    * check ip in backlist,avoid node up/down
    * time range of retry and dead wait:
    *        1 relive    
    *  ---|-----------|------------------|------
    *     ^     2              3            0
              retrying     bear time      real dead    
    *   deadtime
    */   
    if(srv->bklist_cnt){
        list_for_each_entry_safe(p,n,srv->black_list,link){
            if(p->ip == ip){
                gettimeofday(&now,NULL);
                interval = now.tv_sec - p->deadline;
                if(interval >= 1)
                    p->deadline = now.tv_sec;
                tv_sub(&now,&p->stamp);
                /* reconnect closed node*/
                if(now.tv_sec <= WPOLL_TIMEOUT){
                    retry = epoll_retry(srv,p);
                    if(retry){ /*need retry,1000000*/
                        if(interval >= 1){
                            DBG("EPoll: reconnect %d ...\n",ip);
                            client = epoll_connect(srv,ip,srv->port);
                            if(client){ /*reconnect ok*/
                                DBG("EPoll: reconnect %d ok\n",ip);
                                srv->bklist_cnt--;
                                list_del((struct list_head*)p);
                                epoll_unref(srv,p);
                                return 1; /*relive*/
                            }else{
                                /*only reconnect once*/
                                if(p->flag & EPOLL_HALFLIFE){
                                    p->flag &=~EPOLL_HALFLIFE;
                                    p->flag &=~EPOLL_TIMEOUT;
                                    epoll_unref(srv,p);
                                    return 0; /*dead,clean*/
                                }
                            }
                        }
                        /* in range of retry time*/
                        usleep(WPOLL_TICK);
                        return 2;
                    }
                }

                /* already out of dead time*/
                if(now.tv_sec > BLACK_TIMEOUT){
                    DBG("EPoll: backlist client free %d\n",ip);
                    if(p->refcnt == 0){
                        srv->bklist_cnt--;
                        list_del((struct list_head*)p);
                        epoll_unref(srv,p);
                        return 0; /*real dead,clean*/
                    }
                }
                /* in range of dead bear time*/
                return 3;
            }
        }
    }

    /* not found the ip*/
    return 0;
}

int epoll_wrequest(struct epoll_server* srv,struct epoll_client *client,struct Buffer* buf)
{
    int ret = -1;

    if (client->wflag == WEVNET_UNSET)
    {
        client->wflag = WEVENT_REQUESTED;
        /* set write start stamp*/
        struct timeval now;
        gettimeofday(&now,NULL);
        tv_setv(&client->stamp,&now);

        /* record write start time*/
        if(client->wttl == 0) 
        client->wttl = now.tv_sec;
        client->wttl = time(NULL);
        client->wbuf = buf;

        /* set client's fd EPOLLOUT*/
        ret = epoll_rmod(srv,client);
        if(ret != 0){
            /*write request failure*/
            client->wflag = WEVNET_UNSET;
            epoll_wcycle(srv,buf);
            return ret;
        }
    }
    else /*writing buffer,check if write timeout*/
    {
        if(epoll_wtimeout(srv,client) > 0){
            /*if timeout,recycle buffer
            client->flag |= EPOLL_TIMEOUT;
            client->state = EPOLL_CLOSED;*/
            epoll_wcycle(srv,buf);
            DBG("EPoll: write poll timeout,#%d\n",srv->wbuf_cnt);
        }
        else{
            /*if no timeout,recycle*/
            epoll_wcycle(srv,buf);
        }        
    }

    return -1;
}

void* epoll_wpoll(void* arg)
{
    int again;
    int retrying;
    unsigned int ip;
    struct Buffer* buf;
    struct epoll_client *client;
    struct epoll_server* srv = (struct epoll_server*)arg;   

    while (srv->running)
    {
        /* get buffer from srv queue*/
        buf = epoll_wpop(srv);
        if (buf == NULL) {
            usleep(WPOLL_TICK);
            continue;
        }
        
        /* get buffer's destination*/
        again = 0;
        ip = buf->dst;
        epoll_lock(srv);
        client = NULL;

        /* get destination fd client*/
        if(buf->type == EPOLL_FD_BUF){
            client = (struct epoll_client*)hash_get(srv->fdmap,buf->dst);
            if(client == NULL){
                DBG("EPoll: wpoll not found fd %d client,error\n",buf->dst);
            }else{
                if(client->flag & EPOLL_MAPIP){
                    DBG("EPoll: fd %d client already gone,\n",buf->dst);
                    goto write_fail;
                }
            }
        }
        else{
            /* get destination ip client*/
            client = (struct epoll_client*)hash_get(srv->ipmap,ip);
            if (client == NULL) {
                /* check dst ip if in dead list*/
                if((retrying = epoll_deadlist(srv,ip))){
                    if(retrying <= 2){ /*retrying dst ip*/
                        epoll_wcycle(srv,buf);
                        goto write_exit;
                    }else{
                        DBG("EPoll: client %d real dead,error\n",ip);
                        goto write_fail;
                    }
                }else{ /*not found dead client*/
                    /* create a new connection*/
                    client = epoll_connect(srv,ip,srv->port);
                    if (client == NULL){
                        DBG("EPoll: client %d unavaiable,error\n",ip);
                    }
                }
            }
        }

        /* send buffer to destination*/        
        if(client != NULL){
            if (client->state != EPOLL_CLOSED ){
                epoll_ref(client);
                again = buf->refcnt.counter;
                epoll_writebuf(srv,client,buf);
                TRACE("EPoll: write fd %d buf end\n",client->fd);
                epoll_unref(srv,client);
                goto write_exit;
            }else{
                DBG("EPoll: wpoll client closed,error\n");
            }
        }

        /* write buffer result*/
write_fail:
        epoll_wfree(srv,buf,EPOLL_WRITE_FAIT);
        epoll_wsignal(srv);
write_exit:
        epoll_unlock(srv);

        /* avoid write again eat cpu*/
        if(again>3) usleep(WPOLL_TICK);
    }

    return 0;
}

int epoll_output(Buffer* buf)
{
    return epoll_wpush(buf);
}

int epoll_send(struct epoll_server* srv,Buffer* buf)
{
    return epoll_wpush(buf);
}

int epoll_congest(struct epoll_server* srv,uint32 ip)
{
    int delay;
    unsigned int window;
    struct timeval now,stamp;
    struct epoll_client *client;
  

    /* local sent queue check*/
    if(srv->wbuf_cnt > CONGEST_THRESHOLD){
       srv->delay <<= 2;
       if(srv->delay > CONGEST_DELAY)
            srv->delay = CONGEST_DELAY; /* 0.46sec/packet*/
       usleep(srv->wbuf_cnt*srv->delay);
       window = srv->wbuf_cnt;
    }
    else{
        /* 1sec=1000000*/
        srv->delay = 1; /*reset delay*/
        /* get the target ip connection*/
        client = NULL;        
        epoll_lock(srv);    
        client = (struct epoll_client*)hash_get(srv->ipmap,ip);
        if (client != NULL){
            delay = client->delay;
            window = client->win;
            stamp = client->stamp;
        }
        epoll_unlock(srv);
        if(client){
            /*if window is updated in 1sec*/
            gettimeofday(&now,NULL);
            tv_sub(&stamp,&now);
            if(stamp.tv_sec <= 3){
                if (window > CONGEST_THRESHOLD) {
                    delay <<= 2;
                    if(delay > CONGEST_DELAY) 
                        delay = CONGEST_DELAY;
                }
                usleep(window*delay);
            }else{
                usleep(CONGEST_KEEPALIVE);
            }
        }
    }

    return 0;
}

struct Buffer* epoll_buff()
{
    int size;
    struct Buffer* buf;

    buf = buffer.alloc(BIG_ENOUGH);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->type = 0;

    srand((unsigned)time(NULL)); 
    unsigned short int wlen = rand();
    if(wlen < 1000 || wlen > 62*1024)
        wlen = 10000;

    buf->dlen = wlen;
    buf->pkt->seq = 0xBEFE;
    buf->pkt->flag = ACK_TEST;
    buf->pkt->size = wlen-HEADER_SIZE(Packet);
    buf->desc->size = wlen;
    return buf;
}

int epoll_urgent(struct epoll_server* srv,uint32 ip)
{
    int ret,size;
    struct Buffer* buf;

    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    buf->dlen = HEADER_SIZE(Packet);
    buf->pkt->flag |= URG_STOP;
    buf->pkt->flag |= BO_CAST;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    addr.sin_port = htons(srv->port-1);

    ret = sendto(srv->net->sock,buf->data, buf->dlen,
        0,(struct sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        DBG("EPoll: sendto error %d.\n",errno);
    }

    return ret;
}

int epoll_ping(struct epoll_server* srv,uint32 ip)
{
    int ret,size;
    struct Buffer* buf;
    char ipstr[20];

    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    buf->dlen = HEADER_SIZE(Packet);
    buf->pkt->flag |= ACK_TEST;
    buf->pkt->flag |= ACK_PLEASE;
    buf->pkt->win = srv->wbuf_cnt;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(srv->port-1);

    DBG("EPoll: ping %s...\n",
            ip_htoa(ip, ipstr, sizeof(ipstr)));
    ret = sendto(srv->net->sock,buf->data, buf->dlen,
        0,(struct sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        DBG("EPoll: ping - sendto %s return error %d.\n",
                ip_htoa(ip, ipstr, sizeof(ipstr)),
                errno);
    }
    return ret;
}

int epoll_pong(struct epoll_server* srv,uint32 ip)
{
    int ret,size;
    struct Buffer* buf;
    char ipstr[20];

    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    buf->dlen = HEADER_SIZE(Packet);
    buf->pkt->flag |= ACK_TEST;
    buf->pkt->flag |= ACK_YES;
    buf->pkt->win = srv->wbuf_cnt;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(srv->port-1);
    ret = sendto(srv->net->sock,buf->data, buf->dlen,
        0,(struct sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        DBG("EPoll: sendto %s error %d.\n",
                ip_htoa(ip, ipstr, sizeof(ipstr)),
                errno);
    }
    return ret;
}

int epoll_alive(struct epoll_server* srv,uint32 ip,uint16 wnd)
{
    int result = -1;
    struct epoll_client *client;
    char ipstr[20];
    
    client = NULL;
    epoll_lock(srv);    
    client = (struct epoll_client*)hash_get(srv->ipmap,ip);
    if (client != NULL) {
        gettimeofday(&client->stamp,NULL);
        client->win = wnd;
        result = 0; /*update timestamp*/
    }
    epoll_unlock(srv);
    DBG("EPoll: pong %s ...\n",
            ip_htoa(ip, ipstr, sizeof(ipstr)));
    return result;
}

int epoll_probe(struct epoll_server* srv,uint32 ip)
{
    int size;
    struct Buffer* buf;
    struct epoll_packet* pkt;

    /* build probe packet buffer*/
    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->type = EPOLL_PROBE_BUF;    
    buf->priority = PRIORITY_HIGH;
    buf->flag = 0;
    buf->dst = ip;

    /* set server inner flag*/
    pkt = (struct epoll_packet*)buf->data;
    pkt->type = EPOLL_PROBE;

    buf->pkt->seq = 0xBEFE; /*magic*/
    buf->pkt->flag |= ACK_TEST;
    buf->pkt->flag |= SRV_INNER;
    buf->pkt->win = srv->wbuf_cnt;

    /* epoll packet size*/
    buf->dlen = sizeof(struct epoll_packet);
    buf->pkt->size = buf->dlen - HEADER_SIZE(Packet);
    buf->desc->size = buf->dlen;
    buf->owner = srv;
    
    /* enqueue the the probe buf*/
    fifo_push(srv->probe_queue, buf); 
    UP(srv->probe_sem);
    return 0;
}

void* epoll_prober(void *args)
{
    int ret;
    struct Buffer* buf;
    struct epoll_server* srv;
    srv = (struct epoll_server*)args;

    while(srv->running){
        DOWN(srv->probe_sem);
        buf = (struct Buffer*)fifo_pop(srv->probe_queue);
        if (buf){ 
            do {
                ret = epoll_wpush(buf);
                if(ret == -1) usleep(1000);
            } while (ret == -1);
        }
    }
    return 0;
}

int epoll_recovery(struct epoll_server* srv,uint32 ip)
{
    int ret,size;
    struct Buffer* buf;

    buf = buffer.alloc(_1K);
    size = buffer_capacity(buf);
    buf->desc = MAKE_DESC(buf,size);
    buf->pkt = MAKE_PACKET(buf);
    buf->hdr = MAKE_HEAD(buf);
    buf->data = (char*)buf->pkt;
    buf->flag = 0;

    buf->dlen = HEADER_SIZE(Packet);
    buf->pkt->flag |= URG_RAVEL;
    buf->pkt->flag |= BO_CAST;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    addr.sin_port = htons(srv->port-1);

    ret = sendto(srv->net->sock,buf->data, buf->dlen,
        0,(struct sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        DBG("EPoll: sendto error %d.\n",ret);
    }
    return ret;
}

int epoll_packet(struct Buffer* buf)
{
    int ret;
    struct Buffer* rbuf;
    struct epoll_packet* pkt_src;
    struct epoll_packet* pkt_dst;
    struct epoll_server* srv;
    srv = buf->owner;

    if(buf->pkt->flag & ACK_TEST){
        pkt_src = (struct epoll_packet*)buf->data;
        if(pkt_src->type == 0){ /*source request*/
            rbuf = epoll_buff();
            if(rbuf){ /*build forward*/
                pkt_dst = (struct epoll_packet*)rbuf->data;
                pkt_dst->type = 1; /*request*/
                pkt_dst->seq = pkt_src->seq;
                pkt_dst->sn = pkt_src->sn;
                pkt_dst->dst = pkt_src->dst;
                /*avoid congestion,discard buffer*/
                if(pkt_src->op == EPOLL_CONGEST_AVOID)
                    srv->flags |= EPOLL_CONGEST_AVOID;
                
                /*non forward,c->s*/
                if(pkt_dst->dst == buf->dst){
                    rbuf->type = EPOLL_FD_BUF;
                    rbuf->dst = buf->dst;     //fd
                }else{
                    pkt_dst->src = buf->dst;
                    pkt_dst->fd = buf->dst;   //fd
                    rbuf->dst = pkt_src->dst;
                }
                
                rbuf->owner = buf->owner;
                ret = epoll_wpush(rbuf);
                if(ret == -1){
                    buffer_free(rbuf);
                }
            }
        }
        else if(pkt_src->type == 1){ /*receive request*/
            rbuf = epoll_buff();
            if(rbuf){ /*build reply*/
                pkt_dst = (struct epoll_packet*)rbuf->data;
                pkt_dst->type = 2; /*reply*/
                pkt_dst->seq = pkt_src->seq;
                pkt_dst->sn = pkt_src->sn;
                pkt_dst->src = pkt_src->src;
                pkt_dst->dst = buf->dst;
                pkt_dst->fd = pkt_src->fd;

                rbuf->owner = buf->owner;
                rbuf->dst = buf->dst;
                ret = epoll_wpush(rbuf);
                if(ret == -1){
                    buffer_free(rbuf);
                }
            }
        }
        else if(pkt_src->type == 2){ /*receive reply*/
            rbuf = epoll_buff();
            if(rbuf){ /*build result*/
                pkt_dst = (struct epoll_packet*)rbuf->data;
                pkt_dst->type = 3; /*result*/
                pkt_dst->seq = pkt_src->seq;
                pkt_dst->sn = pkt_src->sn;
                pkt_dst->src = pkt_src->src;
                pkt_dst->dst = pkt_src->src;
                
                rbuf->type = EPOLL_FD_BUF;
                rbuf->owner = buf->owner;
                rbuf->dst = pkt_src->fd;
                ret = epoll_wpush(rbuf);
                if(ret == -1){
                    buffer_free(rbuf);
                }
            }
        }
        return 0;
    }
    return -1;
}

struct epoll_ops _eserver_ops =
{
    .add = epoll_add,
    .del = epoll_del,
    .dispatch = epoll_dispatch,
    .loop = epoll_loop,
    .read = epoll_read,
    .create = eserver_create,
    .init = eserver_init,
    .start = eserver_start,
    .stop = eserver_stop,
    .destroy = eserver_destroy,
    .recv = epoll_recv,
    .send = epoll_send,
    .input = epoll_input,
    .output = epoll_wpush,
    .fail = __network_onfail,
    .callback = __network_onsend,
};
