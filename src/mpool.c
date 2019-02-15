/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 *
 * implementation of memory pool
 *
  * Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
  *
 * $Id: mempool.c,v 1.3 2008-12-23 07:16:33 wxlin Exp $
 *
 */

#include "mempool.h"

struct Mempool* mempool_init(unsigned int size,unsigned int num,unsigned int max)
{
	int i;
	struct Slab* slab;
	struct Mempool* pool;
	struct list_head*  obj;

	pool = (struct Mempool*)malloc(sizeof(struct Mempool));
	if(pool){
		memset(pool,0,sizeof(struct Mempool));
		pool->size = size;
		pool->num = num;
		pool->limit = max;
		pool->count = num;
		pool->slab = malloc(sizeof(struct list_head));
		pool->free = malloc(sizeof(struct list_head));
		pool->full = malloc(sizeof(struct list_head));
		INIT_LIST_HEAD(pool->slab);
		INIT_LIST_HEAD(pool->free);
		INIT_LIST_HEAD(pool->full);
		pthread_mutex_init(&pool->lock, NULL);

		slab = (struct Slab*)malloc(sizeof(struct Slab));
		if(slab){
			memset(slab,0,sizeof(struct Slab));
			slab->mem = malloc(size*num);
			if(slab->mem){
				slab->end = slab->mem + size*num;
				for(i=0;i<size*num;i+=pool->size){
					obj = (struct list_head*)(slab->mem+i);
					list_add_tail(obj,pool->free);
				}
				slab->idle = num;
				pool->idle += num;
				list_add_tail(((struct list_head*)slab),pool->slab);
				return pool;
			}
		}
	}

	return NULL;
}

void* mempool_alloc(struct Mempool* pool,unsigned int size)
{
	struct list_head* obj = NULL;
    if(pthread_mutex_lock(&pool->lock) ==0){
	    if(!pool->idle && pool->count < pool->limit){
		    mempool_grow(pool,pool->size,pool->num);
	    }

	    if(pool->idle){
		    obj = pool->free->next;
	        list_del(obj);
		    pool->idle--;
	    }else{ /*the pool reach limitation*/
		    void* p = malloc(size);
		    pthread_mutex_unlock(&pool->lock);
		    return p;
	    }

	    pthread_mutex_unlock(&pool->lock);
    }
	return (void*)obj;
}

void mempool_free(struct Mempool* pool,void* p)
{
	struct list_head *pos;
	struct Slab *slab;
    if(pthread_mutex_lock(&pool->lock) == 0){
	    list_for_each(pos,pool->slab) {
		    slab=list_entry(pos, struct Slab, node);
		    if( p >= slab->mem && p < slab->end){
			    list_add_tail(((struct list_head*)p),pool->free);
				pool->idle++;
			    pthread_mutex_unlock(&pool->lock);
			    return;
		    }
	    }

	    free(p); /*this pointer not in our pool*/
	    pthread_mutex_unlock(&pool->lock);
    }
}

void* mempool_grow(struct Mempool* pool,unsigned int size,unsigned int num)
{
	int i;
	struct Slab *slab = NULL;
	struct list_head* obj = NULL;
	slab = (struct Slab*)malloc(sizeof(struct Slab));
    if(pthread_mutex_unlock(&pool->lock) == 0){
	    if(slab){
		    memset(slab,0,sizeof(struct Slab));
		    slab->mem = malloc(size*num);
		    slab->end = slab->mem + size*num;
		    for(i=0;i<size*num;i+=pool->size){
			    obj = (struct list_head*)(slab->mem+i);
			    list_add_tail(obj,pool->free);
		    }
		    slab->idle = num;
		    pool->idle += num;
		    pool->count += num;

		    list_add_tail(((struct list_head*)slab),pool->slab);
	    }

	    pthread_mutex_unlock(&pool->lock);
    }
	return (void*)slab;
}

void mempool_destroy(struct Mempool* pool)
{
	struct list_head *p,*n;
    struct Slab* slab;

    if(pool){
    	pthread_mutex_unlock(&pool->lock);
    	list_for_each_safe(p,n,pool->slab){
    		slab=list_entry(p, struct Slab, node);
			if(slab->mem) free(slab->mem);
    		list_del(p);
    		free(slab);
		}

		if(pool->slab) free(pool->slab);
		if(pool->free) free(pool->free);
		if(pool->full) free(pool->full);
		pool->slab = NULL;
		pool->free = NULL;
		pool->full = NULL;

		pthread_mutex_unlock(&pool->lock);
		pthread_mutex_destroy(&pool->lock);

		free(pool);
		pool = NULL;

    }
}
