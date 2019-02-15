/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * bytes stream interface
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 *
 * $Id: stream.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */
 
#ifndef _IO_STREAM_
#define _IO_STREAM_

#include "buffer.h"

#define Endian              1
#define SIZEOF_BOOL         1
#define SIZEOF_LONG_DOUBLE  12

enum stream_type
{
	X86_STREAM,
	CDR_STREAM,
	XDR_STREAM
};	

struct stream
{
	struct Buffer* buf;
	char* orig_cur;
	bool orig_swap;
	size_t align_next;
	int type;
	bool swap;
};

/* stream read*/
uchar read_char(stream *is);
uchar read_octet(stream *s);
bool read_boolean(stream* s);
int16 read_short(stream *is);
uint16 read_ushort(stream *is);
int32 read_long(stream *is);
uint32 read_ulong(stream *is);
int64 read_longlong(stream *is);
uint64 read_ulonglong(stream *is);
float read_float(stream *is);
double read_double(stream *is);
long double read_longdouble(stream *is);

/* stream array read*/
char* read_string(stream *is);
void read_char_array(stream *is,uchar* value, size_t);
void read_boolean_array(stream *is,bool*, size_t);
void read_uchar_array(stream *is,uchar*, size_t);
void read_short_array(stream *is,int16*, size_t );
void read_ushort_array(stream *is,uint16*, size_t );
void read_long_array(stream *is,int32*, size_t );
void read_ulong_array(stream *is,uint32*, size_t );
void read_longlong_array(stream *is,int64*, size_t );
void read_ulonglong_array(stream *is,uint64*, size_t );
void read_float_array(stream *is,float*, size_t );
void read_double_array(stream *is,double*, size_t );
void read_longdouble_array(stream *is,long double*, size_t);


/* stream write*/
void write_char(stream *os,char value)
void write_byte(stream *os,char value)
void write_boolean(stream *os,bool value)
void write_octet(stream *os,unsigned char value)
void write_short(stream *os,int16 value)
void write_ushort(stream *os,uint16 value)
void write_long(stream *os,int32 value)
void write_ulong(stream *os,uint32 value)
void write_longlong(stream *os,int64 value)
void write_ulonglong(stream *os,uint64 value)
void write_float(stream *os,float value)
void write_double(stream *os,double value)
void write_longdouble(stream *os,long double value);

/* stream array write*/
void write_string(stream *os,const char*);
void write_boolean_array(stream *os,const bool*, size_t);
void write_char_array(stream *os,const char*, size_t);
void write_short_array(stream *os,const int16*, size_t);
void write_ushort_array(stream *os,const uint16*, size_t);
void write_long_array(stream *os,const int32*, size_t);
void write_ulong_array(stream *os,const uint32*, size_t);
void write_longlong_array(stream *os,const int64*, size_t);
void write_ulonglong_array(stream *os,const uint64*, size_t);
void write_float_array(stream *os,const float*, size_t);
void write_double_array(stream *os,const double*, size_t);
void write_longdouble_array(stream *os,const long double*, size_t);
	
#endif // _IO_STREAM_
