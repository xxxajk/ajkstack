/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

/*
        Globals in all implementations go here
        Luckilly, there aren't alot of them.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "socket.h"
#include "in.h"
#include "netinet.h"

/* globals */

unsigned char mydns[4];

/* pretend clock... perhaps... accuracy only if on a real interrupt */
unsigned int tock;

/* ISN clock... same shit as above... */
unsigned long isnclock;

/* tcpdump value */
int dodump = 0;

unsigned char pakid[] = {0xaa, 0x55}; /* initial packet id, this should be more random */
/* to be filled via code */
unsigned char *bufptrs[BUFFERS + 1];
unsigned char hostname[40]; /* need a larger one? then your INSANE */
unsigned char myip[4];
unsigned char remip[4 * (BUFFERS + 1)];
unsigned char mygate[4]; /* not needed for slip */

/* future...
unsigned int mtucur = MAXMTUMRU;
unsigned int mrucur = MAXMTUMRU;
 */
unsigned char txrxflags[BUFFERS + 1];
#if HANDLEFRAGS
unsigned long fragtime[BUFFERS + 1];
#endif
/* retransmit errors and socket stuffs */
unsigned char errortx[BUFFERS + 1];
unsigned int sockexpecting[BUFFERS + 1];
unsigned long timertx[BUFFERS + 1];
unsigned int rxtxpointer[BUFFERS + 1];
unsigned int sockdlen[BUFFERS + 1];

/* socket data ring buffers I COULD use the comms buffers...
hrmmm... I may l8r if this gets to be too huge of a lib...

 */
unsigned int rbufhead[BUFFERS + 1];
unsigned int rbuftail[BUFFERS + 1];
int rbufcount[BUFFERS + 1];
unsigned char *rdatapointers[1 + BUFFERS];
unsigned char *wdatapointers[1 + BUFFERS];
int wbufcount[BUFFERS + 1];

unsigned char *sockets; /* socket information */

unsigned short portcounter = 0x8000;

unsigned long ticktockclock1;
unsigned long ticktockclock2;
unsigned long loopcali;

char STACKVERSION[] = STACK_IS;


/* tell link parser who we are and where we belong

1300 libznet.lib tcpvars.obj xxLIBRARYxx

 */

