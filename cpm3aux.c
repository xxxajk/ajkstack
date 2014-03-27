/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

cpm3aux.c

Generic CPM 3.0 RS-232C Driver _WITH HARDWARE FLOW CONTROL IN BIOS_
Output MUST be buffered.

 ***Driver	00	cpm3aux
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USECPM3AUX /* Include this code? */

/* this should do the trick to buffer enough, if not, to bad :-) */
#define RSERBUFSIZE 64
#define TSERBUFSIZE 64

#ifdef CPM
#include <cpm.h>
#endif
#include <conio.h>

char HARDDRV[] = "CP/M 3.x AUX driver Version 0.0.2";
unsigned char *serbuf;
unsigned char *tserbuf;
unsigned int serhead;
unsigned int sertail;
unsigned int serdata;
unsigned int tserhead;
unsigned int tsertail;
unsigned int tserdata;

int fixgetch(VOIDFIX) {
        return(getch());
}

int fixkbhit(VOIDFIX) {
        return(kbhit());
}

/* clean out for overflows... because we can't keep up */
void zapbuffers(VOIDFIX) {
        if(serdata > (RSERBUFSIZE - 2)) {
                serhead = sertail = serdata = 0;
        }
}

/* suck/spew data if exists into/from buffer */
void polloutput(VOIDFIX) {
        char c;
        /* 0FFh is returned if the Auxiliary Output device is ready for characters;
         otherwise 0 is returned.
         */
        if(tserdata > 0) {
                c = bdos(0x08);
                if(c) {
                        /* output data to port. */

                        bdos(0x04, (int) (tserbuf[tsertail] & 0xff));
                        tsertail++;
                        tserdata--;
                        if(tsertail == TSERBUFSIZE) tsertail = 0;
                }
        }
}

void pollinput(VOIDFIX) {
        char c;
        /* 0FFh is returned if the Auxiliary Input device has a character ready;
         otherwise 0 is returned.
         */
more:
        if(serdata < RSERBUFSIZE - 1) {
                c = bdos(0x07);
                if(c) {
                        serbuf[serhead] = (unsigned char) (bdos(0x03) & 0xff);
                        serhead++;
                        serdata++;
                        if(serhead == RSERBUFSIZE) serhead = 0;
                        goto more;
                }
        }

        return;
}

/* check if there is data received, return queue size */
int checkdata(VOIDFIX) {
        pollinput(); /* might as well check... */
        return(serdata);
}

/* check if there is any data going out, return queue size */
int check_out(VOIDFIX) {
        polloutput();
        return(tserdata);
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* This driver always commits it's buffers. */
        return(4096);
}

/* output data to buffer. */
void outtodev(c)
unsigned int c;
{
        while(tserdata == TSERBUFSIZE - 1) {
                polloutput();
        }
        tserbuf[tserhead] = (c & 0xff);
        tserhead++;
        tserdata++;
        if(tserhead == TSERBUFSIZE) tserhead = 0;
}

/* wait for data and return it */
unsigned char infromdev(VOIDFIX) {
        register unsigned char c;

        while(serdata < 1) {
                pollinput();
        }
        c = serbuf[sertail];
        sertail++;
        serdata--;
        if(sertail == RSERBUFSIZE) sertail = 0;
        return(c);
}

void initializedev(VOIDFIX) {
        /* Baud rate is pre-configured via DEVICE.COM, so not much here */
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV[0]);
        return;
}

#else /* USECPM3AUX */

/* just make a pretend symbol */
void cpm3aux(VOIDFIX) {
        return;
}
#endif /* USECPM3AUX */


/* tell link parser who we are and where we belong

1100 libznet.lib cpm3aux.obj xxLIBRARYxx

 */

