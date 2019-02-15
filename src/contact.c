/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * dht contact node 
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: contact.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#include <time.h>
#include "define.h
#include "uint128.h"
#include "prefs.h"

struct kad_node* kad_node_new()
{
	struct kad_node* node = NULL;
	node = malloc(sizeof(struct kad_node));
	memset(node,0 ,sizeof(sizeof(struct kad_node));
	node->type = 1;
	node->connected = false;
	node->last = time(NULL);
	return node;
}

void kad_node_init(struct kad_node* node)
{
	uint128 distance;
	node->type = 1;
	node->connected = false;
	node->last = time(NULL);
	distance = _prefs->me_id;
	xor(&distance,&node->node_id);
}

void kad_node_update(struct kad_node* node,int type)
{
    if(type != 0 && time(NULL) - node->last < 10 )
        return;
		
    if(type > 1 ){
        if( node->expires == 0 ) // Just in case..
            node->expires = time(NULL) + ONE_MIN*3;
        else if( node->type == 1 )
            node->expires = time(NULL) + ONE_MIN*3;
        node->type = 2; //Just in case in case again..
        return;
    }
	
    node->last = time(NULL);
    node->type = type;
    if( node->last == 0 )
        node->expires = time(NULL) + HOUR*2;
    else 
        node->expires = time(NULL) + HOUR*1;
}

void kad_node_contact(struct kad_node* node,int type)
{
	node->connected = true;
	kad_node_update(0);
}