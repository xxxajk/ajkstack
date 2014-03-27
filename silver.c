/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

silver.c

Generic 8250 RS-232C Driver
Output MUST be buffered.
Note: this is basically a clone of the dosuart driver.
This version is for c128 and "silver surfer"



 ***Driver	06	silver
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USESILVER /* Include this code? */
#include "net.h"
#ifdef CPM
#include <cpm.h>
#endif
#include <conio.h>

/* this should do the trick to buffer enough, if not, to bad :-) */
char HARDDRV[] = "POLLED DOS UART driver Version 0.0.1";
#define RSERBUFSIZE 64
#define TSERBUFSIZE 1024

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
                outp(BASE_SS + 4, 0x0b);
        }
}

/* polled i/o is a pain in the ass, however it works best */

void polloutput(VOIDFIX) {
        unsigned char c;

        if(tserdata > 0) {
                c = (unsigned char) (inp(BASE_SS + 6) & 0x10);
                c = ((unsigned char) (inp(BASE_SS + 5) & 0x20)) | c;
                if(c == 0x30) {
                        /* output data to port. */
                        outp(BASE_SS, tserbuf[tsertail] & 0xff);
                        tsertail++;
                        tserdata--;
                        if(tsertail == TSERBUFSIZE) tsertail = 0;
                }
        }
}

void pollinput(VOIDFIX) {
        unsigned char c;
        int timeout;

        static int best = 10;

        /*	cli(); */
        c = (unsigned char) (inp(BASE_SS + 5) & 0x02);
        if(c) best = best + 11;
        if(serdata < (RSERBUFSIZE - 32)) {
                c = (unsigned char) (inp(BASE_SS + 5) & 0x01);
                /* talk, then shutup, if we need to  */
                if(!c) {
                        outp(BASE_SS + 4, 0x03);
                }
                timeout = best;
                while(timeout && (serdata < RSERBUFSIZE - 1)) {
                        c = (unsigned char) (inp(BASE_SS + 5) & 0x01);
                        if(c) {
                                outp(BASE_SS + 4, 0x01);
                                serbuf[serhead] = (unsigned char) (inp(BASE_SS) & 0xff);
                                serhead++;
                                serdata++;
                                if(serhead == RSERBUFSIZE) serhead = 0;
                                timeout = best + 5;
                        } else {
                                timeout--;
                        }
                }
                outp(BASE_SS + 4, 0x01);
        }
        /* sti(); */
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
        outp(BASE_SS + 3, 0x83);
        outp(BASE_SS, BAUD_UART_LV);
        outp(BASE_SS + 1, BAUD_UART_HV);
        outp(BASE_SS + 3, 0x03); /* 8n1, why bother with any else? */
        outp(BASE_SS + 4, 0x01);

        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV[0]);
        return;
}


#else /* USEDOSUART */

/* just make a pretend symbol */
void silver(VOIDFIX) {
        return;
}

#endif /* USEDOSUART */


/* tell link parser who we are and where we belong

1106 libznet.lib silver.obj xxLIBRARYxx

 */

