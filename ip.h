/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* ip.h */
#ifndef IP_H_SEEN
#define IP_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif


extern unsigned int createip K__P F__P(unsigned char *, unsigned char *, unsigned char)P__F;
void ip K__P F__P(unsigned int)P__F;
void readhostip K__P F__P(void)P__F;
void dotcpip K__P F__P(void)P__F;
/* this is for immediate processing of an IP packet just recieved */
#define RECEIVED(packet) ip(packet)
#define GETCONFIG readhostip();
#if HANDLEFRAGS
#define PROCFRAGS dotcpip();
#endif
/*
8 bits to play with for locking/message passing
should be enuff for all kinds of things :-)
anything else we need?

0 READY (transmit the buffer to dev)
1 BUSY (transmitting buffer)
2 FILLBUSY (filling buffer, don't bother it)
3 SENT (waiting on ACK)
4 SENDNOCARE (is UDP or ICMPish, or don't expect an ack)
5
6
7 1==tx buffer

0 SWALLOWING (filling buffer from dev buffer)
1 SWALLOWED (buffer ready for IP processing layer)
2 FRAGMENT (buffer was previously processed and tagged)
3 UDP -- buffer ready for UDP processing layer
4 TCP -- buffer ready for TCP processing layer
5 ICMP -- buffer ready for ICMP processing layer
6
7 0==rx buffer

 */

#define READY 0x01
#define BUSY 0x02
#define FILLBUSY 0x04
#define SENT 0x08
#define SENDNOCARE 0x10
#define ISTX 0x80

#define SWALLOWING 0x01
#define SWALLOWED 0x02



#define FLAGMASK 0x1f

#if HANDLEFRAGS
#define FRAGMENT 0x04
#define STARTFRAGTIME 20 /* this probably sucks as a value, but oh-well */
#define FRAGFLAG 0x20
#endif /* HANDLEFRAGS */


#endif /* IP_H_SEEN */
