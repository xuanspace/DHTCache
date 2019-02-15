/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 *
 * Port Map
 *
 * Author(s): Linweixuan  <weixuan.lin@sierraatlantic.com>
 *
 * $Id: portmap.h,v 1.1 2009-03-23 08:56:49 wxlin Exp $
 */

#ifndef PORTMAP_H_
#define PORTMAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"

enum PORT_STATUS
{
    FREE_PORT,
    REUSE_PORT
};

typedef struct PortEntry
{
    struct list_head node;
    size_t  id;
    int	  from;
    int	  to;
    int	  port;
	int	  sock;
	int	  fd;
	int	  result;
    int	  flag;
	void* owner;

	int (*handle_fail)();
	int (*handle_timeout)();
	int (*handle_data)();

} PortEntry;

typedef struct Portmap
{
    struct list_head*  using_ports;
    struct list_head*  free_ports;
	unsigned int       base_port;
	unsigned int       port_range;
    unsigned int       using;
    unsigned int       free;
    unsigned int       max;
    pthread_mutex_t    lock;

} Portmap;

Portmap* alloc_portmap();
void free_portmap(Portmap* portmap);
void print_portmap(Portmap* portmap);
PortEntry* get_portmap(Portmap* portmap, int port);
int unmap_port(Portmap* portmap,int from);
int init_port(PortEntry* map);
int free_port(PortEntry* map);

int map_fport(int from,size_t id);
int map_port(Portmap* portmap,int from,size_t id);
int send_file(char* filename,int ip,int port,size_t id);
int recv_file(char* filename,int port,size_t size,size_t id);
extern struct Portmap* __portmap_svc;

#endif /* PORTMAP_H_ */
