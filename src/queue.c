/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* Queue implementation
* 
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
*
* $Id: queue.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
*
*/

#include "queue.h"
#include "log.h"
#include "epoll.h"

struct queue_t* queue_create(unsigned int capacity)
{
    int ret;
    struct queue_t *queue = NULL;

    queue = (struct queue_t*)malloc(sizeof(struct queue_t));
    memset(queue,0,sizeof(struct queue_t));

    ret = pthread_mutex_init(&queue->qlock,NULL);
    if (ret != Q_SUCCESS) {
        free(queue);
        return NULL;
    }

    ret = pthread_cond_init(&queue->not_empty, NULL);
    if (ret != Q_SUCCESS) {
        free(queue);
        return NULL;
    }

    ret = pthread_cond_init(&queue->not_full, NULL);
    if (ret != Q_SUCCESS) {
        free(queue);
        return NULL;
    }

    /* init members of queue to NULL */
    queue->fds = malloc(capacity * sizeof(int)+100);
    queue->data = malloc(capacity * sizeof(void*));
    memset(queue->fds,0,capacity * sizeof(int));
    queue->size = capacity;
    queue->count = 0;
    queue->in = 0;
    queue->out = 0;
    queue->terminated = 0;
    queue->full_waiters = 0;
    queue->empty_waiters = 0;

    return queue;
}

int queue_push(struct queue_t *queue, void *data)
{
    int ret;

    if (queue->terminated) {
        return Q_STOP; /* out of service */
    }

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    if(queue->flags == 1){
        struct epoll_task* task = (struct epoll_task*)data;
        if(queue->fds[task->fd] == task->fd){
            free(task);
            goto exit;
        }
        queue->fds[task->fd] = task->fd;
    }

    /* wait free space*/
    while (queue->count >= queue->size){
        queue->full_waiters++;
        ret = pthread_cond_wait(&queue->not_full, &queue->qlock);
        queue->full_waiters--;
        if (queue->terminated)
            goto exit;  /* exit cond waiting*/
        if(ret != 0) /* condition error*/
            goto exit;
    }

    queue->data[queue->in] = data;
    queue->in = (queue->in + 1) % queue->size;
    queue->count++;

    if (queue->empty_waiters) {
        ret = pthread_cond_signal(&queue->not_empty);
        if (ret != Q_SUCCESS) {
            pthread_mutex_unlock(&queue->qlock);
            return ret;
        }
    }

exit:
    pthread_mutex_unlock(&queue->qlock);
    return ret;
}

int queue_pushwait(struct queue_t *queue, void *data,long sec)
{
    int ret;
    struct timespec timeout;

    if (queue->terminated) {
        return Q_STOP; /* out of service */
    }

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    /* wait free space*/
    while (queue->count >= queue->size){
        timeout.tv_sec = time(NULL)+sec;
        timeout.tv_nsec = 0;
        queue->full_waiters++;
        ret = pthread_cond_timedwait(&queue->not_full, &queue->qlock,&timeout);
        queue->full_waiters--;
        if (queue->terminated)
            goto exit;  /* exit cond waiting*/
        if(ret == ETIMEDOUT)
            goto exit;  /* wait timed expire */
        if(ret != 0)
            goto exit;  /* condition error*/
    }

    queue->data[queue->in] = data;
    queue->in = (queue->in + 1) % queue->size;
    queue->count++;

    if (queue->empty_waiters) {
        pthread_cond_signal(&queue->not_empty);
    }

exit:
    pthread_mutex_unlock(&queue->qlock);
    return ret;
}

int queue_trypush(struct queue_t *queue, void *data)
{
    int ret;

    if (queue->terminated) {
        return Q_STOP; /* out of service,exit */
    }

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    if (queue_full(queue)) {
        ret = pthread_mutex_unlock(&queue->qlock);
        return Q_EAGAIN;
    }

    queue->data[queue->in] = data;
    queue->in = (queue->in + 1) % queue->size;
    queue->count++;

    if (queue->empty_waiters) {
        ret  = pthread_cond_signal(&queue->not_empty);
        if (ret != Q_SUCCESS) {
            pthread_mutex_unlock(&queue->qlock);
            return ret;
        }
    }

    ret = pthread_mutex_unlock(&queue->qlock);
    return ret;
}

unsigned int queue_size(struct queue_t *queue) 
{
    return queue->count;
}

int queue_pop(struct queue_t *queue, void **data)
{
    int ret;

    if (queue->terminated) {
        return Q_STOP; /* out of service,exit */
    }

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    while (queue->count <= 0) {
        queue->empty_waiters++;
        ret = pthread_cond_wait(&queue->not_empty, &queue->qlock);
        queue->empty_waiters--;
        if (queue->terminated)
            goto exit; /* queue stop*/
        if(ret != 0){
            /* condition wait error*/
            goto exit;
        }
    }

    *data = queue->data[queue->out];
    queue->count--;

    if(queue->flags == 1){
        struct epoll_task* task = (struct epoll_task*)(*data);
        queue->fds[task->fd] = 0;
    }

    queue->out = (queue->out + 1) % queue->size;
    if (queue->full_waiters) {
        ret = pthread_cond_signal(&queue->not_full);
        if (ret != Q_SUCCESS) {
            pthread_mutex_unlock(&queue->qlock);
            return ret;
        }
    }

exit:
    ret = pthread_mutex_unlock(&queue->qlock);
    return ret;
}

int queue_trypop(struct queue_t *queue, void **data)
{
    int ret;

    if (queue->terminated) {
        return Q_STOP; /* out of service,exit */
    }

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    if (queue_empty(queue)) {
        ret = pthread_mutex_unlock(&queue->qlock);
        return Q_EAGAIN;
    }

    *data = queue->data[queue->out];
    queue->count--;

    queue->out = (queue->out + 1) % queue->size;
    if (queue->full_waiters) {
        ret = pthread_cond_signal(&queue->not_full);
        if (ret != Q_SUCCESS) {
            pthread_mutex_unlock(&queue->qlock);
            return ret;
        }
    }

    ret = pthread_mutex_unlock(&queue->qlock);
    return ret;
}

int queue_interrupt_all(struct queue_t *queue)
{
    int ret;
    if ((ret = pthread_mutex_lock(&queue->qlock)) != Q_SUCCESS) {
        return ret;
    }
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);

    if ((ret = pthread_mutex_unlock(&queue->qlock)) != Q_SUCCESS) {
        return ret;
    }

    return Q_SUCCESS;
}

int queue_term(struct queue_t *queue)
{
    int ret;

    ret = pthread_mutex_lock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }

    queue->terminated = 1;
    ret = pthread_mutex_unlock(&queue->qlock);
    if (ret != Q_SUCCESS) {
        return ret;
    }
    return queue_interrupt_all(queue);
}

int queue_destroy(struct queue_t *queue)
{
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->qlock);

    if(queue->fds)
        free(queue->fds);
    if(queue->data)
        free(queue->data);
    if(queue)
        free(queue);
    return Q_SUCCESS;
}

