/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Hash function implementation
 *
 * Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: hash.c,v 1.1 2005-08-12 11:02:12 wxlin Exp $
 */
 
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hash.h"

static unsigned int hash_pjw(char *key, unsigned int len)
{
	unsigned int h = 0, g;
	char *end = key + len; 

	while (key < end) {
		h = (h << 4) + *key++;
		if ((g = (h & 0xF0000000))) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return h;
}

static unsigned int hash_nr2(char *key, unsigned int len)
{
	register unsigned int nr=1, nr2=4; 

	while (len) {
		nr^= (((nr & 63)+nr2)*((unsigned int)(unsigned char) *key++))+ (nr << 8);
		nr2+=3;
	}
	return nr;
} 

/* 
* Fowler/Noll/Vo hash 
* 
* The basis of the hash algorithm was taken from an idea sent by email to the 
* IEEE Posix P1003.2 mailing list from Phong Vo (kpv@research.att.com) and 
* Glenn Fowler (gsf@research.att.com). Landon Curt Noll (chongo@toad.com) 
* later improved on their algorithm. 
* 
* The magic is in the interesting relationship between the special prime 
* 16777619 (2^24 + 403) and 2^32 and 2^8. 
* 
* This hash produces the fewest collisions of any function that we¡¯ve seen so 
* far, and works well on both numbers and strings. 
*/

static unsigned int hash_fnv(char *key, unsigned int len)
{
	char *end = key+len;
	unsigned int hash;

	for (hash = 0; key < end; key++)
	{
		hash *= 16777619;
		hash ^= (unsigned int) *(unsigned char*) key;
	} 
	return (hash);
} 

/* 
* Integer hash function
* 
* Based on an original suggestion on Robert Jenkin's part in 1997
* 32 bit mix function
*/
static unsigned int hash_32shift(int key)
{
	key = ~key + (key << 15); // key = (key << 15) - key - 1;
	key = key ^ (key >>> 12);
	key = key + (key << 2);
	key = key ^ (key >>> 4);
	key = key * 2057; // key = (key + (key << 3)) + (key << 11);
	key = key ^ (key >>> 16);
	return key;
}


/*
 *  String hash functions
 * 
 */

// BKDR Hash 
unsigned int BKDRHash(char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;
 
	while (*str) {
		hash = hash * seed + (*str++);
	}
 
	return (hash & 0x7FFFFFFF);
}

unsigned int SDBMHash(char *str)
{
	unsigned int hash = 0;
 
	while (*str) {
		// equivalent to: hash = 65599*hash + (*str++);
		hash = (*str++) + (hash << 6) + (hash << 16) - hash;
	}
 
	return (hash & 0x7FFFFFFF);
}
 
// RS Hash 
unsigned int RSHash(char *str)
{
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int hash = 0;
 
	while (*str) {
		hash = hash * a + (*str++);
		a *= b;
	}
 
	return (hash & 0x7FFFFFFF);
}
 
// JS Hash 
unsigned int JSHash(char *str)
{
	unsigned int hash = 1315423911;
 
	while (*str) {
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}
 
	return (hash & 0x7FFFFFFF);
}
 
// P. J. Weinberger Hash 
unsigned int PJWHash(char *str)
{
	unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
	unsigned int ThreeQuarters	= (unsigned int)((BitsInUnignedInt  * 3) / 4);
	unsigned int OneEighth = (unsigned int)(BitsInUnignedInt / 8);
	unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt 
                                               - OneEighth);
	unsigned int hash	= 0;
	unsigned int test	= 0;
 
	while (*str) {
		hash = (hash << OneEighth) + (*str++);
		if ((test = hash & HighBits) != 0) {
			hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}
 
	return (hash & 0x7FFFFFFF);
}
 
// ELF Hash 
unsigned int ELFHash(char *str)
{
	unsigned int hash = 0;
	unsigned int x	= 0;
 
	while (*str) {
		hash = (hash << 4) + (*str++);
		if ((x = hash & 0xF0000000L) != 0) {
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}
 
	return (hash & 0x7FFFFFFF);
}
  
// DJB Hash 
unsigned int DJBHash(char *str)
{
	unsigned int hash = 5381;
 
	while (*str) {
		hash += (hash << 5) + (*str++);
	}
 
	return (hash & 0x7FFFFFFF);
}
 
// AP Hash 
unsigned int APHash(char *str)
{
	int i;
	unsigned int hash = 0;	
 
	for (i=0; *str; i++)
	{
		if ((i & 1) == 0)
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		else
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
	}
 
	return (hash & 0x7FFFFFFF);
}
