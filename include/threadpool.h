/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Thread pool declares
 *
 * wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: threadpool.h,v 1.3 2008-12-24 04:07:19 wxlin Exp $
 */
 
#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREADS_IN_POOL 10

typedef void *ThreadPool;

typedef void* (*thread_func)(void *);

extern ThreadPool alloc_threadpool();
extern int init_threadpool(ThreadPool tpool,int num);
extern int request_threadpool(ThreadPool tp, thread_func fn, void *arg);
extern void destroy_threadpool(ThreadPool tp);

#ifdef __cplusplus
}
#endif

#endif // __THREAD_POOL_H__

