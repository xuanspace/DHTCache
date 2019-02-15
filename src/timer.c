/**
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* implementation of timer queue
*
* Author(s): wxlin <weixuan.lin@sierraatlantic.com>
*
* $Id: timer.c,v 1.7 2008-12-30 09:41:25 wxlin Exp $
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "timeval.h"
#include "timer.h"
#include "heap.h"
#include "mempool.h"
#include "log.h"

void* timer_schedule(void* param);
void* timer_callback(void* param);

struct callback_queue* alloc_callback_queue(int size)
{
    struct callback_queue* queue = NULL;    
    queue = (struct callback_queue*)malloc(sizeof(struct callback_queue));
    if(queue){
        memset(queue,0,sizeof(struct callback_queue));
        queue->max = MAX_CALLBACK_QUEUE;
        queue->list = malloc(sizeof(struct list_head));
        INIT_LIST_HEAD(queue->list);
    }
    return queue;
}

int callback_queue_init(struct callback_queue* queue)
{
    if(pthread_mutex_init(&queue->lock, NULL) != 0)
        return -1;

    if(pthread_cond_init(&queue->empty, NULL) != 0)
        return -1;

    if(pthread_cond_init(&queue->full, NULL) != 0)
        return -1;

    return 0;
}

int callback_queue_start(struct callback_queue* queue)
{
    queue->running = 1;
    if(pthread_create(&queue->thread,NULL,timer_callback,(void*)queue) != 0)
        return -1;
    return 0;
}

int stop_callback_queue(struct callback_queue* queue)
{
    queue->running = 0;
    if(pthread_cond_broadcast(&queue->empty) != 0)
        return -1;
    if(pthread_cond_broadcast(&queue->full) != 0)
        return -1;		
    if(pthread_join(queue->thread, NULL) !=0)
        return -1;
    return 0;
}

int callback_queue_put(struct callback_queue* queue,struct timer_entry *timer)
{
    int status = 0;
    if(pthread_mutex_lock(&queue->lock) == 0){
        while (queue->count >= queue->max) {
            queue->full_waiters++;
            status = pthread_cond_wait(&queue->empty, &queue->lock);
            queue->full_waiters--;
            if (queue->running == 0)
                goto exit; /* exit cond waiting*/
            if(status != 0)
                goto exit; /* condition wait error*/
        }
        if(status == 0){ /*success*/
            queue->count++;
            list_add_tail((struct list_head*)timer,queue->list);
            if (queue->empty_waiters)
                pthread_cond_signal(&queue->full);
        }
exit:
        pthread_mutex_unlock(&queue->lock);
    }
    return status;
}

struct timer_entry* callback_queue_get(struct callback_queue* queue)
{
    int status = 0;
    struct timer_entry* timer = NULL;

    if(pthread_mutex_lock(&queue->lock) == 0){
        while (queue->count <= 0) {
            queue->empty_waiters++;
            status = pthread_cond_wait(&queue->full, &queue->lock);
            queue->empty_waiters--;
            if (queue->running == 0)
                goto exit; /*exit*/
            if(status != 0){
                /* Condition wait error*/
                goto exit;
            }
        }

        if (status == 0 && queue->running){ /*success*/
            if (!list_empty(queue->list)){
                timer = list_first_entry(queue->list,struct timer_entry,node);
                if(timer){
                    list_del(((struct list_head*)timer));
                    queue->count--;
                    if (queue->full_waiters)
                        pthread_cond_signal(&queue->empty);
                }
            }
        }
exit:
        pthread_mutex_unlock(&queue->lock);
    }
    return timer;
}

void destroy_callback_queue(struct callback_queue* queue)
{
    if(queue){
        pthread_mutex_destroy(&queue->lock);
        pthread_cond_destroy(&queue->empty);
        pthread_cond_destroy(&queue->full);
        list_for_each_free(queue->list);
        free(queue);
    }
}

struct timer_queue* timer_queue_create(int flags)
{
    struct timer_queue* tqueue = NULL;
    tqueue = malloc(sizeof(struct timer_queue));
    if(tqueue){
        memset(tqueue,0,sizeof(struct timer_queue));
        tqueue->max = MAX_TIMER_QUEUE;
        //tqueue->timers = NULL; /*auto alloc*/
        tqueue->timers = (struct timer_entry **)
            malloc(sizeof(struct timer_entry*)*MAX_TIMER_QUEUE);
        tqueue->size = MAX_TIMER_QUEUE;
        tqueue->map = hash_new(7);
        tqueue->cblist =  alloc_callback_queue(0);
        tqueue->cblist->owner = tqueue;
        tqueue->flags = flags; /*default inner id*/
        tqueue->state = TIMER_INIT;
        tqueue->tpool = mempool_init(sizeof(struct timer_entry),
            MAX_TIMER_QUEUE,MAX_TIMER_QUEUE);
        if(tqueue->tpool == NULL)
            WARN("Timer: Alloc timer memory pool failure!\n");
    }
    return tqueue;
}

int timer_queue_init(struct timer_queue* tqueue)
{
    if(tqueue == NULL)
        return -1;
    if(pthread_mutex_init(&tqueue->lock, NULL) != 0)
        return -1;
    if(pthread_cond_init(&tqueue->cond,NULL) != 0)
        return -1;
    if(pthread_cond_init(&tqueue->empty,NULL) != 0)
        return -1;
    if(pthread_cond_init(&tqueue->full,NULL) != 0)
        return -1;
    if(callback_queue_init(tqueue->cblist) != 0)
        return -1;
    return 0;
}

int timer_queue_start(struct timer_queue* tqueue)
{
    if(tqueue == NULL)
        return -1;
    tqueue->running = 1;
    tqueue->state = TIMER_SCHEDULING;
    if(pthread_create(&tqueue->scheduler,NULL,timer_schedule,(void*)tqueue) != 0)
        return -1;
    if(callback_queue_start(tqueue->cblist) !=0)
        return -1;
    return 0;
}

int timer_queue_stop(struct timer_queue* tqueue)
{
    if(tqueue == NULL)
        return -1;
    tqueue->running = 0; /*set stop*/
    if(pthread_cond_broadcast(&tqueue->cond) != 0)
        return -1;
    if(pthread_cond_broadcast(&tqueue->empty) != 0)
        return -1;
    if(pthread_cond_broadcast(&tqueue->full) != 0)
        return -1;
    if(stop_callback_queue(tqueue->cblist) !=0)
        return -1;
    if(pthread_join(tqueue->scheduler, NULL) !=0)
        return -1;
    return 0;
}

void timer_queue_destroy(struct timer_queue* tqueue)
{
    if(tqueue){
        pthread_mutex_lock(&tqueue->lock);
        hash_destroy(tqueue->map);
        free(tqueue->timers);
        destroy_callback_queue(tqueue->cblist);
        pthread_mutex_unlock(&tqueue->lock);
        pthread_cond_destroy(&tqueue->cond);
        pthread_cond_destroy(&tqueue->empty);
        pthread_cond_destroy(&tqueue->full);
        free(tqueue);
    }
}

struct timer_entry* new_timer(struct timer_queue* tq)
{
    /* alloc a timer from pool */
    struct timer_entry *timer = NULL;
    timer = (struct timer_entry*)mempool_alloc(
        tq->tpool,sizeof(struct timer_entry));
    if(timer != NULL){
        memset(timer,0,sizeof(struct timer_entry));
        timer->status = TIMER_INIT;
        timer->flags = TIMER_FIRSTRUN;
        timer->heap_id = -1;
    }
    return timer;
}

inline
void free_timer(struct timer_queue* tq,struct timer_entry *timer)
{
    /* if set free fn,release param*/
    if(timer->free){
        /*check repeat over,free flag=1*/
        if(timer->flags & TIMER_STOP)
            timer->free(timer->param,1);
        else if(timer->flags & TIMER_DELAY)
            timer->free(timer->param,2);
        else
            timer->free(timer->param,0);
    }
    /* release timer ptr to pool */
    mempool_free(tq->tpool,timer);
}

unsigned int add_timer(struct timer_queue* tq,struct timeval* expire,
                       unsigned int flags,timer_callback_func func, void *param)
{
    unsigned int tid;
    struct timer_entry *timer = NULL;
    timer = malloc(sizeof(struct timer_entry));
    if(timer != NULL){
        memset(timer,0,sizeof(struct timer_entry));
        timer->setup.tv_sec = expire->tv_sec;
        timer->setup.tv_usec = expire->tv_usec;
        timer->flags = flags;
        timer->heap_id = -1;
        timer->param = param;
        timer->callback = func;

        pthread_mutex_lock(&tq->lock);
        while (tq->count >= tq->max){
            tq->full_waiters++;
            pthread_cond_wait(&tq->empty, &tq->lock);
            tq->full_waiters--;
        }
        /*use the 16bit timer id*/
        if(timer->flags & TIMER_USE16BID){
            if(tq->ids > 65535)
                tq->ids = 0;
        }
        /*not generate timer id*/
        if(tq->ids == 0){
            tq->ids++;
        }
        tid = tq->ids++;
        timer->timer_id = tid;
        if(insert_timer(tq,timer) == 0){
            hash_set(tq->map, timer->timer_id, (void*)timer);
            timer->status = TIMER_WATTING;
            //tq->count++;
            if (tq->empty_waiters)
                pthread_cond_signal(&tq->full);
        }else{
            free_timer(tq,timer);
            tid = 0; /*failure*/
        }
        pthread_mutex_unlock(&tq->lock);
        return tid;
    }
    if(timer != NULL)
        free(timer);
    return 0;
}

unsigned int push_timer(struct timer_queue* tq,struct timer_entry *timer)
{
    int ret;
    unsigned int tid=0;
    if(timer != NULL){
        if(pthread_mutex_lock(&tq->lock) == 0){
            while (tq->count >= tq->max){
                tq->full_waiters++;
                ret = pthread_cond_wait(&tq->empty, &tq->lock);
                tq->full_waiters--;
                if (tq->running == 0)
                    goto exit; /* exit cond waiting*/
                if(ret != 0)
                    goto exit; /* condition wait error*/
            }
            if(!(timer->flags & TIMER_CUSTOM)){
                if(tq->ids == 0)
                    tq->ids++;
                tid = tq->ids++;
                timer->timer_id = tid;
            }
            timer->flags |= TIMER_FIRSTRUN;
            if(insert_timer(tq,timer) == 0){
                hash_set(tq->map,timer->timer_id,(void*)timer);
                timer->status = TIMER_WATTING;
                //tq->count++;
                if (tq->empty_waiters)
                    pthread_cond_signal(&tq->full);
            }else{
                tid = 0; /*failure*/
                free_timer(tq,timer);
            }
exit:
            pthread_mutex_unlock(&tq->lock);}
        return tid;
    }
    return 0;
}

int repeat_timer(struct timer_queue* tq,struct timer_entry *timer)
{
    int ret = -1;

    /* check timer whether already deleted*/
    pthread_mutex_lock(&tq->lock);
    if(hash_exists(tq->map,timer->timer_id)){
        while (tq->count >= tq->max){
            tq->full_waiters++;
            ret = pthread_cond_wait(&tq->empty, &tq->lock);
            tq->full_waiters--;
            if (tq->running == 0)
                goto exit; /* exit cond waiting*/
            if(ret != 0)
                goto exit; /* condition wait error*/
        }
        if(insert_timer(tq,timer) == 0){
            timer->status = TIMER_WATTING; /* in schedule list*/
            //tq->count++;
            if (tq->empty_waiters)
                pthread_cond_signal(&tq->full);
            ret = 0; /*repeat successfully*/
        }
    }
exit:
    pthread_mutex_unlock(&tq->lock);
    return ret;
}

/*
* If the user have set the timer start at a time with flag
* TIMER_STARTAT,function will skip set expire time.
*/
int insert_timer(struct timer_queue* tq,struct timer_entry *timer)
{
    int ret = 0;
    struct timer_entry *top;

    /*if indicated start time,skip make expire time*/
    if(timer->flags & TIMER_STARTAT){
        timer->flags &= ~TIMER_STARTAT;
    }else{
        /*calculate expire time*/
        gettimeofday(&timer->expire,NULL);
        /*record startup time*/
        if(timer->flags & TIMER_FIRSTRUN){
            timer->start = timer->expire;
            timer->flags &= ~TIMER_FIRSTRUN;
        }else{
            if(timer->flags & TIMER_DELAYINC)
                timer->setup.tv_usec <<= 1;
        }
        tv_add(&timer->expire,&timer->setup);
    }
    /* if a new early timer before scheduling*/
    if(tq->count){
        top = heap_top(tq); /*interrupt,notify*/
        if(top->status == TIMER_SCHEDULING){
            if(tv_lt(&top->expire,&timer->expire)){
                tq->state = TIMER_INTERRUPT;
                pthread_cond_signal(&tq->cond);
                /*a early timer*/
            }
        }
    }
    /* insert timer to heap*/
    heap_elem_init(timer);
    heap_push(tq,timer);
    return ret;
}

inline struct timer_entry* first_timer(struct timer_queue* tq)
{
    struct timer_entry* timer = NULL;
    if(tq->count){
        timer = heap_pop(tq);
    }
    return timer;
}

void* timer_schedule(void* param)
{
    int ret = 0;
    int status;
    struct timeval now;
    struct timer_entry *timer;
    struct timespec timeout;
    struct timer_queue *tqueue = (struct timer_queue*)param;
    unsigned int timer_id;
    //LOG("TIMER: Schedule thread start\n");

    while (tqueue->running){
        pthread_mutex_lock(&tqueue->lock);
        while (tqueue->count<=0){ /*waiting for new timer*/
            tqueue->empty_waiters++;
            ret = pthread_cond_wait(&tqueue->full, &tqueue->lock);
            tqueue->empty_waiters--;
            if (tqueue->running == 0){
                goto exit; /*exit*/
            }
            if(ret != 0){
                /* Condition wait error*/
                DBG("Timer: Schedule condition wait error %d\n",ret);
                goto exit;
            }
        }

        if(ret == 0){ /*new timer in*/
            do{
                /*normal,interrupted,canceled*/
                tqueue->state = TIMER_SCHEDULING;
                timer = heap_pop(tqueue);
                if (timer == NULL){
                    DBG("Timer: Heap empty!\n");
                    goto exit;
                }
                timeout.tv_sec = timer->expire.tv_sec;
                timeout.tv_nsec = timer->expire.tv_usec * 1000;
                while(timeout.tv_nsec >= 1000000000){
                    timeout.tv_nsec -= 1000000000;
                    timeout.tv_sec += 1;
                }

                timer_id = timer->timer_id;
                timer->status = TIMER_SCHEDULING;
                status = pthread_cond_timedwait(
                    &tqueue->cond, &tqueue->lock, &timeout);
                if(tqueue->running == 0)
                    goto exit;

                /* new early timer in,pop top again*/
            }while(tqueue->state == TIMER_INTERRUPT);

            if((timer = hash_get(tqueue->map,timer_id))){
                if(status == ETIMEDOUT){
                    /*timer's condition wait timed expire */
                    timer->status = TIMER_EXPIRED;
                    gettimeofday(&now,NULL);

                    /*log out expire time for test
                    DBG("Timer: Expire %d,now %d,%d,ex %d,%d,q %d\n",
                        timer_id,now.tv_sec,now.tv_usec,
                        timer->expire.tv_sec,timer->expire.tv_usec,
                        tqueue->count);
                    */
                }
                else if(status != 0){
                    timer->status = TIMER_ERROR;
                    /* condition wait raise error!*/
                    DBG("TIMER: Timer schedule condition error %d,tv %d,%d.\n",
                        status,timeout.tv_sec,timeout.tv_nsec);
                    break;
                }
                else if (status == 0){ /*timer interrupted,canceled*/
                    timer->status = TIMER_CANCLED;
                    DBG("Timer: Canceled %d\n",timer_id);
                }
                /* remove from schedule,into cb*/
                remove_timer(tqueue,timer);
            }
        }
exit:
        pthread_mutex_unlock(&tqueue->lock);
    }

    //LOG("TIMER: Schedule thread exit\n");
    return NULL;
}

/*
* If the timer still need repeat,keep the tid map till
* timer deleted.
*/
int remove_timer(struct timer_queue* tqueue,struct timer_entry *timer)
{
    int result = 0;

    /* if continue repeat,keep map*/
    if(!(timer->flags & TIMER_REPEAT)) {
        hash_remove(tqueue->map, timer->timer_id);
    }

    /* erase the timer from heap*/
    heap_erase(tqueue,timer);

    /* status already locked in scheduler,now entrance cb status*/
    timer->status = TIMER_CBWAITING;
    callback_queue_put(tqueue->cblist,timer);
    if (tqueue->full_waiters) {
        pthread_cond_signal(&tqueue->empty);
    }
    return result;
}


/*
* If the timer flag is TIMER_USEC,del_timer return the
* timer time-consuming value.del_timer charge timer mem
* free outsite of callback.
*/
long del_timer(struct timer_queue* tqueue,unsigned int tid,int flag)
{
    long result = -1;
    struct timer_entry *timer;

    if(pthread_mutex_lock(&tqueue->lock) == 0){
        timer = (struct timer_entry*)hash_remove(tqueue->map,tid);
        if(timer != NULL){
            /*DBG("Timer:del %d\n",tid);*/            
            result = 0; /*remove ok*/
            /* deliver timer free flag*/
            if(flag == TIMER_DELAY)
                timer->flags |= TIMER_DELAY;
            if(timer->flags & TIMER_USEC){
                struct timeval now;
                gettimeofday(&now,NULL);
                tv_sub(&now,&timer->start);
                /*return spend usec*/
                if((now.tv_sec == 0) && now.tv_usec){
                    result = now.tv_usec; 
                }else{
                    DBG("TIMER: timer %d over 1 sec.\n",tid);                    
                }
            }

            /*if in callback status,free by cb*/
            if(timer->status >= TIMER_WATTING &&
                timer->status < TIMER_CBWAITING)
            {
                if(timer->status == TIMER_SCHEDULING)
                    pthread_cond_signal(&tqueue->cond);
                if (tqueue->full_waiters)
                    pthread_cond_signal(&tqueue->empty);
                heap_erase(tqueue,timer);
                free_timer(tqueue,timer);
            }
        }else{
            DBG("TIMER: timer %d already acked.\n",tid);
        }
        pthread_mutex_unlock(&tqueue->lock);
    }
    return result;
}

/*
* The timer cancel function is used for repeat timer.
* We can use it in the inner of callback function to
* detach timer id map.
*/
int cancel_timer(struct timer_queue* tqueue,unsigned int tid)
{
    int result = -1;
    struct timer_entry *timer;
    if(pthread_mutex_lock(&tqueue->lock) == 0){
        timer = (struct timer_entry*)hash_get(tqueue->map,tid);
        if(timer != NULL){
            if(timer->status == TIMER_SCHEDULING)
                pthread_cond_signal(&tqueue->cond);
            hash_remove(tqueue->map, tid);
            result = 0;
        }
        pthread_mutex_unlock(&tqueue->lock);
    }
    return result;
}

void del_all_timer(struct timer_queue* tqueue)
{
    unsigned int i;
    struct timer_entry *timer;
    if(pthread_mutex_lock(&tqueue->lock) == 0){
        for(i=0;i<tqueue->size;i++){
            if(tqueue->timers[i]){
                if(timer->status == TIMER_SCHEDULING)
                    pthread_cond_signal(&tqueue->cond);
                hash_remove(tqueue->map, timer->timer_id);
                free_timer(tqueue,timer);
            }
        }
        free(tqueue->timers);
        pthread_mutex_unlock(&tqueue->lock);
    }
}

/*
* Provide two way to control the timer repeat,one is the return
* value of call back function can be used to indicate whether
* continue the timer,the other way is use timer flag.
* Timer in callback status range,cb charge timer memory free.
*/
int timer_fncall(struct timer_queue* tqueue,struct timer_entry* timer)
{
    int ret = 0;
    if(timer->callback){
        ret = timer->callback(timer->timer_id,timer->param);
        /*controlled by callback ret*/
        if(ret == TIMER_REPEAT){
            if(repeat_timer(tqueue,timer) == 0)
                return ret;/*continue repeat*/
        }
    }else{
        DBG("TIMER: Timer call back function null.\n");
    }

    /*controlled by timer flag*/
    if(timer->flags & TIMER_REPEAT){
        if(--timer->repeat){
            /* repeat will check map first*/
            if(repeat_timer(tqueue,timer) == 0)
                return ret; /*continue repeat*/
        }else{
            /* timer have been removed from heap, but 
            * map maybe still in, here sure del from map,
            * though may removed del_timer,we do again.
            * the timer in call range status,free by cb.
            */
            WARN("Timer: Repeat tid %d over\n",timer->timer_id);
            timer->flags |= TIMER_STOP;
            pthread_mutex_lock(&tqueue->lock);
            hash_remove(tqueue->map, timer->timer_id);
            pthread_mutex_unlock(&tqueue->lock);
        }
    }

    /*non repeat,or repeat end,free it*/
    free_timer(tqueue,timer);
    return ret;
}

void* timer_callback(void* param)
{
    struct timer_entry* timer;
    struct callback_queue *cqueue;
    struct timer_queue* tqueue;

    cqueue = (struct callback_queue*)param;
    tqueue = cqueue->owner;

    //LOG("TIMER: Callback thread start\n");

    while (cqueue->running && tqueue->running){
        timer =  callback_queue_get(tqueue->cblist);
        if(timer){
            timer_fncall(tqueue,timer);
        }
    }

    //LOG("TIMER: Callback thread exit\n");
    return NULL;
}

