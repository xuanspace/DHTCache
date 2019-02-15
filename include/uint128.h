/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Version 1.0
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: version.h 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#ifndef _UINT128_H_
#define _UINT128_H_

struct uint128
{
	unsigned long data[4];
};
typedef struct uint128 uint128;

struct uint128_ops 
{
	size_t (*get_bits)(uint128 *value,size_t bit);
	void (*set_bits)(uint128 *dst,size_t bit,unsigned int value);
	void (*init)(uint128 *data,uint128 *value, size_t bits);
	void (*set_value)(uint128 *dst,uint128 *src);
	void (*set_ulong)(uint128 *dst,unsigned long value);
	void (*set_bytescpy)(uint128 *dst,byte *value);
	void (*set_bytes)(uint128 *dst,byte *value);
	void (*set_random)(uint128 *value);	
	void (*xor)(uint128 *dst,uint128 *value);
	void (*xor_bytes)(uint128 *dst,byte *value);
	void (*to_bytes)(uint128 *value,byte *b);
	int (*compare)(uint128 *dst,uint128 *src);
	int (*compare_ulong)(uint128 *dst,unsigned long value);
	void (*add)(uint128 *dst,uint128 *value);
	void (*add_ulong)(uint128 *dst,unsigned long value);
	void (*subtract)(uint128 *dst,uint128 *value);
	void (*subtract_ulong)(unsigned long value);
	void (*shift_left)(uint128 *dst,size_t bits);
};

#endif //_UINT128_H_
