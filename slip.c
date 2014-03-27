/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
 RFC 1055 SLIP


 ***Wires	00	slip
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "tcpvars.h"
#include "ioconfig.h"

#if USESIMPLESLIP

#include "ip.h"
#include "net.h"
#include "slip.h"
#include "tcpx.h"
#include "compat.h"

/* debug safemallocs */
#define SLIPM 0

/* tx one slip character. */

void sliptxone(c)
unsigned int c;
{

        if(c == FRAME) {
                PUTDATA(NOTFRAME1);
                PUTDATA(NOTFRAME2);
                return;
        } else if(c == NOTFRAME1) {
                PUTDATA(NOTFRAME1);
                PUTDATA(NOTFRAME3);
        } else {
                PUTDATA(c);
        }
}

/* rx one slip character */
int sliprxone(VOIDFIX) {
        register unsigned char c;
        register unsigned char d;

        c = GETDATA;
        if(c == FRAME) return -1;
        if(c == NOTFRAME1) {
                d = GETDATA;
                if(d == NOTFRAME2) return(FRAME);
                if(d == NOTFRAME3) return(NOTFRAME1);
                return(-1);
        }
        return(c);
}

/* sliptx */
void sliptx(bufnum)
int bufnum;
{
        register unsigned char *buf;
        register unsigned int i;

        buf = bufptrs[bufnum];
        txrxflags[bufnum] = ((txrxflags[bufnum] & (~READY)) | BUSY) & 0xff;
#if DEBUGSLIP
        printf("\n\nTX %i\n", bufnum);
#endif
        PUTDATA(FRAME);
        for(i = 0; i < rxtxpointer[bufnum]; i++) {
                sliptxone(*buf);
#if DEBUGSLIP
                printf("%2.2x ", buf[i]&0xff);
                if(i == 19 || i == 39 || i == 59 || i == 79) putchar('\n');
#endif
                buf++;
#if 0
                if((i & 0x1f)) IOPOLL /* send as we go, but don't eat ALL the cpu */
#endif
                }
        PUTDATA(FRAME);
#if DEBUGSLIP
        putchar('\n');
#endif
        /*
        Free this buffer
         */
        freebuffer(bufnum);
        /* flush the tx buffers */

        /* method 1 */
#ifdef IOPOLL
        IOPOLL
        while(CHECKOUT > 0) {
                /* IOPOLL */
        }
#endif
        /* method 2 */
#ifdef XFLUSH_UART
        while(XFLUSH_UART);
#endif
}

/* sliprx, snag in scheduled chunks if need be */
void sliprx(VOIDFIX) {
        register int i;
        int j;
        register unsigned char *buf;
        int bufnum;
        int freebuf;
#ifdef LOOPS
        int loops; /* loop constraint test */
        loops = 64;
#endif
        bufnum = -1;
        freebuf = -1;

        /* what buffer is in-use or free? */
#ifdef IOPOLL
        IOPOLL
#endif
                for(i = 0; i < BUFFERS; i++) {
#ifdef IOPOLL
                IOPOLL
#endif
                        if((txrxflags[i] & 0xff) == SWALLOWING) {
                        bufnum = i;
                }
                if(!(txrxflags[i] & 0xff)) {
                        freebuf = i;
                }
        }
        if((bufnum > -1) || (freebuf > -1)) {
                if(bufnum < 0) {
                        /* new buffer */
                        bufnum = freebuf;
                        bufptrs[bufnum] = MALLOC(MAXMTUMRU);
                        if(!bufptrs[bufnum]) {
                                /* ran out of RAM, so we have to wait :-( */
#if SLIPM
                                printf("slip.c: %i No RAM\n", __LINE__);
#endif
                                /* Unfortunately, this can cause a "lockup" condition. */
                                /* The grand solution is to free an old buffer that is swallowed. */
                                bufnum = -1;
                                for(i = 0; i < BUFFERS; i++) {
                                        if((txrxflags[i] & 0xff) == SWALLOWED) {
                                                /* Sacrifice all inbound buffers so something else can do work. */
                                                txrxflags[i] = 0x00;
                                                free(bufptrs[i]);
                                                /* Whatever packet this was, is now dropped on the floor. */
                                        }
                                }
                                return;
                        }
                        txrxflags[bufnum] = SWALLOWING;
                        rxtxpointer[bufnum] = 0;
                }
                /* we have now the buffer and it's pointers */
                /* *PROBLEM*
                        if the buffer overflows during a ping flood
                        we end up with *A LOT* of garbage and no replies!
                        so lets gulp this bastard *fast* in here!
                 */
#ifdef IOPOLL
                IOPOLL
#endif
                        j = CHECKIN;
                while(j > 0) {
#ifdef IOPOLL
                        IOPOLL
#endif
                                i = sliprxone();
                        if(i == -1) {
                                if(rxtxpointer[bufnum] < 20) {
                                        /* assume line noise or
                                                        extra EOF char */
                                        rxtxpointer[bufnum] = 0x00;
#ifdef IOPOLL
                                        IOPOLL
#endif
                                } else {
                                        /* end of frame */
                                        txrxflags[bufnum] = SWALLOWED;
                                        RECEIVED(bufnum); /* process it */
                                        /* something to tx? */
                                        if((txrxflags[bufnum] & (ISTX | READY)) == (ISTX | READY)) {
                                                /* send it out, this reduces latency */
                                                sliptx(bufnum);
                                        }
                                        j = -1; /* bail out of this loop */
#ifdef IOPOLL
                                        IOPOLL
#endif
                                }
                        } else {
#ifdef IOPOLL
                                IOPOLL
#endif
                                        /* some data, save it. */
                                        buf = bufptrs[bufnum] + rxtxpointer[bufnum];
                                buf[0] = i & 0xff;
                                rxtxpointer[bufnum]++;
#ifdef IOPOLL
                                IOPOLL
#endif
                                        j = CHECKIN;
                                if(rxtxpointer[bufnum] > MAXMTUMRU) {
                                        /* buffer has filled!! */
                                        /* dump the fucker */
                                        rxtxpointer[bufnum] = 0;
                                        zapbuffers();
                                        j = -1;
                                }
                        }
#ifdef LOOPS
                        loops--;
                        if(!loops) {
                                j = -1; /* bail out, enough loops */
                        }
#ifdef KEYSBAD
                        if(fixkbhit()) {
                                j = -1; /* bail out, keyboard needs service */
                        }
#endif
#endif
                }
        } /* else there was no free buffer */
#ifdef IOPOLL
        IOPOLL
#endif
}

/* sliprxtx */

void sliprxtx(VOIDFIX) {
        int i;
        int gorx;

        gorx = 0;

        /* walk thru all tx buffers */
        for(i = BUFFERS - 1; i > 0; i--) {
                /* something to tx? */
#ifdef IOPOLL
                IOPOLL
#endif
                        if((txrxflags[i] & (ISTX | READY)) == (ISTX | READY)) {
                        /* send it out */
                        sliptx(i);
                }
#if HANDLEFRAGS
        }
        /* process fragments and anything else swallowed */
        PROCFRAGS
#else
                if((txrxflags[i] & (SWALLOWED)) == (SWALLOWED)) {
                        gorx = 1;
                        RECEIVED(i);
                }
        }
#endif /* HANDLEFRAGS */
#ifdef IOPOLL
        IOPOLL
#endif
                if(!gorx) sliprx();
}


#else

void notusingslip(VOIDFIX) {
        return;
}

#endif /* USESIMPLESLIP */


/* tell link parser who we are and where we belong

0200 libznet.lib slip.obj xxLIBRARYxx

 */

