/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 *
 * Port Map
 *
 * Author(s): Linweixuan  <weixuan.lin@sierraatlantic.com>
 *
 * $Id: portmap.c,v 1.1 2009-03-23 08:56:49 wxlin Exp $
 */

#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "portmap.h"
#include "log.h"

#define TCP_CORK 		3
#define SOCKET_ERROR 	-1
#define PORTMAP_MAX     10
#define BASE_PORT       7000
#define BASE_RANGE      10
#define BUF_SIZE        1024*64
#define MAX_LISTEN      1
#define MAX_RETRY       3

struct Portmap* __portmap_svc = NULL;

void portmap_exit()
{
	if(__portmap_svc){
		free_portmap(__portmap_svc);
	}
}

Portmap* alloc_portmap()
{
    int n = sysconf(_SC_ATEXIT_MAX);
    DBG("VFILE[*]: Sysconf max atexit = %d\n", n);
    n = atexit(portmap_exit);
    if (n != 0) {
       DBG("VFILE[*]: cannot set portmap exit function.\n");
       return NULL;
    }

    Portmap* portmap = malloc(sizeof(Portmap));
    if(portmap){
        memset(portmap,0,sizeof(Portmap));
        portmap->using_ports = malloc(sizeof(struct list_head));
        portmap->free_ports = malloc(sizeof(struct list_head));
        INIT_LIST_HEAD(portmap->using_ports);
        INIT_LIST_HEAD(portmap->free_ports);
        portmap->max = PORTMAP_MAX;
        portmap->base_port = BASE_PORT;
        portmap->port_range = portmap->base_port+BASE_RANGE;
        pthread_mutex_init(&portmap->lock, NULL);
    }
    return portmap;
}

void print_portmap(Portmap* portmap)
{
	PortEntry* map;
	pthread_mutex_lock(&portmap->lock);
	list_for_each_entry(map,portmap->using_ports,node){
		DBG("VFILE[%d]: port:%d,from %d\n",map->id,map->port,map->from);
	}
	DBG("VFILE[*]: ------------------\n");
	pthread_mutex_unlock(&portmap->lock);
}

void free_portmap(Portmap* portmap)
{
	struct PortEntry *p,*n;
	pthread_mutex_lock(&portmap->lock);
    list_for_each_entry_safe(p,n,portmap->using_ports,node){
	    list_del((struct list_head*)p);
        p->flag = FREE_PORT;
	    free_port(p);
	    free(p);
    }

    list_for_each_entry_safe(p,n,portmap->free_ports,node){
	    list_del((struct list_head*)p);
        p->flag = FREE_PORT;
	    free_port(p);
	    free(p);
    }
    pthread_mutex_unlock(&portmap->lock);
    pthread_mutex_destroy(&portmap->lock);
    free(portmap);
}

PortEntry* get_portmap(Portmap* portmap, int port)
{
	PortEntry* map = NULL;
	pthread_mutex_lock(&portmap->lock);
	list_for_each_entry(map,portmap->using_ports,node){
		if(map->port == port)
			break;
	}
	pthread_mutex_unlock(&portmap->lock);
	return map;
}

int map_port(Portmap* portmap,int from,size_t id)
{
	PortEntry* map = NULL;
	pthread_mutex_lock(&portmap->lock);
	if (portmap->free){
		map = list_first_entry(portmap->free_ports,PortEntry,node);
		map->id = id;
        map->fd = -1;
		//map->sock = -1;
		map->from = from;
		if(init_port(map) > 0){
			portmap->free--;
			portmap->using++;
			list_add_tail((struct list_head*)map,portmap->using_ports);
			portmap->base_port = map->port;
		}
	}else{
		if (portmap->free + portmap->using < portmap->max){
			map = malloc(sizeof(PortEntry));
			if(map){
				memset(map,0,sizeof(PortEntry));
				map->id = id;
                map->fd = -1;
				map->sock = -1;
				map->port = portmap->base_port++;
				map->from = from;
				if(init_port(map) > 0){
					portmap->using++;
					list_add_tail((struct list_head*)map,portmap->using_ports);
					portmap->base_port = map->port;
				}
			}
		}
	}
	pthread_mutex_unlock(&portmap->lock);

	return map ? map->port : -1;
}

int unmap_port(Portmap* portmap, int port)
{
	int result = -1;
	PortEntry* map;

	pthread_mutex_lock(&portmap->lock);
	list_for_each_entry(map,portmap->using_ports,node){
		if(map->port == port){
			list_move_tail((struct list_head*)map,portmap->free_ports);
			portmap->free++;
			portmap->using--;
			result = 0;
			free_port(map);
			break;
		}
	}
	pthread_mutex_unlock(&portmap->lock);

	return result;
}

int init_port(PortEntry* map)
{
	int sock;
    int count=0;
	struct sockaddr_in addr;

    /* if reuse port socket*/
    if(map->flag == REUSE_PORT && map->sock)
        return map->port;

    /* create map port socket*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		DBG("VFILE[%d]: cannot open socket.\n",map->id);
		return -1;
	}

	/* Set socket send buffer size	*/
	int optval = 1024*1024;
	socklen_t optlen = sizeof(int);
	int ret = setsockopt( sock, SOL_SOCKET, SO_SNDBUF,&optval, optlen);
	if(ret == SOCKET_ERROR){
		DBG("VFILE[%d]: Can't set the socket send buffer size!\n",map->id);
		goto erro;
	}

	/* Set socket receive buffer size */
	ret = setsockopt( sock, SOL_SOCKET, SO_RCVBUF,&optval, optlen);
	if(ret == SOCKET_ERROR){
		DBG("VFILE[%d]: Can't set the socket receive buffer size!\n",map->id);
		goto erro;
	}else{
		DBG("VFILE[%d]: Set socket buffer size %dk.\n",map->id,optval/1024);
	}

	optval = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,&optval, optlen);
	if(ret == SOCKET_ERROR){
		DBG("VFILE[%d]: Unable to set addr reuse!\n",map->id);
		goto erro;
    }

	/* Put a cork into the socket as we want to combine the write() calls */
	optval = 1;
	ret = setsockopt(sock, IPPROTO_TCP, TCP_CORK, &optval, optlen);
	if(ret == SOCKET_ERROR){
		DBG("VFILE[%d]: Can't set the socket cork!\n",map->id);
		goto erro;
	}

	/* bind map server port*/
	if(map->to){
        do{
            count++;
		    addr.sin_family = AF_INET;
		    addr.sin_addr.s_addr = htonl(map->to);
		    addr.sin_port = htons(map->port);
		    ret = connect(sock,(struct sockaddr*)&addr,sizeof(addr));
		    if(ret == SOCKET_ERROR){
			    DBG("VFILE[%d]: Can't connect peer sock.!\n",map->id);
                if(count > MAX_RETRY)
			        goto erro;
                sleep(1);
            }else{
                DBG("VFILE[%d]: Connect %s-%d ok.\n",map->id,
                    inet_ntoa(addr.sin_addr),map->port);
            }
        }while(ret == SOCKET_ERROR);
	}

	if(map->from){        
		do{
            count++;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(map->port++);

			ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
            if(ret == SOCKET_ERROR) {
                DBG("VFILE[%d]: Can't bind the port to socket!\n",map->id);
                continue;
            }
            ret = listen(sock, MAX_LISTEN);
            if(ret == SOCKET_ERROR) {
                DBG("VFILE[%d]: Can't listen the socket port!\n",map->id);
            }
		}while(ret == SOCKET_ERROR);
	}

	/* set sock port map entry*/
	map->port--;
	map->sock = sock;
	return map->port;

erro:
	if(sock)
	close(sock);
	map->port = -1;
	map->sock = -1;
	return -1;
}

int free_port(PortEntry* map)
{
	if (map->fd){
		close(map->fd);
		map->fd = -1;
	}

    if(map->flag != REUSE_PORT){
	    if (map->sock){
		    close(map->sock);
		    map->sock = -1;
	    }
    }
	return map->port;
}

int send_file(char* filename,int ip,int port,size_t id)
{
	struct stat fs;
	PortEntry map;

	memset(&map,0,sizeof(PortEntry));
	map.fd = open(filename, O_RDONLY);
    if (map.fd == -1){
        DBG("VFILE[%d]: Can't open file %s!\n",map.id,filename);
		return -1;
    }

    if(fstat(map.fd, &fs) == -1){
        DBG("VFILE[%d]: Can't fstat file %s!\n",map.id,filename);
		goto erro;
    }

    map.id = id;
    map.from = 0;
    map.to = ip;
	map.port = port;

    if(init_port(&map) == -1){
        DBG("VFILE[%d]: Failed to init port %d!\n",map.id,port);
		goto erro;
    }

	off_t offset = 0;
	int n = sendfile(map.sock, map.fd, &offset, fs.st_size);
    if ( n < 0){
        DBG("VFILE[%d]: Sendfile transfer error!\n",map.id);
		goto erro;
    }
    if(n != fs.st_size){
        DBG("VFILE[%d]: Sendfile transfer size error!\n",map.id);
        goto erro;
    }
    else{
        DBG("VFILE[%d]: Send %d bytes done.\n",map.id,n);
    }
	return n;

erro:
	free_port(&map);
	return -1;
}

int map_fport(int from,size_t id)
{
	/* check portmap instance*/
	if(__portmap_svc == NULL){
		__portmap_svc = alloc_portmap();
		if(!__portmap_svc) return -1;
	}

	/* map send file to port*/
    int port = map_port(__portmap_svc, from, id);
    if(port == -1)
        DBG("VFILE[%d]: map receive file port error!\n",id);
    return port;
}

int recv_file(char* filename,int port,size_t size,size_t id)
{
	PortEntry* map;
	unsigned int recv_count = 0;

	/* open a file for writing*/
	int fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC);
    if(fd == -1) {
        DBG("VFILE[%d]: Can't open file %s!\n",id,filename);
        return -1;
    }
    DBG("VFILE[%d]: Create file %s!\n",id,filename);

	/* get sendfile portmap entry*/
	map = get_portmap(__portmap_svc, port);
    if(map == NULL) {
        DBG("VFILE[%d]: Can't get map file port %d!\n",id,port);
        goto erro;
    }
	map->fd = fd;

	/* Accept send file connect*/
	int peer;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	peer = accept(map->sock,(struct sockaddr *)&addr,&addrlen);
    if (peer == -1){
        DBG("VFILE[%d]: Accept client connection error.\n",id);
        goto erro;
    }

    /* Prepare receive sock data*/
    struct sockaddr_in *inAddr = (struct sockaddr_in*)&addr;
    DBG("VFILE[%d]: Accept file %s.\n",id,inet_ntoa(inAddr->sin_addr));
    DBG("VFILE[%d]: Receiving...\n",id);

    struct timeval start,end;
    gettimeofday(&start, NULL);
    char buf[BUF_SIZE] = {0};

    /* Receive send file datas*/
    int n,m;
    recv_count = 0;
    while (1)
    {
        n = recv( peer, buf, sizeof( buf ), 0);
        if(n < 0){
            DBG("VFILE[%d]: Peer close socket or error!\n",id);
            break;
        }else{
        	recv_count += n;
        	m = write(fd, buf, n);
        	if(m<0 && m!=n)
        		goto erro;
            if(size == recv_count){
                DBG("VFILE[%d]: Receive %d bytes done.\n",id,size);
                map->flag = REUSE_PORT;
        		break;
            }
        }
    }
    
    gettimeofday(&end, NULL);
    unsigned int second = end.tv_sec - start.tv_sec;
    DBG("VFILE[%d]: Receive finished.\n",id);
    DBG("VFILE[%d]: Network speed %fk/%d sec\n",id,(float)(recv_count/1024),second);
    
erro:
    close(peer);
    close(fd); map->fd = -1;
	unmap_port(__portmap_svc, port);
    return (recv_count == size) ? size : -1;
}
