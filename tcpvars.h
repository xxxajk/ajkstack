/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* for all files to build the lib, only this one need be included */
/* MAS 04/22/02 - Fixed fragtime prototype */

#ifndef TCPVARS_H_SEEN
#define TCPVARS_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#include "ioconfig.h"
#include <ctype.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern unsigned char pakid[]; /* initial packet id */


/* pretend clock */
extern unsigned int tock;

/* ISN clock */
extern unsigned long isnclock;

/* tcpdump value */
extern int dodump;


/* to be filled via code */
extern unsigned char *bufptrs[];
extern unsigned char hostname[];
extern unsigned char myip[];
extern unsigned char remip[];
extern unsigned char mygate[]; /* not needed for slip */
/* future...
extern unsigned int mtucur;
extern unsigned int mrucur;
 */
extern unsigned char txrxflags[];
#if HANDLEFRAGS
extern unsigned long fragtime[];
#endif

/* retransmit errors stuffs */
extern unsigned char errortx[];
extern unsigned int sockexpecting[];
extern unsigned long timertx[];
extern unsigned int rxtxpointer[];
extern unsigned int sockdlen[];
extern char *sock_data[];
/* socket data ring buffers I COULD use the comms buffers...
hrmmm... I may l8r if this gets to be to huge of a lib...
as far as writes go, just commit the packet immediately.
 */
extern unsigned int rbufhead[];
extern unsigned int rbuftail[];
extern int rbufcount[];
extern unsigned char *rdatapointers[];
extern unsigned char *wdatapointers[];
extern int wbufcount[];

extern unsigned char *sockets; /* socket information */

extern unsigned short portcounter;

extern unsigned long ticktockclock1;
extern unsigned long ticktockclock2;
extern unsigned long loopcali;

extern char STACKVERSION[];
#ifdef	__cplusplus
}
#endif

#endif /* TCPVARS_H_SEEN */

