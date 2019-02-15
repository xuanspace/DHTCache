/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* Timeval utilities functions
*
* Author(s): wxlin  <weixuan.lin@sierraatlantic.com>
*
* $Id:timeval.h,v 1.4 2008-12-30 08:14:55 wxlin Exp $
*
*/

#ifndef _TIMEVAL_H_
#define _TIMEVAL_H_

#include <time.h>
#include <sys/time.h>
#include "datatypes.h"

/*typedef enum {false, true} bool;*/

static inline bool tv_nz (struct timeval *tv);
static inline void tv_clear (struct timeval *tv);
static inline void tv_set (struct timeval *tv, long sec, long usec);
static inline void tv_setv (struct timeval *tv, struct timeval *tv2);
static inline void tv_add (struct timeval *dest, const struct timeval *src);
static inline void tv_add_usec (struct timeval *dest, long usec);
static inline void tv_sub(struct timeval *dest, const struct timeval *src);
static inline void tv_div(struct timeval *tv, struct timeval *a, int n);
static inline void tv_mul(struct timeval *tv, struct timeval *a, int n);
static inline bool tv_lt ( struct timeval *t1,  struct timeval *t2);
static inline bool tv_le (const struct timeval *t1, const struct timeval *t2);
static inline bool tv_ge (const struct timeval *t1, const struct timeval *t2);
static inline bool tv_gt (const struct timeval *t1, const struct timeval *t2);
static inline bool tv_eq (const struct timeval *t1, const struct timeval *t2);
static inline void tv_delta (struct timeval *dest, const struct timeval *t1, const struct timeval *t2);
static inline void ts_clear (struct timespec *ts);
static inline bool ts_nz (struct timespec *ts);
static inline void ts_set (struct timespec *ts, long sec, long nsec);
static inline void ts_add(struct timespec *dest, struct timespec *src);
static inline void ts_add_nsec(struct timespec *dest, long nsec);
static inline void ts_add_usec(struct timespec *dest, long usec);
static inline void ts_sub(struct timespec *dest, struct timespec *src);
static inline bool ts_lt (const struct timespec *t1, const struct timespec *t2);
static inline bool ts_le (const struct timespec *t1, const struct timespec *t2);
static inline bool ts_ge (const struct timespec *t1, const struct timespec *t2);
static inline bool ts_gt (const struct timespec *t1, const struct timespec *t2);
static inline bool ts_eq (const struct timespec *t1, const struct timespec *t2);
static inline void time_tots (struct timespec *ts, time_t t);
static inline void ts_totime (time_t t,struct timespec *ts, int prec);

static inline bool
tv_nz (struct timeval *tv)
{
    return tv->tv_sec || tv->tv_usec;
}

static inline void
tv_clear (struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}

static inline void
tv_set (struct timeval *tv, long sec, long usec)
{
    tv->tv_sec = sec;
    tv->tv_usec = usec;
}

static inline void
tv_setv (struct timeval *tv, struct timeval *tv2)
{
    tv->tv_sec = tv2->tv_sec;
    tv->tv_usec = tv2->tv_usec;
}

static inline void
tv_add (struct timeval *dest, const struct timeval *src)
{
    dest->tv_sec += src->tv_sec;
    dest->tv_usec += src->tv_usec;
    while (dest->tv_usec >= 1000000)
    {
        dest->tv_usec -= 1000000;
        dest->tv_sec += 1;
    }
}

static inline void
tv_add_usec (struct timeval *dest, long usec)
{
    dest->tv_usec += usec;
    while (dest->tv_usec >= 1000000)
    {
        dest->tv_usec -= 1000000;
        dest->tv_sec += 1;
    }
}

static inline void
tv_sub(struct timeval *dest, const struct timeval *src)
{
    dest->tv_sec = dest->tv_sec - src->tv_sec;
    dest->tv_usec = dest->tv_usec - src->tv_usec;
    if (dest->tv_usec < 0) {
        dest->tv_sec--;
        dest->tv_usec += 1000000;
    }
}

static inline void
tv_div(struct timeval *tv, struct timeval *a, int n)
{
    tv->tv_usec = (a->tv_sec % n * 1000000 + a->tv_usec + n / 2) / n;
    tv->tv_sec = a->tv_sec / n + tv->tv_usec / 1000000;
    tv->tv_usec %= 1000000;
}

static inline void
tv_mul(struct timeval *tv, struct timeval *a, int n)
{
    tv->tv_usec = a->tv_usec * n;
    tv->tv_sec = a->tv_sec * n + a->tv_usec / 1000000;
    tv->tv_usec %= 1000000;
}

static inline bool
tv_lt ( struct timeval *t1,  struct timeval *t2)
{
    if (t1->tv_sec < t2->tv_sec)
        return true;
    else if (t1->tv_sec > t2->tv_sec)
        return false;
    else
        return t1->tv_usec < t2->tv_usec;
}

static inline bool
tv_le (const struct timeval *t1, const struct timeval *t2)
{
    if (t1->tv_sec < t2->tv_sec)
        return true;
    else if (t1->tv_sec > t2->tv_sec)
        return false;
    else
        return t1->tv_usec <= t2->tv_usec;
}

static inline bool
tv_ge (const struct timeval *t1, const struct timeval *t2)
{
    if (t1->tv_sec > t2->tv_sec)
        return true;
    else if (t1->tv_sec < t2->tv_sec)
        return false;
    else
        return t1->tv_usec >= t2->tv_usec;
}

static inline bool
tv_gt (const struct timeval *t1, const struct timeval *t2)
{
    if (t1->tv_sec > t2->tv_sec)
        return true;
    else if (t1->tv_sec < t2->tv_sec)
        return false;
    else
        return t1->tv_usec > t2->tv_usec;
}

static inline bool
tv_eq (const struct timeval *t1, const struct timeval *t2)
{
    return t1->tv_sec == t2->tv_sec && t1->tv_usec == t2->tv_usec;
}

static inline void
tv_delta (struct timeval *dest, const struct timeval *t1, const struct timeval *t2)
{
    int sec = t2->tv_sec - t1->tv_sec;
    int usec = t2->tv_usec - t1->tv_usec;

    while (usec < 0)
    {
        usec += 1000000;
        sec -= 1;
    }

    if (sec < 0)
        usec = sec = 0;

    dest->tv_sec = sec;
    dest->tv_usec = usec;
}

/* Operations on timespecs */
static inline void
ts_clear (struct timespec *ts)
{
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
}

static inline bool
ts_nz (struct timespec *ts)
{
    return ts->tv_sec || ts->tv_nsec;
}

static inline void
ts_set (struct timespec *ts, long sec, long nsec)
{
    ts->tv_sec = sec;
    ts->tv_nsec = nsec;
}

static inline void
ts_add(struct timespec *dest, struct timespec *src)
{
    dest->tv_sec += src->tv_sec;
    dest->tv_nsec += src->tv_nsec;
    while (dest->tv_nsec >= 1000000000) {
        dest->tv_sec++;
        dest->tv_nsec -= 1000000000;
    }
}

static inline void
ts_add_nsec(struct timespec *dest, long nsec)
{
    dest->tv_nsec += nsec;
    while (dest->tv_nsec >= 1000000000) {
        dest->tv_sec++;
        dest->tv_nsec -= 1000000000;
    }
}

static inline void
ts_add_usec(struct timespec *dest, long usec)
{
    dest->tv_nsec += usec*1000;
    while (dest->tv_nsec >= 1000000000) {
        dest->tv_sec++;
        dest->tv_nsec -= 1000000000;
    }
}

static inline void
ts_sub(struct timespec *dest, struct timespec *src)
{
    dest->tv_sec -= src->tv_sec;
    dest->tv_nsec -= src->tv_nsec;
    if (dest->tv_nsec < 0) {
        dest->tv_sec--;
        dest->tv_nsec += 1000000000;
    }
}

static inline bool
ts_lt (const struct timespec *t1, const struct timespec *t2)
{
    if (t1->tv_sec < t2->tv_sec)
        return true;
    else if (t1->tv_sec > t2->tv_sec)
        return false;
    else
        return t1->tv_nsec < t2->tv_nsec;
}

static inline bool
ts_le (const struct timespec *t1, const struct timespec *t2)
{
    if (t1->tv_sec < t2->tv_sec)
        return true;
    else if (t1->tv_sec > t2->tv_sec)
        return false;
    else
        return t1->tv_nsec <= t2->tv_nsec;
}

static inline bool
ts_ge (const struct timespec *t1, const struct timespec *t2)
{
    if (t1->tv_sec > t2->tv_sec)
        return true;
    else if (t1->tv_sec < t2->tv_sec)
        return false;
    else
        return t1->tv_nsec >= t2->tv_nsec;
}

static inline bool
ts_gt (const struct timespec *t1, const struct timespec *t2)
{
    if (t1->tv_sec > t2->tv_sec)
        return true;
    else if (t1->tv_sec < t2->tv_sec)
        return false;
    else
        return t1->tv_nsec > t2->tv_nsec;
}

static inline bool
ts_eq (const struct timespec *t1, const struct timespec *t2)
{
    return t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec;
}

static inline void
time_tots (struct timespec *ts, time_t t)
{
    ts->tv_sec = (time_t)((t) / 1000000);
    ts->tv_nsec = (long)(((t) % 1000000) * 1000);
}

static inline void
ts_totime (time_t t,struct timespec *ts, int prec)
{
    t = (u_long)(ts->tv_sec * 1000000);
    t += (u_long)(ts->tv_nsec / 1000);
    /* Add in 1 usec for lost nsec precision if wanted. */
    if (prec)
        t++;
}

#endif /*_TIMEVAL_H_*/
