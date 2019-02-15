
#ifndef _SLAB_H_
#define _SLAB_H_

#include <config.h>
#include <core.h>


typedef struct slab_pages slab_page_t;

struct slab_pages {
    void*     		slab;
    slab_page_t*	next;
    void*       	prev;
};

typedef struct
{
    atomic_t    	lock;
    size_t          min_size;
    size_t          min_shift;
    slab_page_t*    pages;
    slab_page_t   	free;
    u_char*			start;
    u_char*			end;
    shmtx_t       	mutex;
} slab_pool_t;


void slab_init(slab_pool_t *pool);
void *slab_alloc(slab_pool_t *pool, size_t size);
void *slab_alloc_locked(slab_pool_t *pool, size_t size);
void slab_free(slab_pool_t *pool, void *p);
void slab_free_locked(slab_pool_t *pool, void *p);


#endif /* _SLAB_H_ */
