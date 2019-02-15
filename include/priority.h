/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * priority define
 *
 * wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: priority.h,v 1.3 2008-12-24 04:07:19 wxlin Exp $
 */
 
#ifndef _PRIORITY_H_
#define _PRIORITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PRIORITY_LEVEL	6

enum PRIORITY_CALSS{
    PRIORITY_IDLE,
    PRIORITY_DELAY,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_HIGHEST
};

#ifdef __cplusplus
}
#endif

#endif // _PRIORITY_H_
