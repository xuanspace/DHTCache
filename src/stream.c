/*
 * Kad Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * DHT Bytes Stream
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: stream.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */
 
#include "stream.h"

void stream_reset(struct stream* s)
{
	s->buf->ptr = 0;
	s->swap = 0;
}

int stream_skip(struct stream* s,size_t length)
{
	register char* cur = s->buf->ptr + length;
	if(cur < s->buf->ptr || cur > s->buf->end)
		return -1;

	s->buf->ptr = cur;
    return 0;
}

void check_chunk(stream *s)
{
}

void stream_read_endian(stream *s)
{
	s->swap = (read_boolean(s) != Endian);
}

bool read_boolean(stream* s)
{
	if(s->buf->ptr > s->buf->end)
		DBG("Stream: read boolean\n");
	
	bool val = (*(s->buf ->ptr)++) ? true : false;
	return val;
}

uchar read_octet(stream *s)
{
	if(s->buf->ptr > s->buf->end)
        DBG("Stream: read octet\n");

	uchar val = *(s->buf->ptr)++;
	return val;
}

uchar read_char(stream *s)
{
    check_chunk(s);

    if(s->buf->ptr > s->buf->end)
        DBG("Stream: read char overflow\n");

    return *(s->buf->ptr++);
}

int16 read_short(stream *s)
{
    int16 val;
    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 2;
    if(next > s->buf->end)
        DBG("Stream: read short overflow\n");

    val = *(const int16*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint16& v = (uint16&)val;
        v = (v << 8) | (v >> 8);
    }

    return val;
}

uint16 read_ushort(stream *s)
{
    uint16 val;
    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 2;
    if(next > s->buf->end)
        DBG("Stream: read unsigned short overflow\n");

    val = *(const uint16*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        val = (val << 8) | (val >> 8);
	}
	
    return val;
}

int32 read_long(stream *s)
{
	int32 val;
    check_chunk();

    register char* cur = s->buf->ptr;
    register char* next = cur + 4;
    if(next > s->buf->end)
        DBG("Stream: read long overflow\n");

    val = *(const int32*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint32& v = (uint32&)val;
        v = ((v << 24) | ((v & 0xff00) << 8)
            | ((v >> 8) & 0xff00) | (v >> 24));
    }

    return val;
}

uint32 read_ulong(stream *s)
{
    uint32 val;

    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 4;
    if(next > s->buf->end)
        DBG("Stream: read unsigned long overflow\n");

    val = *(const uint32*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        val = ((val << 24) | ((val & 0xff00) << 8)
        | ((val >> 8) & 0xff00) | (val >> 24));
    }

    return val;
}

int64 read_longlong(stream *s)
{
    int64 val;

    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 8;
    if(next > s->buf->end)
        DBG("Stream: read long long overflow\n");

    val = *(const int64*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint64& v = (uint64&)val;
        v = ((v << 56) | ((v & 0xff00) << 40)
            | ((v & 0xff0000) << 24) | ((v & 0xff000000) << 8)
            | ((v >> 8) & 0xff000000) | ((v >> 24) & 0xff0000)
            | ((v >> 40) & 0xff00) | (v >> 56));
    }

    return val;
}

uint64 read_ulonglong(stream *s)
{
    uint64 val;

    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 8;
    if(next > s->buf->end)
        DBG("Stream: read ulong long overflow\n");

    val = *(const uint64*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        val = ((val << 56) | ((val & 0xff00) << 40)
        | ((val & 0xff0000) << 24) | ((val & 0xff000000) << 8)
        | ((val >> 8) & 0xff000000) | ((val >> 24) & 0xff0000)
        | ((val >> 40) & 0xff00) | (val >> 56));
    }

    return val;
}

float read_float(stream *s)
{
    float val;

    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 4;
    if(next > s->buf->end)
        DBG("Stream: read float overflow\n");

    val = *(const float*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint32* v = (uint32*)&val;
        *v = ((*v << 24) | ((*v & 0xff00) << 8)
            | ((*v >> 8) & 0xff00) | (*v >> 24));
    }

    return val;
}

double read_double(stream *s)
{
    double val;

    check_chunk(s);

    register char* cur = s->buf->ptr;
    register char* next = cur + 8;
    if(next > s->buf->end)
        DBG("Stream: read double over flow\n");

    val = *(const double*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint32 v0 = ((uint32*)&val)[0];
        uint32 v1 = ((uint32*)&val)[1];
        v0 = ((v0 << 24) | ((v0 & 0xff00) << 8)
            | ((v0 >> 8) & 0xff00) | (v0 >> 24));
        v1 = ((v1 << 24) | ((v1 & 0xff00) << 8)
            | ((v1 >> 8) & 0xff00) | (v1 >> 24));
        ((uint32*)&val)[0] = v1;
        ((uint32*)&val)[1] = v0;
    }

    return val;
}

#if SIZEOF_LONG_DOUBLE < 12

long double
read_longdouble(stream *s)
{
    DBG("Stream: long double not supported\n");
    return long double(); // Some compilers need this
}

#else

long double
read_longdouble(stream *s)
{
    long double val;

    check_chunk();

    register char* cur = s->buf->ptr;
    register char* next = cur + 16;
    if(next > s->buf->end)
        DBG("Stream: read long double overflow\n");

#if SIZEOF_LONG_DOUBLE == 16

    val = *(const long double*)cur;
    s->buf->ptr = next;

    if(s->swap)
    {
        uint64 v0 = ((uint64*)&val)[0];
        uint64 v1 = ((uint64*)&val)[1];
        v0 = ((v0 << 56) | ((v0 & 0xff00) << 40)
            | ((v0 & 0xff0000) << 24) | ((v0 & 0xff000000) << 8)
            | ((v0 >> 8) & 0xff000000) | ((v0 >> 24) & 0xff0000)
            | ((v0 >> 40) & 0xff00) | (v0 >> 56));
        v1 = ((v1 << 56) | ((v1 & 0xff00) << 40)
            | ((v1 & 0xff0000) << 24) | ((v1 & 0xff000000) << 8)
            | ((v1 >> 8) & 0xff000000) | ((v1 >> 24) & 0xff0000)
            | ((v1 >> 40) & 0xff00) | (v1 >> 56));
        ((uint64*)&val)[0] = v1;
        ((uint64*)&val)[1] = v0;
    }

#endif

#if SIZEOF_LONG_DOUBLE == 12

#   ifdef WORDS_BIGENDIAN

#       error 12-bytes long doubles only supported for little endian!

#   else

    uchar tmp[18];

    if(s->swap)
    {
        *(uint16*)&tmp = 0;
        memcpy(tmp + 2, cur, 14);

        uint64 v0 = ((uint64*)&tmp)[0];
        uint64 v1 = ((uint64*)&tmp)[1];
        v0 = ((v0 << 56) | ((v0 & 0xff00) << 40)
            | ((v0 & 0xff0000) << 24) | ((v0 & 0xff000000) << 8)
            | ((v0 >> 8) & 0xff000000) | ((v0 >> 24) & 0xff0000)
            | ((v0 >> 40) & 0xff00) | (v0 >> 56));
        v1 = ((v1 << 56) | ((v1 & 0xff00) << 40)
            | ((v1 & 0xff0000) << 24) | ((v1 & 0xff000000) << 8)
            | ((v1 >> 8) & 0xff000000) | ((v1 >> 24) & 0xff0000)
            | ((v1 >> 40) & 0xff00) | (v1 >> 56));
        ((uint64*)&tmp)[0] = v1;
        ((uint64*)&tmp)[1] = v0;

        *(uint64*)(tmp + 4) >>= 1; // Adjust mantissa
        tmp[11] |= 0x80;           // Set leading significant bit

        val = *(const long double*)(tmp + 4);
    }
    else
    {
        *(uint16*)&tmp[16] = 0;
        memcpy(tmp + 2, cur + 2, 14);

        *(uint64*)(tmp + 6) >>= 1; // Adjust mantissa
        tmp[13] |= 0x80;           // Set leading significant bit

        val = *(const long double*)(tmp + 6);
    }

    s->buf->ptr = next;

#   endif

#endif

    return val;
}

#endif


char* read_string(stream *s)
{
    char* s;

    check_chunk(s);

    uint32 len = read_ulong();
    if(len == 0)
        DBG("Stream: read zero length string\n");

    if(s->buf->ptr + len < s->buf->ptr || s->buf->ptr + len > s->buf->end)
        DBG("Stream: read string overflow\n");

    s = new char[len - 1];

    memcpy(s, s->buf->ptr, len - 1);
    s->buf->ptr += len - 1;

    //
    // Check for terminating null char
    //
    if(*(s->buf->ptr++) != '\0')
    {
        delete [] s;
        DBG("Stream: read string no terminator\n");
    }

    s[len - 1] = '\0';

    if(strlen(s) + 1 != len)
    {
        delete [] s;
        DBG("Stream: read null char string\n");
    }

    return s;
}

void 
read_char_array(stream *s,uchar* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        if(s->buf->ptr + length < s->buf->ptr ||
            s->buf->ptr + length > s->buf->end)
            DBG("Stream: read char array overflow\n");

        memcpy(value, s->buf->ptr, length);
        s->buf->ptr += length;
    }
}

void 
read_boolean_array(stream *s,bool* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* next = s->buf->ptr + length;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read boolean array overflow\n");                                        

        memcpy(value, s->buf->ptr, length);
        s->buf->ptr = next;
    }
}

void 
read_uchar_array(stream *s,uchar* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* next = s->buf->ptr + length;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read octet array overflow\n");                                                        

        memcpy(value, s->buf->ptr, length);
        s->buf->ptr = next;
    }
}

void 
read_short_array(stream *s,int16* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
		size_t arrsize = length * 2;		
        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read shor array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint16* v = (uint16*)value;
            while(length-- > 0)
            {
                *v = (*v << 8) | (*v >> 8);
                v++;
            }
        }
    }
}

void
read_ushort_array(stream *s,uint16* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 2;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read ushort array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            while(length-- > 0)
            {
                *value = (*value << 8) | (*value >> 8);
                value++;
            }
        }
    }
}

void
read_long_array(stream *s,int32* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 4;
        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read long array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint32* v = (uint32*)value;
            while(length-- > 0)
            {
                *v = ((*v << 24) | ((*v & 0xff00) << 8)
                    | ((*v >> 8) & 0xff00) | (*v >> 24));
                v++;
            }
        }
    }
}

void
read_ulong_array(stream *s,uint32* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 4;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read ulong array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            while(length-- > 0)
            {
                *value = ((*value << 24) | ((*value & 0xff00) << 8)
                    | ((*value >> 8) & 0xff00) | (*value >> 24));
                value++;
            }
        }
    }
}

void
read_longlong_array(stream *s,int64* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 8;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read longlong array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint64* v = (uint64*)value;
            while(length-- > 0)
            {
                *v = ((*v << 56) | ((*v & 0xff00) << 40)
                    | ((*v & 0xff0000) << 24) | ((*v & 0xff000000) << 8)
                    | ((*v >> 8) & 0xff000000) | ((*v >> 24) & 0xff0000)
                    | ((*v >> 40) & 0xff00) | (*v >> 56));
                v++;
            }
        }
    }
}

void
read_ulonglong_array(stream *s,uint64* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 8;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read ulonglong array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint64* v = value;
            while(length-- > 0)
            {
                *v = ((*v << 56) | ((*v & 0xff00) << 40)
                    | ((*v & 0xff0000) << 24) | ((*v & 0xff000000) << 8)
                    | ((*v >> 8) & 0xff000000) | ((*v >> 24) & 0xff0000)
                    | ((*v >> 40) & 0xff00) | (*v >> 56));
                v++;
            }
        }
    }
}

void
read_float_array(stream *s,float* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 4;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read float array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint32* v = (uint32*)value;
            while(length-- > 0)
            {
                *v = ((*v << 24) | ((*v & 0xff00) << 8)
                    | ((*v >> 8) & 0xff00) | (*v >> 24));
                v++;
            }
        }
    }
}

void
read_double_array(stream *s,double* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 8;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read double array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint32* v = (uint32*)value;
            while(length-- > 0)
            {
                uint32 v0 = v[0];
                uint32 v1 = v[1];
                v0 = ((v0 << 24) | ((v0 & 0xff00) << 8)
                    | ((v0 >> 8) & 0xff00) | (v0 >> 24));
                v1 = ((v1 << 24) | ((v1 & 0xff00) << 8)
                    | ((v1 >> 8) & 0xff00) | (v1 >> 24));
                v[0] = v1;
                v[1] = v0;
                v += 2;
            }
        }
    }
}

#if SIZEOF_LONG_DOUBLE < 12

void 
read_longdouble_array(stream *s,long double* value, size_t length)
{
    DBG("Stream: long double not supported\n");
}

#else

void
read_longdouble_array(stream *s,long double* value, size_t length)
{
    if(length)
    {
        check_chunk(s);

#if SIZEOF_LONG_DOUBLE == 16

        register char* cur = s->buf->ptr;
        size_t arrsize = length * 16;

        register char* next = cur + arrsize;
        if(next < s->buf->ptr || next > s->buf->end)
            DBG("Stream: read long double array overflow\n");

        memcpy(value, cur, arrsize);
        s->buf->ptr = next;

        if(s->swap)
        {
            uint64* v = (uint64*)value;
            while(length-- > 0)
            {
                uint64 v0 = v[0];
                uint64 v1 = v[1];
                v0 = ((v0 << 56) | ((v0 & 0xff00) << 40)
                    | ((v0 & 0xff0000) << 24) | ((v0 & 0xff000000) << 8)
                    | ((v0 >> 8) & 0xff000000) | ((v0 >> 24) & 0xff0000)
                    | ((v0 >> 40) & 0xff00) | (v0 >> 56));
                v1 = ((v1 << 56) | ((v1 & 0xff00) << 40)
                    | ((v1 & 0xff0000) << 24) | ((v1 & 0xff000000) << 8)
                    | ((v1 >> 8) & 0xff000000) | ((v1 >> 24) & 0xff0000)
                    | ((v1 >> 40) & 0xff00) | (v1 >> 56));
                v[0] = v1;
                v[1] = v0;
                v += 2;
            }
        }

#endif

#if SIZEOF_LONG_DOUBLE == 12

        uint32 i;
        for(i = 0 ; i < length ; i++)
            value[i] = read_longdouble(s);

#endif
    }
}
#endif


void align_next(stream *s,size_t n)
{
    s->align_next = n;
}

void write_endian(stream *s)
{ 
    write_boolean(s,Endian); 
}

void write_length(size_t start)
{
    size_t length = s->buf->ptr - s->buf->data - (start + 4);
    char* p = s->buf->data + start;
    *((size_t*)p) = length;
}

void write_align(stream *s,size_t size, size_t align)
{
    /* use write_align(ULong) if align == 0 */
    assert(align > 0); 

    /*
     If we're at the end of the current buffer, then we are about
     to write new data. We must first check if we need to start a
     chunk, which may result in a recursive call to write_align().
    */
    if(s->buf->ptr == s->buf->end){
        check_chunk();
    }

    /* If s->align_next is set, then use the larger of s->align_next and align*/
    if(s->align_next > 0){
        align = (s->align_next > align ? s->align_next : align);
        s->align_next = 0;
    }
    
    /* Align to the requested boundary */
	register char* newcur = s->buf->ptr;
	
    /*If there isn't enough room, then reallocate the buffer*/
    if(newcur + size > s->buf->end) {
        s->buf->realloc(newcur - s->buf->data + size);
	}
}

//
// write data type buffer
//

void write_char(stream *s,char value)
{
    write_align(s, 1, 1);
    *(s->buf->ptr) = value;
    s->buf->ptr += 1;
}

void write_byte(stream *s,char value)
{
    write_align(s, 1, 1);
    *(s->buf->ptr) = value;
    s->buf->ptr += 1;
}

void write_boolean(stream *s,bool value)
{
    write_align(s, 1, 1);
    *(s->buf->ptr) = value;
    s->buf->ptr += 1;
}

void write_octet(stream *s,unsigned char value)
{
    write_align(s, 1, 1);
    *(s->buf->ptr) = value;
    s->buf->ptr += 1;
}

void write_short(stream *s,int16 value)
{
    write_align(s, 2, 2);
    *(int16*)(s->buf->ptr) = value;
    s->buf->ptr += 2;
}

void write_ushort(stream *s,uint16 value)
{
    write_align(s, 2, 2);
    *(uint16*)(s->buf->ptr) = value;
    s->buf->ptr += 2;
}

void write_long(stream *s,int32 value)
{
    write_align(s, 4, 4);
    *(int32*)(s->buf->ptr) = value;
    s->buf->ptr += 4;
}

void write_ulong(stream *s,uint32 value)
{
    write_align(s, 4, 4);
    *(uint32*)(s->buf->ptr) = value;
    s->buf->ptr += 4;
}

void write_longlong(stream *s,int64 value)
{
    write_align(s, 8, 8);
    *(int64*)(s->buf->ptr) = value;
    s->buf->ptr += 8;
}

void write_ulonglong(stream *s,uint64 value)
{
    write_align(s, 8, 8);
    *(uint64*)(s->buf->ptr) = value;
    s->buf->ptr += 8;
}

void write_float(stream *s,float value)
{
    write_align(s, 4, 4);
    *(float*)(s->buf->ptr) = value;
    s->buf->ptr += 4;
}

void write_double(stream *s,double value)
{
    write_align(s, 8, 8);
    *(double*)(s->buf->ptr) = value;
    s->buf->ptr += 8;
}

void
write_string(stream *s,const char* value)
{
    size_t len = strlen(value);
    uint32 capacity = (uint32)len + 1;
    write_ulong(capacity);

    memcpy(s->buf->ptr, value, len);
    s->buf->ptr += len;
    *(s->buf->ptr++) = '\0';
}

void 
write_boolean_array(stream *s,const bool* value, 
                                size_t length)
{
    if(length)
    {
        write_align(s,length,1);
#if SIZEOF_BOOL == 1
        memcpy(s->buf->ptr, value, length);
        s->buf->ptr += length;
#else 
        for(size_t i = 0; i < length; i++)
        {
            *(s->buf->ptr) = value[i] ? 1 : 0;
            s->buf->ptr++;
        }
#endif
    }
}

void 
write_char_array(stream *s,const char* value, 
                             size_t length)
{
    if(length)
    {
        write_align(s,length,1);
        memcpy(s->buf->ptr, value, length);
        s->buf->ptr += length;
    }
}

void 
write_short_array(stream *s,const int16* value, 
                              size_t length)
{
    if(length)
    {
        size_t arrsize = length * 2;
        write_align(s, arrsize, 2);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_ushort_array(stream *s,const uint16* value, 
                               size_t length)
{
    if(length)
    {
        size_t arrsize = length * 2;
        write_align(s, arrsize, 2);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_long_array(stream *s,const int32* value, 
                             size_t length)
{
    if(length)
    {
        size_t arrsize = length * 4;
        write_align(s, arrsize, 4);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_ulong_array(stream *s,const uint32* value, 
                              size_t length)
{
    if(length)
    {
        size_t arrsize = length * 4;
        write_align(s, arrsize, 4);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_longlong_array(stream *s,const int64* value,
                                 size_t length)
{
    if(length)
    {
        size_t arrsize = length * 8;
        write_align(s, arrsize, 8);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_ulonglong_array(stream *s,const uint64* value,
                                  size_t length)
{
    if(length)
    {
        size_t arrsize = length * 8;
        write_align(s, arrsize, 8);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_float_array(stream *s,const float* value, 
                              size_t length)
{
    if(length)
    {
        size_t arrsize = length * 4;
        write_align(s, arrsize, 4);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

void 
write_double_array(stream *s,const double* value,
                               size_t length)
{
    if(length)
    {
        size_t arrsize = length * 8;
        write_align(s, arrsize, 8);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
    }
}

#if SIZEOF_LONG_DOUBLE < 12

void
write_longdouble_array(stream *s,const long double* value, size_t length)
{
    DBG("Stream: write long double not supported\n");    
}

#else

void
write_longdouble_array(stream *s,const long double* value, size_t length)
{
    if(length)
    {
#if SIZEOF_LONG_DOUBLE == 16

        size_t arrsize = length * 16;
        write_align(s, arrsize, 8);
        memcpy(s->buf->ptr, value, arrsize);
        s->buf->ptr += arrsize;
#endif

#if SIZEOF_LONG_DOUBLE == 12
        for(size_t i = 0 ; i < length ; i++)
            write_longdouble(value[i]);
#endif
    }
}

#endif