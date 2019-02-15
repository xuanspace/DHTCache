/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Global defines
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: globalh 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "uint128.h"

struct preferences
{
	uint128		me_id;
};

struct preferences __prefs;

struct preferences* get_prefs();
void get_distance(dht_node* node);

void get_distance(dht_node* node)
{
	xor(&node->node_id,&__prefs->me_id);
}

#endif //_GLOBAL_H_
