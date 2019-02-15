/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * MD5.c  Message-Digest Algorithm 5
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: md5.h 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */
 
#ifndef _MD5_H_
#define _MD5_H_

#include "types.h"

struct MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* _MD5_H_ */
