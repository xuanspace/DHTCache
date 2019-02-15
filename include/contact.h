/*
 * DHT Cache Server
 * Copyright (c), 2008, GuangFu, 
 * 
 * dht contact node 
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: contact.h 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#ifndef _DHTNODE_H_
#define _DHTNODE_H_

#include <time.h>
#include "uint128.h"

struct kad_node
{
	uint128		node_id;
	uint128		distance;
	uint32		ip;
	uint16		tcp_port;
	uint16		udp_port;
	byte		type;
	time_t		last;
	time_t		expires;
	bool		connected;
};

struct kad_node* kad_node_new();
int kad_node_init(struct kad_node* node);
int kad_node_type(struct kad_node* node,int type);

#endif /*_DHTNODE_H_*/
