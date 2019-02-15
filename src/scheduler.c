/**
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 *
 * implementation of scheduler queue
 *
 * $Id: scheduler.c,v 1.12 2009-01-14 03:22:44 hcai Exp $
 *
 */

#include "scheduler.h"
#include "message.h"
#include "piranha.h"
#include "mempool.h"
#include "log.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>

int async_request_wait( AsyncRequest* request,struct timeval* expire);
void* piranha_scheduler_svc(void* param);

//
// implementation of async request
//
Invocation* alloc_invocation(ActiveQueue* queue)
{
	Invocation* invocation;
	invocation = (Invocation*)mempool_alloc(queue->ipool,sizeof(Invocation));
	if(invocation){
		memset(invocation,0,sizeof(Invocation));
	}
	return invocation;
}

inline void free_invocation(ActiveQueue* queue,Invocation* invocation)
{	
	if(invocation){
		if (!invocation->keepmsg)
			buffer.free(invocation->msg); // test
		mempool_free(queue->ipool,invocation);
	}
}

AsyncRequest* alloc_async_request(ActiveQueue* queue,call_func pfun)
{
	AsyncRequest* request;
	request = (AsyncRequest*)mempool_alloc(queue->rpool,sizeof(AsyncRequest));
	if(request){
		memset(request,0,sizeof(AsyncRequest));
		request->call = pfun;
		if(queue->flags != NO_NOTIFY){
		pthread_mutex_init(&request->lock, NULL);
		pthread_cond_init(&request->cond,NULL);
		request->async_wait = async_request_wait;
		}
	}
	return request;
}

void free_async_request(ActiveQueue* queue,AsyncRequest* request)
{
	if(queue->flags != NO_NOTIFY){
		pthread_cond_broadcast(&request->cond);
		pthread_mutex_lock(&request->lock);
		pthread_cond_destroy(&request->cond);
		pthread_mutex_unlock(&request->lock);
		pthread_mutex_destroy(&request->lock);
	}
	mempool_free(queue->rpool,request);
}

int async_request_wait(AsyncRequest* request,struct timeval* expire)
{
	struct timeval now;
	struct timespec timeout;
	int retcode = 0;

	pthread_mutex_lock(&request->lock);
	gettimeofday(&now,NULL);
	timeout.tv_sec = now.tv_sec + expire->tv_sec;
	timeout.tv_nsec = now.tv_usec * 1000;

	while (request->complete==false && retcode != ETIMEDOUT) {
		retcode = pthread_cond_timedwait(
			&request->cond, &request->lock, &timeout);
	}
	pthread_mutex_unlock(&request->lock);

	if (retcode == ETIMEDOUT) {
		return -1; /*time out*/
	}

	return 0; /*success*/
}

int async_request_notify(AsyncRequest* request)
{
	pthread_mutex_lock(&request->lock);
	request->complete = true;
	pthread_cond_signal(&request->cond);
	pthread_mutex_unlock(&request->lock);
	return 0;
}

//
//  active queue for async call
//

ActiveQueue* alloc_active_queue()
{
	ActiveQueue* queue = NULL;
	queue = (ActiveQueue*)malloc(sizeof(ActiveQueue));
	if(queue){
		memset(queue,0,sizeof(ActiveQueue));
		queue->max = MAX_ACTIVE_QUEUE-1;
		queue->list = malloc(sizeof(struct list_head));
		INIT_LIST_HEAD(queue->list);
		queue->ipool = mempool_init(sizeof(Invocation),100,MAX_ACTIVE_QUEUE);
		queue->rpool = mempool_init(sizeof(AsyncRequest),100,MAX_ACTIVE_QUEUE);
	}
	return queue;
}

int active_queue_init(ActiveQueue* queue)
{
	if(pthread_mutex_init(&queue->lock, NULL) != 0)
		return -1;
	if(pthread_cond_init(&queue->cond,NULL) != 0)
		return -1;

	queue->count = 0;
	queue->active = 0;
	queue->status = 0;
	return 0;
}

int active_queue_put(ActiveQueue* queue,AsyncRequest* request,struct timeval* expire)
{
	struct timeval now;
	struct timespec timeout;
	int status = 0;

	gettimeofday(&now,NULL);
	timeout.tv_sec = now.tv_sec + expire->tv_sec;
	timeout.tv_nsec = now.tv_usec * 1000;

	pthread_mutex_lock(&queue->lock);
	while (queue->count > queue->max) {
		status = pthread_cond_timedwait(
			&queue->cond, &queue->lock, &timeout);
		if(status == ETIMEDOUT){
			/*Condition wait timed out */
			break;
		}
		if(status != 0){
			/* Condition wait error*/
			break;
		}
	}

	if(status == 0){ /*success*/
		list_add_tail(((struct list_head*)request),queue->list);
		queue->count++;
        if(queue->count == 1)
        pthread_cond_signal(&queue->cond);
	}
	pthread_mutex_unlock(&queue->lock);

	return status;
}

AsyncRequest* active_queue_get(ActiveQueue* queue,struct timeval* expire)
{
	struct timeval now;
	struct timespec timeout;
	AsyncRequest* request = NULL;
	int status = 0;

	gettimeofday(&now,NULL);
	timeout.tv_sec = now.tv_sec + expire->tv_sec;
	timeout.tv_nsec = now.tv_usec * 1000;

	pthread_mutex_lock(&queue->lock);
	while (queue->count >= 0) {
		status = pthread_cond_timedwait(
			&queue->cond, &queue->lock, &timeout);
		if(status == ETIMEDOUT){
			/*Condition wait timed out */
			break;
		}
		if(status != 0){
			/* Condition wait error*/
			break;
		}
	}

	if (status == 0){ /*success*/
		request = list_first_entry(queue->list,AsyncRequest,list);
		queue->count--;
	}

	pthread_mutex_unlock(&request->lock);
	return request;
}

int active_queue_is_empty (ActiveQueue* queue)
{
	bool is_empty = false;
	if (pthread_mutex_lock(&queue->lock) ==0){
		if (list_empty(queue->list))
			is_empty = true;
		pthread_mutex_unlock(&queue->lock);
	}
	return is_empty;
}

int active_queue_is_full (ActiveQueue* queue)
{
	bool is_full = false;
	if (pthread_mutex_lock(&queue->lock) ==0){
		if (queue->count > queue->max)
			is_full = true;
		pthread_mutex_unlock(&queue->lock);
	}
	return is_full;
}

void* active_queue_svc(void* param)
{
	ActiveQueue* queue = (ActiveQueue*)param;
	AsyncRequest* request = NULL;

	pthread_mutex_lock(&queue->lock);
	while (queue->active)
	{
		while (list_empty(queue->list)){
			pthread_cond_wait(&queue->cond, &queue->lock);
		}

		if (!queue->active)
		  break;

		request = list_first_entry(queue->list,AsyncRequest,list);
		if(request){
			list_del(((struct list_head*)request));
			queue->count--;
			pthread_mutex_unlock(&queue->lock);
			/* do our call routing here*/
			request->result = request->call(request->param);
		}
		pthread_mutex_lock(&queue->lock);
	}
	pthread_mutex_unlock(&queue->lock);

  return NULL;

}

int active_queue_start(ActiveQueue* queue)
{
	if (pthread_mutex_lock(&queue->lock) ==0){
		queue->active = 1;
		pthread_mutex_unlock(&(queue->lock));
		pthread_cond_broadcast(&(queue->cond));
		return 0;
	}
	return -1;
}

int active_queue_stop(ActiveQueue* queue)
{
	if (pthread_mutex_lock(&queue->lock) ==0){
		/*avoid empty queue waiting*/
		AsyncRequest* request_stop;
		request_stop = alloc_async_request(queue,NULL);
		request_stop->priority = 0xffffffff;
		queue->count++;
		list_add_tail(((struct list_head*)request_stop),queue->list);
		queue->active = 0;

		pthread_cond_broadcast(&queue->cond);
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}
	return -1;
}

int active_queue_close(ActiveQueue* queue)
{
	AsyncRequest* request;
	pthread_mutex_lock(&queue->lock);
	while (list_empty(queue->list)){
		request = list_first_entry(queue->list,AsyncRequest,list);
		if(request){
			list_del(((struct list_head*)request));
		}
	}
	mempool_destroy(queue->ipool);
	mempool_destroy(queue->rpool);
	pthread_mutex_unlock(&queue->lock);
	pthread_cond_destroy(&queue->cond);
	pthread_mutex_destroy(&queue->lock);

	queue->status = 0;
	queue->count = 0;
	queue->active = 0;
	return 0;
}

//
//  event adapter for dispatch
//

EventAdapter* alloc_event_adapter()
{
	EventAdapter* adapter = NULL;
	adapter = (EventAdapter*)malloc(sizeof(EventAdapter));
	if(adapter){
		memset(adapter,0,sizeof(EventAdapter));
	}
	return adapter;
}

int event_adapter_init(EventAdapter* adapter)
{
	int retcode;
	adapter->max_event = MAX_EVENT;
	adapter->event_map = hash_new(7);
	retcode = pthread_mutex_init(&adapter->lock, NULL);
	return retcode;
}

int event_adapter_finit(EventAdapter* adapter)
{
	int retcode = -1;
	if(pthread_mutex_lock(&adapter->lock) == 0){
		void *args = hash_first(adapter->event_map);
		while (args) {
			args = hash_next(adapter->event_map);
		}
		hash_destroy(adapter->event_map);
		adapter->event_map = NULL;
		pthread_mutex_unlock(&adapter->lock);
		retcode = 0;
	}
	pthread_mutex_destroy(&adapter->lock);
	return retcode;
}

void* event_adapter_get(EventAdapter* adapter,int id)
{
	void* handle = NULL;
	if(pthread_mutex_lock(&adapter->lock) == 0){
		handle = hash_get(adapter->event_map, id);
		pthread_mutex_unlock(&adapter->lock);
	}
	return handle;
}

int event_adapter_register(EventAdapter* adapter,int id,void *handle)
{
	if(pthread_mutex_lock(&adapter->lock) == 0){
		hash_set(adapter->event_map, id, handle);
		pthread_mutex_unlock(&adapter->lock);
		return 0;
	}
	return -1;
}

int event_adapter_unregister(EventAdapter* adapter,int id)
{
	if(pthread_mutex_lock(&adapter->lock) == 0){
		hash_remove(adapter->event_map, id);
		pthread_mutex_unlock(&adapter->lock);
		return 0;
	}
	return -1;
}

//
// Piranha Scheduler
//

Scheduler* scheduler_alloc()
{
	Scheduler* scheduler = NULL;
	scheduler = (Scheduler*)malloc(sizeof(Scheduler));
	if(scheduler){
		memset(scheduler,0,sizeof(Scheduler));
		scheduler->queue = alloc_active_queue();
		scheduler->adapter = alloc_event_adapter();
	}
	return scheduler;
}

int init_scheduler(Scheduler* scheduler)
{
    if(!scheduler)
    	return -1;
	if(active_queue_init(scheduler->queue) != 0)
		return -1;
	if(event_adapter_init(scheduler->adapter) !=0)
		return -1;

	scheduler->queue->flags = NO_NOTIFY;
	return 0;
}

AsyncRequest* scheduler_get(Scheduler* scheduler)
{
	AsyncRequest* request = NULL;
	ActiveQueue* queue = scheduler->queue;

	if (!list_empty(queue->list)){
		request = list_first_entry(queue->list,AsyncRequest,list);
		if(request){
			list_del(((struct list_head*)request));
			queue->count--;
		}
	}

	if(queue->count<=0)
		pthread_cond_signal(&queue->cond);

	return request;
}

void _scheduler_put(Scheduler* scheduler,void* piranha, Host* host,int* keepmsg, Buffer* buf)
{
    EventAdapter* adapter = scheduler->adapter;
    char messagename[64];	
    event_func handle;

    buffer.touch(buf, "piranha->scheduler_svc");
    Message *msg = (Message *) buffer.get(buf);
    handle = (event_func)event_adapter_get(adapter,msg->header.type);
    if(handle){
        LOG("Scheduler: %s from %s\n",MessageTypeName(msg, messagename, sizeof(messagename)),inet_ntoa(host->sin_addr));
        handle(piranha,host,keepmsg,buf);
    }else{
        BUG("Message %d unregistered.\n",msg->header.type);
    }
	if (!(*keepmsg))
		buffer.free(buf);
}	

AsyncRequest* scheduler_put(Scheduler* scheduler,void* piranha, Host* host,int* keepmsg, Buffer* buf)
{
	struct timeval now;
	struct timeval timeout;
	Invocation* invocation = NULL;
	AsyncRequest* request = NULL;
	ActiveQueue* queue = scheduler->queue;

	invocation = alloc_invocation(queue);
	invocation->param = piranha;
	host_assign(&invocation->from, host);
	invocation->keepmsg = *keepmsg;
	invocation->msg = buf;

	gettimeofday(&now,NULL);
	timeout.tv_sec = 100000;

	request = alloc_async_request(queue,NULL);
	if(invocation && request){
		request->queue = queue;
		request->param = (void*)invocation;
		active_queue_put(queue,request,&timeout);
		return request;
	}

	/* if failure, free previous allocation*/
	if(invocation)
        free_invocation(queue,invocation);		
	if(request)
        free_async_request(queue,request);
	return NULL;
}

void* scheduler_svc(void* param)
{
	Scheduler* scheduler = (Scheduler*)param;
	ActiveQueue* queue = scheduler->queue;
	EventAdapter* adapter = scheduler->adapter;

	pthread_mutex_lock(&queue->lock);
	while (queue->active)
	{
		while (queue->count<=0){
			pthread_cond_wait(&queue->cond, &queue->lock);
		}

		if (!queue->active)
			break;

		AsyncRequest* request = scheduler_get(scheduler);
		pthread_mutex_unlock(&queue->lock);
		if(request != NULL){
			/* do our call routing here*/
			Invocation* invocation = (Invocation*)request->param;
			if(invocation){
				Message *msg;
				event_func handle;
				buffer.touch(invocation->msg, "piranha->scheduler_svc");
				msg = (Message *) buffer.get(invocation->msg);
				handle = (event_func)event_adapter_get(adapter,msg->header.type);
				if(handle){
					handle(invocation->param,
						   &invocation->from,
						   &invocation->keepmsg,
						   invocation->msg);
					request->result = 0;
					free_invocation(queue,invocation);
					if(queue->flags != NO_NOTIFY)
					async_request_notify(request);
					free_async_request(queue,request);					
				}else{
					BUG("Message %d unregistered.\n",msg->header.type);
				}
			}
		}
		pthread_mutex_lock(&queue->lock);
	}
	pthread_mutex_unlock(&queue->lock);
	
	DBG("Message scheduler service exit.\n");
	return NULL;
}

inline int scheduler_regsiter(Scheduler* scheduler,int id,event_func handle)
{
	return event_adapter_register(scheduler->adapter,id,(void*)handle);
}

inline int scheduler_unregsiter(Scheduler* scheduler,int id)
{
	return event_adapter_unregister(scheduler->adapter,id);
}

int start_scheduler(Scheduler* scheduler)
{
	scheduler->queue->active = 1;
	if(pthread_create(&scheduler->worker,NULL,
		scheduler_svc,(void*)scheduler) == 0)
	{
		return 0;
	}
	return -1;
}

int stop_scheduler(Scheduler* scheduler)
{
	return active_queue_stop(scheduler->queue);
}

int destroy_scheduler(Scheduler* scheduler)
{
	active_queue_close(scheduler->queue);
	event_adapter_finit(scheduler->adapter);
    free(scheduler->adapter);
    free(scheduler->queue);
	free(scheduler);
	scheduler = NULL;
	return 0;
}

