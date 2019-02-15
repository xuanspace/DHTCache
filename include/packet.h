/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Packet interfaces
 * 
* Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
 *
 * $Id: packet.h,v 1.2 2008-09-02 09:30:22 wlin Exp $
 *
 */
#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdio.h>

enum packet_flags
{
    ACK_TEST        = 0x080,  /* Acknowledgments test*/
    URG_STOP        = 0x100,  /* Network urgent stop*/
    URG_RAVEL       = 0x200,  /* Network urgent recovery*/
    SRV_INNER       = 0x400,  /* Identify inner connetion*/
    SRV_OUTER       = 0x800,  /* Identify outer connetion*/
    DBG_TRACE       = 0x1000  /* Debug trace packet*/
};

struct packet
{
	uint32  magic;			   /* Magic number*/
	uint8   verion;		   	   /* Packet version*/
    uint16  seq;			   /* Packet sequence*/
    uint16  flag;       	   /* Packet flags*/
    uint16  win;        	   /* Window size*/
    uint32  size;       	   /* packet body size */
}__attribute__ ((packed));


#endif /* _QUEUE_H_ */
