/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * preferences
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: prefs.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#include <time.h>
#include "define.h
#include "uint128.h"

struct prefs
{
	char filename;
	uint128 node_id;
	uint32 node_ip;
	uint32 tcp_port;
	uint32 version;
	uint32 udp_port;
};

extern struct prefs _prefs;