/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

dosuart.c

Generic DOS UART RS-232C Driver
Output MUST be buffered.
IRQ's less than 9 only supported.

 ***Driver	04	dosuart
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USEDOSUART /* Include this code? */
#include "net.h"

#ifdef M5
#include <conio.h>
#include <dos.h>
/* NORMALIZE */
#define cli _disable
#define sti _enable
#endif /* M5 */

#if POLLED_DOS_UART
/* this should do the trick to buffer enough, if not, to bad :-) */
char HARDDRV[] = "POLLED DOS UART driver Version 0.0.1";
#define RSERBUFSIZE 32
#define TSERBUFSIZE 1024
#else /* POLLED_DOS_UART */
char HARDDRV[] = "ISR DOS UART driver Version 0.0.1";
#define RSERBUFSIZE 32
#define TSERBUFSIZE 4096
/* 2 to RSERBUFSIZE - 1 */
#define LOWWATER 8
/* LOWWATER + 1 to RSERBUFSIZE - 17 */
#define HIGHWATER (RSERBUFSIZE - 17)
#endif /* POLLED_DOS_UART */

unsigned char *serbuf;
unsigned char *tserbuf;
volatile unsigned int serhead;
volatile unsigned int sertail;
volatile unsigned int serdata;
unsigned int tserhead;
unsigned int tsertail;
volatile unsigned int tserdata;
static void (interrupt far *old_break_ISR)();

int fixgetch(VOIDFIX) {
        return(getch());
}

int fixkbhit(VOIDFIX) {
        return(kbhit());
}

/* clean out for overflows... because we can't keep up */
void zapbuffers(VOIDFIX) {
#if !POLLED_DOS_UART

        if(serdata > (RSERBUFSIZE - 2)) {
                cli();
                serhead = sertail = serdata = 0;
                sti();
                outp(BASE_8250 + 4, 0x0b);
        }
#endif
}

void interrupt far break_ISR(VOIDFIX) {
}

void breakon(VOIDFIX) {
        _dos_setvect(0x23, old_break_ISR);
}

void breakoff(VOIDFIX) {
        old_break_ISR = _dos_getvect(0x23);
        _dos_setvect(0x23, break_ISR);
}


#if POLLED_DOS_UART

/* polled i/o is a pain in the ass, however it works best */

void polloutput(VOIDFIX) {
        unsigned char c;

        if(tserdata > 0) {
                c = (unsigned char) (inp(BASE_8250 + 6) & 0x10);
                c = ((unsigned char) (inp(BASE_8250 + 5) & 0x20)) | c;
                if(c == 0x30) {
                        /* output data to port. */
                        outp(BASE_8250, tserbuf[tsertail] & 0xff);
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

        cli();
        c = (unsigned char) (inp(BASE_8250 + 5) & 0x02);
        if(c) best = best + 11;
        if(serdata < RSERBUFSIZE - 32) {
                c = (unsigned char) (inp(BASE_8250 + 5) & 0x01);
                /* talk, then shutup, if we need to  */
                if(!c) {
                        outp(BASE_8250 + 4, 0x03);
                }
                timeout = best;
                while(timeout && (serdata < RSERBUFSIZE - 1)) {
                        c = (unsigned char) (inp(BASE_8250 + 5) & 0x01);
                        if(c) {
                                outp(BASE_8250 + 4, 0x01);
                                serbuf[serhead] = (unsigned char) (inp(BASE_8250) & 0xff);
                                serhead++;
                                serdata++;
                                if(serhead == RSERBUFSIZE) serhead = 0;
                                timeout = best + 5;
                        } else {
                                timeout--;
                        }
                }
                outp(BASE_8250 + 4, 0x01);
        }
        sti(); /* sti(); */
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

void downpolled(VOIDFIX) {
        endnetwork();
        breakon();
}

void initializedev(VOIDFIX) {
#if INIT_BAUD
        outp(BASE_8250 + 3, 0x83);
        outp(BASE_8250, BAUD_UART_LV);
        outp(BASE_8250 + 1, BAUD_UART_HV);
#endif
        outp(BASE_8250 + 3, 0x03); /* 8n1, why bother with any else? */
        outp(BASE_8250 + 4, 0x01);

        breakoff();
        atexit(downpolled);
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV[0]);
        return;
}
#else /* POLLED_DOS_UART */

static void (interrupt far *old_serial_ISR)();

/* check if there is data received, return queue size */
int checkdata(VOIDFIX) {
        if(serdata <= LOWWATER) {
                outp(BASE_8250 + 4, 0x0b);
        }
        return(serdata);
}

/* check if there is any data going out, return queue size */
int check_out(VOIDFIX) {
        return(tserdata);
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* This driver doesn't commit it's buffers... */
        return(TSERBUFSIZE - check_out());
}

/* output data to buffer. */
void outtodev(c)
unsigned int c;
{
        while(check_out() == (TSERBUFSIZE - 1)) {
                if(((inp(BASE_8250 + 6) & 0x10) == 0x10)) {
                        outp(BASE_8250 + 1, 0x0b);
                }
        }
        tserbuf[tserhead] = (c & 0xff);
        tserhead++;
        if(tserhead == TSERBUFSIZE) tserhead = 0;
        cli();
        tserdata++;
        sti();
        outp(BASE_8250 + 1, 0x0b);
}

/* wait for data and return it */
unsigned char infromdev(VOIDFIX) {
        register unsigned char c;

        while(!checkdata());
        c = serbuf[sertail];
        sertail++;
        if(sertail == RSERBUFSIZE) sertail = 0;
        cli();
        serdata--;
        sti();
        checkdata();
        return(c);
}

void interrupt far serial_ISR(VOIDFIX) {
        register int c;

        cli();
ISR_loop:
        c = inp(BASE_8250 + 2) & 0x07;
        switch(c) {
                case 0x04: /* data available */
                        if(serdata > (RSERBUFSIZE - 2)) {
                                /* unfortunatly, drop the char, this is better than a zap for some things. */
                                inp(BASE_8250);
                        } else {
                                serbuf[serhead] = (unsigned char) (inp(BASE_8250) & 0xff);
                                serhead++;
                                serdata++;
                                if(serhead == RSERBUFSIZE) serhead = 0;
                                if(serdata == HIGHWATER) {
                                        outp(BASE_8250 + 4, 0x09);
                                }
                        }
                        break;

                case 0x02: /* output ready for next */
                        if((!tserdata) || (!(inp(BASE_8250 + 6) & 0x10))) {
                                outp(BASE_8250 + 1, 0x09);
                        } else {
                                outp(BASE_8250, tserbuf[tsertail] & 0xff);
                                tsertail++;
                                tserdata--;
                                if(tsertail == TSERBUFSIZE) tsertail = 0;
                        }
                        break;

                case 0x06: /* we don't care about this one at all */
                        inp(BASE_8250 + 5);
                        /* fall thru */
                case 0x00: /* cts or other line changed */
                        if((inp(BASE_8250 + 6) & 0x10)) {
                                outp(BASE_8250 + 1, 0x0b);
                        } else {
                                outp(BASE_8250 + 1, 0x09);
                        }
                        break;

                default: /* nothing left to do */
                        sti(); /* sti(); */
                        outp(0x20, 0x20);
                        return;
        }
        goto ISR_loop;
}

void serialon(VOIDFIX) {
        old_serial_ISR = _dos_getvect(0x08 + IRQ_8250);
        _dos_setvect(0x08 + IRQ_8250, serial_ISR);
        cli();
        outp(BASE_8250 + 1, 0x09);
        outp(BASE_8250 + 4, 0x0b);
        outp(0x21, (inp(0x21) & ~((char) 1 << IRQ_8250)));
        sti();
}

void serialoff(VOIDFIX) {
        cli();
        outp(0x21, (inp(0x21) | ((char) 1 << IRQ_8250)));
        outp(BASE_8250 + 4, 0x01);
        outp(BASE_8250 + 1, 0x00);
        sti();
        _dos_setvect(0x08 + IRQ_8250, old_serial_ISR);

}

void cleanisrs(VOIDFIX) {
        endnetwork();
        serialoff();
        breakon();
}

void initializedev(VOIDFIX) {
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
#if INIT_BAUD
        outp(BASE_8250 + 3, 0x83);
        outp(BASE_8250, BAUD_UART_LV);
        outp(BASE_8250 + 1, BAUD_UART_HV);
#endif
        outp(BASE_8250 + 3, 0x03); /* 8n1, why bother with any else? */
        breakoff();
        serialon();
        atexit(cleanisrs);
        printf("%s\n", &HARDDRV[0]);
        return;
}


#endif /* POLLED_DOS_UART */

#else /* USEDOSUART */

/* just make a pretend symbol */
void dosuart(VOIDFIX) {
        return;
}

#endif /* USEDOSUART */


/* tell link parser who we are and where we belong

1104 libznet.lib dosuart.obj xxLIBRARYxx

 */

