/*
 * Kad Cache
 * Copyright (c), 2009, GuangFu, 
 * 
 * DHT Bytes Buffer
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: buffer.h 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <time.h>
#include <sys/time.h>
#include "types.h"
#include "atomic.h"

struct packet;
struct buffer
{
	/* These two members must be first. */
    struct buffer* 		next;		/* next buffer pointer */
    struct buffer* 		prev;		/* prev buffer pointer */
    struct packet* 		pkt;		/* transport layer packet */
    unsigned int 		src;		/* buffer source from*/
    unsigned int 		dst;		/* buffer destination to*/
    unsigned char 		type;		/* category type buffer */
    unsigned short  	flag;		/* buffer bitmap flags*/
    unsigned short  	priority;   /* priority 0 ~ 16 class*/
    struct timeval 		tstamp;		/* timestamp we arrived */
	unsigned int 		size;		/* buffer capacity size*/
	unsigned int 		dlen;		/* buffer data length*/
	unsigned char*		head,    	/* head of buffer*/
	unsigned char* 		data;		/* data head pointer*/
    unsigned char* 		ptr;		/* buffer cursor pointer*/    
	unsigned char*		end;		/* last of buffer pointer*/
    atomic_t 			refcnt;		/* reference counter*/
    void* 				owner;		/* buffer owner pointer*/
};

struct buffer_head 
{
	/* These two members must be first. */
	struct buffer*		next;		/* next buffer pointer */
	struct buffer*		prev;		/* prev buffer pointer */
	unsigned int 	   	qlen;		/* buffer queue length*/
	pthread_mutex_t 	lock;       /* buffer queue lock*/
};
		
/* buffer_alloc
 *
 * Allocate a buffer from memory pool
 */
struct buffer* buffer_alloc(size_t size);

/* buffer_free
 *
 * Free a buffer to memory pool
 */
void buffer_free(struct buffer* buf);

/* buffer_push
 *
 * Push a block region size to buffer, return the block start pointer
 */
void *buffer_push(struct buffer *buf, size_t size);

/* buffer_pop
 *
 * Pop a block region size, return the block end pointer
 */
void* buffer_pop(struct buffer *buf, size_t size);

/* buffer_get
 *
 * Get current buffer data pointer
 */
void* buffer_get(struct buffer *buf);

/* buffer_size
 * 
 * Return the current size used
 */
int buffer_size(struct buffer *buf);

/* buffer_capacity
 *
 * Return the buffer true capacity size
 */
int buffer_capacity(struct buffer *buf);


#endif /* __BUFFER_H__*/

