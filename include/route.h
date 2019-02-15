/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * dht route
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: route.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#ifndef _ROUTE_H_
#define _ROUTE_H_

#include "types.h

struct route_option 
{
	const char *network;
	const char *netmask;
	const char *gateway;
	const char *metric;
};

struct route_table
{
	int size;
	bool gateway;
	bool local;
	bool default;
	struct route_option routes[MAX_ROUTES];
};

struct route
{
	bool defined;
	struct route_option *opt;
	in_addr_t network;
	in_addr_t netmask;
	in_addr_t gateway;
	bool metric_defined;
	int metric;
};

#endif /*_ROUTE_H_*/

