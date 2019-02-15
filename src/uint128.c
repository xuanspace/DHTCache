/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Unsigned int 128
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: uint128.h 4655 2009-07-27 11:25:59Z wxlin $
 *  big endian = BE
 */

size_t get_bits(uint128 *value,size_t bit)
{
	if (bit > 127)
		return 0;
	int n = bit / 32;
	int shift = 31 - (bit % 32);
	return ((value[n] >> shift) & 1);
}

void set_value(uint128 *dst,uint128 *src)
{
	dst->data[0] = src->data[0];
	dst->data[1] = src->data[1];
	dst->data[2] = src->data[2];
	dst->data[3] = src->data[3];
}

void set_ulong(uint128 *dst,unsigned long value)
{
	dst->data[0] = 0;
	dst->data[1] = 0;
	dst->data[2] = 0;
	dst->data[3] = value;
}

void set_bytescpy(uint128 *dst,byte *value)
{
	memcpy((byte*)dst->data, value, 16);
}

void set_bytes(uint128 *dst,byte *value)
{
	int i;
	memset(dst,0,sizeof(uint128));
	for (i=0; i<16; i++)
		dst->data[i/4] |= ((unsigned long)value[i]) << (8*(3-(i%4)));
}

void set_random(uint128 *value)
{
	for (int i=0; i<16; i++)
		value->data[i/4] ^= rand() << (8*(3-(i%4)));
	return *this;
}

void set_bits(uint128 *dst,size_t bit,unsigned int value) 
{
	int n = bit/32;
	int shift = 31 - (bit % 32);
	dst->data[n] |= (1 << shift);
	if (value == 0)
		dst->data[n] ^= (1 << shift);
}

void xor(uint128 *dst,uint128 *value)
{
	for (int i=0; i<4; i++)
		dst->data[i] ^= value->data[i];
}

void xor_bytes(uint128 *dst,byte *value)
{
	uint128 temp;
	set_bytes(temp,value);
	xor(dst,temp);
}

void to_bytes(uint128 *value,byte *b)
{
	int i;
	for (i=0; i<16; i++)
		b[i] = (byte)(value->data[i/4] >> (8*(3-(i%4))));
}

int compare(uint128 *dst,uint128 *src)
{	
	int i;
	for (i=0; i<4; i++) {
	    if (dst->data[i] < src->data[i])
			return -1;
	    if (dst->data[i] > src->data[i])
			return 1;
	}
	return 0;
}

int compare_ulong(uint128 *dst,unsigned long value) 
{
	if ((dst->data[0] > 0) || (dst->data[1] > 0) || (dst->data[2] > 0) || (dst->data[3] > value))
		return 1;
	if (dst->data[3] < value)
		return -1;
	return 0;
}

void add(uint128 *dst,uint128 *value)
{
	if (value == 0)
		return;
	int64_t sum = 0;
	for (int i=3; i>=0; i--){
		sum += dst->data[i];
		sum += value->data[i];
		dst->data[i] = (unsigned long)sum;
		sum = sum >> 32;
	}
}

void add_ulong(uint128 *dst,unsigned long value)
{
	if (value == 0)
		return;
	uint128 temp;
	set_ulong(temp,value);
	add(dst,temp);
}

void subtract(uint128 *dst,uint128 *value)
{
	if (value == 0)
		return;
	int64_t sum = 0;
	for (int i=3; i>=0; i--){
		sum += dst->data[i];
		sum -= value->data[i];
		dst->data[i] = (unsigned long)sum;
		sum = sum >> 32;
	}
	return *this;
}

void subtract_ulong(unsigned long value)
{
	if (value == 0)
		return;
	uint128 temp;
	set_ulong(temp,value);
	subtract(dst,temp);
}

void shift_left(uint128 *dst,size_t bits)
{
    if ((bits == 0) || (compare_ulong(0) == 0))
        return;
		
	if (bits > 127){
		set_ulong(0);
		return;
	}

	unsigned long result[] = {0,0,0,0};
	int index_shift = (int)bits / 32;
	int64_t shifted = 0;
	for (int i=3; i>=index_shift; i--)
	{
		shifted += ((__int64)dst->data[i]) << (bits % 32);
		result[i-index_shift] = (unsigned long)shifted;
		shifted = shifted >> 32;
	}
	for (int i=0; i<4; i++)
		dst->data[i] = result[i];
}

struct __uint128_ops uint128 = 
{
	get_bits,
	set_bits,
	init,
	set_value,
	set_ulong,
	set_bytescpy,
	set_bytes,
	set_random,	
	xor,
	xor_bytes,
	to_bytes,
	compare,
	compare_ulong,
	add,
	add_ulong,
	subtract,
	subtract_ulong,
	shift_left,
};
