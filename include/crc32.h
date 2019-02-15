/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 * 
 * Functions for computing 32-bit CRC.
 * 
* Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: crc32.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */

#ifndef CRC32_H
#define CRC32_H

/* This computes a 32 bit CRC of the data in the buffer, and returns the
   CRC.  The polynomial used is 0xedb88320. */
unsigned long crc32(const unsigned char *buf, unsigned int len);

#endif /* CRC32_H */
