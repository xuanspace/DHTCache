/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 *
 * implementation of a threadpool.
 * 
 * wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: threadpool.c,v 1.3 2008-12-30 09:42:57 wxlin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "threadpool.h"
#include "log.h"

struct thread_t{
    pthread_t id;
    thread_func fn;
    void *arg;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void* parent;
};

struct threadpool_t{
    int tp_index;
    int tp_max;
    int tp_stop;
    int tp_total;
    pthread_mutex_t tp_mutex;
    pthread_cond_t tp_idle;
    pthread_cond_t tp_full;
    pthread_cond_t tp_empty;
    struct thread_t** tp_list;
    void* owner;
};

typedef struct thread_t thread_t;
typedef struct threadpool_t threadpool_t;

ThreadPool alloc_threadpool()
{
    threadpool_t *pool = NULL;
    pool = (threadpool_t *) malloc(sizeof(threadpool_t));
    if (pool == NULL) {
        DBG("TPOOL: Out of memory creating a new threadpool!\n");
        return NULL;
    }
    return pool;
}

int init_threadpool(ThreadPool tpool,int num)
{
    threadpool_t *pool = tpool;

    if ((num <= 0) || (num > MAX_THREADS_IN_POOL))
        return -1;

    pthread_mutex_init(&pool->tp_mutex, NULL);
    pthread_cond_init(&pool->tp_idle, NULL);
    pthread_cond_init(&pool->tp_full, NULL);
    pthread_cond_init(&pool->tp_empty, NULL);

    pool->tp_max = num;
    pool->tp_index = 0;
    pool->tp_stop = 0;
    pool->tp_total = 0;
    pool->tp_list = (thread_t**)malloc(sizeof(void*) * MAX_THREADS_IN_POOL);
    memset(pool->tp_list, 0, sizeof(void*) * MAX_THREADS_IN_POOL);

    return 0;
}

int recycle_thread(threadpool_t* pool, thread_t* thread)
{
    int ret = -1;

    pthread_mutex_lock( &pool->tp_mutex );
    if( pool->tp_index < pool->tp_max ) {
        pool->tp_list[ pool->tp_index ] = thread;
        pool->tp_index++;
        ret = 0;

        pthread_cond_signal( &pool->tp_idle );
        if( pool->tp_index >= pool->tp_total ) {
            pthread_cond_signal( &pool->tp_full );
        }
    }
    pthread_mutex_unlock( &pool->tp_mutex );

    return ret;
}

void* worker_thread(void* arg)
{
    thread_t* thread = (thread_t*)arg;
    threadpool_t* pool = (threadpool_t*)thread->parent;

    while(((threadpool_t*)thread->parent)->tp_stop == 0 ) {
        thread->fn( thread->arg );

        if( ((threadpool_t*)thread->parent)->tp_stop != 0 )
            break;

        pthread_mutex_lock( &thread->mutex );
        if( recycle_thread( thread->parent, thread ) == 0 ) {
            pthread_cond_wait( &thread->cond, &thread->mutex );
            pthread_mutex_unlock( &thread->mutex );
        } else {
            pthread_mutex_unlock( &thread->mutex );
            pthread_cond_destroy( &thread->cond );
            pthread_mutex_destroy( &thread->mutex );
            free( thread );
            break;
        }
    }

    pthread_mutex_lock( &pool->tp_mutex );
    pool->tp_total--;
    if( pool->tp_total <= 0 ) 
        pthread_cond_signal( &pool->tp_empty );
    pthread_mutex_unlock( &pool->tp_mutex );

    return 0;
}

int request_threadpool(ThreadPool tp, thread_func fn, void *arg)
{
    int ret = 0;

    threadpool_t *pool = (threadpool_t*) tp;
    pthread_attr_t attr;
    thread_t * thread = NULL;

    pthread_mutex_lock( &pool->tp_mutex );
    while( pool->tp_index <= 0 && pool->tp_total >= pool->tp_max ) {
        pthread_cond_wait( &pool->tp_idle, &pool->tp_mutex );
    }

    if( pool->tp_index <= 0 ) {
        thread_t * thread = ( thread_t * )malloc( sizeof( thread_t ) );
        memset( &( thread->id ), 0, sizeof( thread->id ) );
        pthread_mutex_init( &thread->mutex, NULL );
        pthread_cond_init( &thread->cond, NULL );
        thread->fn = fn;
        thread->arg = arg;
        thread->parent = pool;

        pthread_attr_init( &attr );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

        if( pthread_create( &thread->id, &attr, worker_thread, thread ) == 0) {
            pool->tp_total++;
            DBG("TPOOL: Create thread#%ld\n", thread->id );
        } else {
            ret = -1;
            DBG("TPOOL: Cannot create thread\n" );
            pthread_mutex_destroy( &thread->mutex );
            pthread_cond_destroy( &thread->cond );
            free( thread );
        }
    } else {
        pool->tp_index--;
        thread = pool->tp_list[ pool->tp_index ];
        pool->tp_list[ pool->tp_index ] = NULL;

        thread->fn = fn;
        thread->arg = arg;
        thread->parent = pool;

        pthread_mutex_lock( &thread->mutex );
        pthread_cond_signal( &thread->cond ) ;
        pthread_mutex_unlock ( &thread->mutex );
    }

    pthread_mutex_unlock( &pool->tp_mutex );

    return ret;
}

void destroy_threadpool(ThreadPool tp)
{
    threadpool_t *pool = (threadpool_t*)tp;

    pthread_mutex_lock( &pool->tp_mutex );

    if( pool->tp_index < pool->tp_total ) {
        DBG("TPOOL: Waiting for %d thread(s) to finish.\n", pool->tp_total - pool->tp_index );
        pthread_cond_wait( &pool->tp_full, &pool->tp_mutex );
    }

    int i = 0;
    pool->tp_stop = 1;
    for( i = 0; i < pool->tp_index; i++ ) {
        thread_t * thread = pool->tp_list[ i ];

        pthread_mutex_lock( &thread->mutex );
        pthread_cond_signal( &thread->cond ) ;
        pthread_mutex_unlock ( &thread->mutex );
    }

    if( pool->tp_total > 0 ) {
        DBG("TPOOL: Waiting for %d thread(s) to exit\n", pool->tp_total );
        pthread_cond_wait( &pool->tp_empty, &pool->tp_mutex );
    }

    for( i = 0; i < pool->tp_index; i++ ) {
        free( pool->tp_list[i] );
        pool->tp_list[i] = NULL;
    }

    pthread_mutex_unlock( &pool->tp_mutex );

    pool->tp_index = 0;

    pthread_mutex_destroy( &pool->tp_mutex );
    pthread_cond_destroy( &pool->tp_idle );
    pthread_cond_destroy( &pool->tp_full );
    pthread_cond_destroy( &pool->tp_empty );

    free( pool->tp_list );
    free( pool );

    LOG("TPOOL: Finish destroy.\n");
}
