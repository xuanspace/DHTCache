/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * implementation of memory pool
 *
 * Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
 *
 * $Id: mempool.h,v 1.2 2008-12-12 03:36:44 wxlin Exp $
 *
 */

#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Slab {
	struct list_head  	  node;
	struct list_head*  	  free;
	volatile unsigned int idle;
	void*				  mem;
	void*				  end;
};

struct Mempool {
	struct list_head* 	  slab;	    /*all slabs list*/
	struct list_head* 	  free;     /*free slab list,not used*/
	struct list_head* 	  full;	    /*full slab list,not used */
	volatile unsigned int size;	    /*object size in a slab */
	volatile unsigned int num;      /*object number in a slab*/
	volatile unsigned int idle;	    /*idle objects in mempool*/
	volatile unsigned int count;    /*total objects in mempool*/
	volatile unsigned int limit;	/*the max objects in pool */
	pthread_mutex_t       lock;
};

extern struct Mempool* mempool_init(unsigned int size,unsigned int num,unsigned int max);
extern void* mempool_alloc(struct Mempool* pool,unsigned int size);
extern void* mempool_grow(struct Mempool* pool,unsigned int size,unsigned int num);
extern void mempool_free(struct Mempool* pool,void* p);
extern void mempool_destroy(struct Mempool* pool);

#ifdef __cplusplus
}
#endif

#endif /*__MEM_POOL_H__*/
