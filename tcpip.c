/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

All together now! This connects TCP to IP, and connects SOCKETS to TCP.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "tcpvars.h"
#include "slip.h"
#include "in.h"
#include "netinet.h"
#include <time.h>
#include "ip.h"
#include "socket.h"
#include "tcpx.h"
#include "compat.h"

#define TCPIPVERBOSE 0

/* initialize tcpip and socket variables, bring up device */
void varinit(VOIDFIX) {
#if HAVE_RTC
        struct tm t;
#endif
        int i;

        sockets = safemalloc(SOKDAT * SOKALLOW);

        memset(sockets, 0, (SOKDAT * SOKALLOW));
        printf("TCP/UDP/IP stack Copyright 2001-2014 Andrew J. Kroll, http://tcp.dr.ea.ms/\n");

        for(i = 0; i < BUFFERS + 1; i++) {
                timertx[i] = errortx[i] = txrxflags[i] = 0x00;
#if HANDLEFRAGS
                fragtime[i] =
#endif
                        sockdlen[i] = rbufcount[i] = wbufcount[i] = 0;
                bufptrs[i] = rdatapointers[i] = wdatapointers[i] = NULL;
        }
        initializedev();
        tock = TIMETICK;
#if HAVE_RTC

        /*

        Systems with RTC ISN init, including:
        - Linux
        - ELKS
        - DOS
        - OS/M
        - MP/M
        - Concurrent-CP/M
        - CP/M 3.x (CP/M+)

         ****WARNING****
                THIS CODE CAN'T CHECK FOR BROKEN OR STUCK CLOCKS.
                IF ISN IS DUPLICATED ON SECOND CORE STARTUP, YOU
                HAVE TO WAIT ABOUT SIX MINUTES BEFORE YOU CAN
                ATTEMPT CONNECTING AGAIN.
                THIS ONLY AFFECTS INITIAL STACK CORE STARTUP.

         */

        isnclock = time((time_t *) & t);
        if(!isnclock) {
                isnclock = t.tm_sec + (t.tm_min * 60) + (t.tm_hour * 60) + (t.tm_yday * 3600);
        }
        if(!isnclock) {
                isnclock = rand();
#if TCPIPVERBOSE
                printf("\nNO RTC!!!\n");
#endif
        }
#if TCPIPVERBOSE
        else {
                printf("\nTIME %s\n", ctime((time_t *) & t));
        }
#endif
#else
        /* no RTC */
        isnclock = rand();
#endif
        portcounter = (isnclock & 0x0fff) + (isnclock & 0xf);
        while((portcounter < 32768) || (portcounter > 65000)) portcounter = portcounter * 2 + 12800;
        for(i = portcounter; i > 0; i--) {
                bumpid();
                inc32word((unsigned char *) &isnclock);
        }
#if TCPIPVERBOSE
        printf("\nISN=%8.8lx\n", isnclock);
#endif
}

/* tell link parser who we are and where we belong

0400 libznet.lib tcpip.obj xxLIBRARYxx

 */

