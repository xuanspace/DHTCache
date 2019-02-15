/*
 * Kad Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * DHT Bytes Buffer
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: buffer.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */
#include "buffer.h"
#include "list.h"
#include "log.h"

/* 
 * buffer_alloc
 *
 * Allocate a buffer from memory pool, and init the members
 * return buffer pointer.
 */
struct buffer* buffer_alloc(size_t size) 
{
	struct buffer* buf = NULL;

	buf = (struct buffer*)malloc(sizeof(struct buffer));
	if(buf != NULL){
		memset(buf, 0, sizeof(struct buffer));
		if (size > 0)
			buf->data = (unsigned char*)malloc(size);
		else
			buf->data = NULL;
		
		buf->size = size;
		buf->head = buf->data;
		buf->ptr = buf->data;
		buf->end = buf->data + size;
	}
	
	return buf;
}

/* 
 * buffer_free
 *
 * free the buffer's data and the buffer itself 
 */
void buffer_free(struct buffer* buf) 
{
	if (buf == NULL)
		return;

	if (buf->data)
		free(buf->data)	
		
	free(buf);
}

/* 
 * buffer_resize
 *
 * realloc the buffer size, and reset buffer
 */
void buffer_resize(struct buffer *buf, size_t newsize) {
{
	buf->data = realloc(buf->data, newsize);
	buf->size = newsize;
	buf->size = MIN(newsize, buf->len);
}

/*
 *  buffer_copy
 *
 * Create a copy of buf, allocating required memory etc.
 * The new buffer is sized the same as the length of the source buffer.
 */
struct buffer* buffer_copy(struct buffer* buf) 
{
	struct buffer* rbuf;
	rbuf = buffer_new(buf->size);
	if(rbuf != NULL){
		rbuf->size = buf->size;
		memcpy(rbuf->data, buf->data, buf->size);
	}
	return rbuf;
}

/*
 *  buffer_shared
 *
 *  Is the buffer shared
 *  Returns true if more than one person has a reference to this buffer.
 */
static inline int buffer_shared(const struct buffer *buf)
{
	return atomic_read(&buf->refcnt) != 1;
}

/*
 *	buffer_ref - reference buffer
 *
 *	Makes another reference to a  buffer and returns a pointer
 *	to the buffer.
 */
static inline struct buffer *buffer_ref(struct buffer *skb)
{
	atomic_inc(&skb->refcnt);
	return skb;
}

static inline unsigned char *buffer_push(struct buffer *buf, unsigned int len)
{
	buf->data -= size;
	buf->size  += size;
	return buf->data;
}

/* 
 * buffer_get
 *
 * Get current buffer data pointer
 */
static inline void* buffer_get(struct buffer *buf)
{
	return buf->data;
}

/* 
 *  buffer_size
 *
 * Return the current size used
 */
static inline int buffer_size(struct buffer *buf)
{
	return buf->dlen;
}

/* 
 * buffer_capacity
 *
 * Return the buffer true capacity size
 */
static inline int buffer_capacity(struct buffer *buf)
{
	return buf->size;
}

/*
 *	buffer_tailroom - bytes at buffer end
 *	@buf: buffer to check
 *
 *	Return the number of bytes of free space at the tail of an buffer
 */
static inline int buffer_tailroom(const struct buffer *buf)
{
	return buf->data_len ? 0 : buf->end - buf->tail;
}

static inline unsigned int buffer_headlen(const struct buffer *buf)
{
	return buf->len - buf->data_len;
}

/*
 *	buffer_reserve - adjust headroom
 *	@buf: buffer to alter
 *	@len: bytes to move
 *
 *	Increase the headroom of an empty &buffer by reducing the tail
 *	room. This is only allowed for an empty buffer.
 */
static inline void buffer_reserve(struct buffer *buf, int len)
{
	buf->data += len;
	buf->tail += len;
}

static inline unsigned char *buffer_end_ptr(const struct buffer *buf)
{
	return buf->head + buf->end;
}

/*
 *	buffer_queue_head_init
 *
 *	Init an buffer queue.
 *
 */
static inline void buffer_queue_head_init(struct buffer_head *list)
{
	pthread_mutex_init((&list->lock, NULL);
	list->prev = list->next = (struct buffer *)list;
	list->qlen = 0;
}

/*
 *	buffer_peek
 *
 *	Peek an buffer. .
 *
 *	Returns %NULL for an empty list or a pointer to the head element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */
static inline struct buffer *buffer_peek(struct buffer_head *list_)
{
	struct buffer *list = ((struct buffer *)list_)->next;
	if (list == (struct buffer *)list_)
		list = NULL;
	return list;
}

/*
 *	buffer_peek_tail
 *
 *	Peek an &buffer. 
 *
 *	Returns %NULL for an empty list or a pointer to the tail element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */
static inline struct buffer *buffer_peek_tail(struct buffer_head *list_)
{
	struct buffer *list = ((struct buffer *)list_)->prev;
	if (list == (struct buffer *)list_)
		list = NULL;
	return list;
}

/*
 *	buffer_queue_len
 *
 *	Return the length of an buffer queue.
 */
static inline unsigned int buffer_queue_len(const struct buffer_head *list_)
{
	return list_->qlen;
}

/*
 *	buffer_queue_after
*	queue a buffer at the list head
 *
 *	@list: list to use
 *	@prev: place after this buffer
 *	@news: buffer to queue
 *
 *	Queue a buffer int the middle of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */
static inline void buffer_queue_after(struct buffer_head *list,
				     struct buffer *prev,struct buffer *news)
{
	struct buffer *next;
	list->qlen++;

	next = prev->next;
	news->next = next;
	news->prev = prev;
	next->prev  = prev->next = news;
}

/*
 *	buffer_queue_head 
 *      queue a buffer at the list head
 *
 *	@list: list to use
 *	@news: buffer to queue
 *
 *	Queue a buffer at the start of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */
extern void buffer_queue_head(struct buffer_head *list, struct buffer *news);
{
	buffer_queue_after(list, (struct buffer *)list, news);
}

/*
 *	buffer_queue_tail
*	queue a buffer at the list tail
*
 *	@list: list to use
 *	@news: buffer to queue
 *
 *	Queue a buffer at the end of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */
static inline void buffer_queue_tail(struct buffer_head *list,
				   struct buffer *news)
{
	struct buffer *prev, *next;

	list->qlen++;
	next = (struct buffer *)list;
	prev = next->prev;
	news->next = next;
	news->prev = prev;
	next->prev  = prev->next = news;
}

static inline struct buffer* buffer_dequeue(struct buffer_head *list)
{
	struct buffer_head *next, *prev, *result;

	prev = (struct buffer_head*) list;
	next = prev->next;
	result = NULL;
	
	if (next != prev) {
		result = next;
		next = next->next;
		list->qlen--;
		next->prev = prev;
		prev->next = next;
		result->next = result->prev = NULL;
	}
	return result;
}

/*
 *	buffer_insert
 *	Insert a packet on a list.
 */
static inline void buffer_insert(struct buffer *news,
				struct buffer *prev, struct buffer *next,
				struct buffer_head *list)
{
	news->next = next;
	news->prev = prev;
	next->prev  = prev->next = news;
	list->qlen++;
}

/*
 *	buffer_dequeue_tail 
 *	Place a packet after a given packet in a list.
 */
static inline void buffer_append(struct buffer *old, struct buffer *news, struct buffer_head *list)
{
	buffer_insert(news, old, old->next, list);
}

/*
 * remove buffer from list. _Must_ be called atomically, and with
 * the list known..
 */
static inline void buffer_unlink(struct buffer *buf, struct buffer_head *list)
{
	struct buffer *next, *prev;

	list->qlen--;
	next = buf->next;
	prev = buf->prev;
	buf->next = NULL;
	buf->prev = NULL;
	next->prev = prev;
	prev->next = next;
}

/*
 *	buffer_dequeue_tail 
 *     remove from the tail of the queue
 *
 *	@list: list to dequeue from
 *
 *	Remove the tail of the list. This function does not take any locks
 *	so must be used with appropriate locks held only. The tail item is
 *	returned or %NULL if the list is empty.
 */
static inline struct buffer *buffer_dequeue_tail(struct buffer_head *list)
{
	struct buffer *buf = skb_peek_tail(list);
	if (buf != NULL)
		buffer_unlink(buf, list);
	return buf;
}
