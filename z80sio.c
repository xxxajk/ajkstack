/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

z80sio.c

Generic Z80SIO Z8440 RS-232C Driver _WITH HARDWARE FLOW CONTROL_


For hardware flow control to work,
PHYSICALLY swap the DTR and CTS lines on the cable.
This is because we need to actually control the CTS line manually.


 ***Driver	05	z80sio
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USEZ80SIO /* Include this code? */

/* Config for Z80 SIO */
/*
   If your system has it's SIO located in a different location than mine,
   change it in hrdware.h
 */
/* At higher speeds, this can be lowered. I think I will auto calibrate this */
#define TESTLOOP	6  /* loop to not drop out  */

/*
    Anything below shouldn't need modification.
 */

#define senddata(abyte)	(outp(SIO_DATA_PORT, abyte))		/* transmit data	 */
#define readdata	(inp(SIO_DATA_PORT))			/* read data		 */
#define controlw(abyte)	(outp(SIO_CMD_PORT, abyte))		/* write to control port */
#define controlr	(inp(SIO_CMD_PORT))			/* read control port	 */

#define outok		((inp(SIO_CMD_PORT) & 0x04))		/* TXEMPTY?		 */
#define ctsok		((inp(SIO_CMD_PORT) & 0x20))		/* CTS? 		 */
#define inok		((inp(SIO_CMD_PORT) & 0x01))		/* RXAVAILABLE?		 */

/* this should do the trick to buffer enough, if not, to bad :-) */
#define RSERBUFSIZE 300
#define TSERBUFSIZE 74
#ifdef CPM
#include <cpm.h>
#endif
#include <conio.h>

char HARDDRV[] = "Generic Z80SIO driver Version 0.0.2";
unsigned char *serbuf;
unsigned char *tserbuf;
unsigned int serhead;
unsigned int sertail;
unsigned int serdata;
unsigned int tserhead;
unsigned int tsertail;
unsigned int tserdata;
unsigned int wreg0, wreg1, wreg2, wreg3, wreg4, wreg5;

int fixgetch(VOIDFIX) {
        return(getch());
}

int fixkbhit(VOIDFIX) {
        return(kbhit());
}

void writesio(regi, data)
int regi;
unsigned int data;
{
        if(regi) {
                controlw(regi);
                controlw(data);
        } else {
                controlw(data);
        }
}

/* WARNING this does NOT check for wrong register reads!!! */
unsigned int readsio(regi)
int regi;
{
        if(regi > 0) {
                controlw(regi & 0x07);
                return(controlr);
        } else {
                return(controlr);
        }
}

/* tell sender to shut up! */
/* #define X_FM 0x7f */
/* #define X_FC 0x80 */
#define X_FM 0xf8
#define GO 0x02
#define STOP 0x00

#define flowcontroll(onoff) wreg5 = (wreg5 & X_FM) | onoff; controlw(5); controlw(wreg5);
/*
#define flowcontroll(onoff)
 */

/* clean out for overflows... because we can't keep up */
void zapbuffers(VOIDFIX) {
        if(serdata > (RSERBUFSIZE - 2)) {
                serhead = sertail = serdata = 0;
        }
}

/* suck/spew data if exists into/from buffer */
void pollinput(VOIDFIX) {
        if((serdata <= (RSERBUFSIZE - 2)) && (inok)) {
                serbuf[serhead] = readdata & 0xff;
                serhead++;
                serdata++;
                if(serhead == RSERBUFSIZE) serhead = 0;
        }

        /* notice that polled output is also done here */
        while(ctsok && outok && (tserdata > 0)) {
                senddata(tserbuf[tsertail]);
                tsertail++;
                tserdata--;
                if(tsertail == TSERBUFSIZE) tsertail = 0;
        }

        if((serdata <= (RSERBUFSIZE - 2)) && (!inok)) {
                flowcontroll(GO); /* spew! */
                flowcontroll(STOP); /* shut up! */
        }
}

/* check if there is data in the buffer */
int checkdata(VOIDFIX) {
        pollinput(); /* might as well check... */
        return(serdata);
}

int check_out(VOIDFIX) {
        pollinput();
        return(tserdata);
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* This driver always commits it's buffers. */
        return(4096);
}

/* output data to port buffer, if busy, suck in/blow out data */
void outtodev(c)
unsigned int c;
{
        while(tserdata == TSERBUFSIZE - 1) {
                pollinput();
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
        serhead = sertail = serdata = tserhead = tsertail = tserdata =
                wreg1 = /* Don't touch this */
                wreg2 = 0; /* Don't touch this */
#if USECOM8116
        outp(COM_8116, BAUD_UART); /* set the baud rate on the 8116 */
#endif /* USE_8116 */
        /* main register */
        wreg0 = 0x18;
        writesio(0, wreg0); /* RESET register selects */
        writesio(0, wreg0); /* another, in case register 0 wasn't selected */

        /* rx/tx common options */
        wreg4 = WREGCLK; /* NoParity, 1 stop | clock */
        writesio(4, wreg4);

        /* irq register */
        writesio(1, wreg1);

        /* irq vector register */
        /* writesio(2, wreg2); */

        /* rx settings */
        wreg3 = 0xc1; /* RXenabled, 8bits/char  ... 0xe1 for dart */
        writesio(3, wreg3);


        /* tx settings & control */
        wreg5 = 0xea; /* RTS, TXenabled, 8bits/char, DTR  */
        writesio(5, wreg5);
        flowcontroll(0);
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV);
}

#else /* USEZ80DART */

/* just make a pretend symbol */
void z80sio(VOIDFIX) {
        return;
}
#endif /* USEZ80DART */


/* tell link parser who we are and where we belong

1105 libznet.lib z80sio.obj xxLIBRARYxx

 */

/*
; List of commands to set up SIO channel A for asynchronous operation.

siotbl: DB      18H             ; Channel reset
        DB      18H             ; another, in case register 0 wasn't selected
        DB      04H             ; Select register 4
        DB      44H             ; 1 stop bit, clock*16
        DB      01H             ; Select register 1
        DB      00H             ; No interrupts enabled
        DB      03H             ; Select register 3
        DB      0C1H            ; Rx enable, 8 bit Rx character
        DB      05H             ; Select register 5
        DB      0EAH            ; Tx enable, 8 bit Tx character,
                                ;  raise DTR and RTS
siolen  equ     $-siotbl        ; length of command list

 */