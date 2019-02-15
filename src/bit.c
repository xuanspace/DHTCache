/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * dht contact node 
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: contact.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#include "define.h

/* \doc Set |bit| in |flags|. */

__inline__ void bit_set( unsigned long *flags, int bit )
{
   *flags |= (1UL << bit);
}

/* \doc Clear |bit| in |flags|. */

__inline__ void bit_clr( unsigned long *flags, int bit )
{
   *flags &= ~(1UL << bit);
}

/* \doc Test |bit| in |flags|, returning non-zero if the bit is set and
   zero if the bit is clear. */

__inline__ int bit_tst( unsigned long *flags, int bit )
{
   return (*flags & (1UL << bit));
}

/* \doc Return a count of the number of bits set in |flags|. */

__inline__ int bit_cnt( unsigned long *flags )
{
   unsigned long x = *flags;

#if SIZEOF_LONG == 4
   x = (x >> 1  & 0x55555555) + (x & 0x55555555);
   x = ((x >> 2) & 0x33333333) + (x & 0x33333333);
   x = ((x >> 4) + x) & 0x0f0f0f0f;
   x = ((x >> 8) + x);
   return (x + (x >> 16)) & 0xff;
#else
#if SIZEOF_LONG == 8
   x = (x >> 1  & 0x5555555555555555) + (x & 0x5555555555555555);
   x = ((x >> 2) & 0x3333333333333333) + (x & 0x3333333333333333);
   x = ((x >> 4) + x) & 0x0f0f0f0f0f0f0f0f;
   x = ((x >> 8) + x) & 0x00ff00ff00ff00ff;
   x = ((x >> 16) + x) & 0x0000ffff0000ffff;
   return (x + (x >> 32)) & 0xff;
#else
   err_internal( __FUNCTION__,
                 "Implemented for 32-bit and 64-bit longs, not %d-bit longs\n",
                 SIZEOF_LONG * 8 );
#endif
#endif
}
