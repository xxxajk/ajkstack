/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

z80dart.c

Generic Z80DART Z8470 RS-232C Driver _WITH HARDWARE FLOW CONTROL_

 ***Driver	01	z80dart
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"
#include "hrdware.h"

#if USEZ80DART /* Include this code? */

/* Config for Z80 DART */
/*
   If your system has it's DART located in a different location than mine,
   change it in hrdware.h
 */

/*
    Anything below shouldn't need modification.
 */


#define GO 0x80
#define STOP 0x00
#define senddata(abyte)	(outp(DART_DATA_PORT, abyte))		/* transmit data	 */
#define readdata	(inp(DART_DATA_PORT))			/* read data		 */
#define controlw(abyte)	(outp(DART_CMD_PORT, abyte))		/* write to control port */
#define controlr	(inp(DART_CMD_PORT))			/* read control port	 */

#define outok		((inp(DART_CMD_PORT) & 0x04))		/* TXEMPTY?		 */
#define ctsok		((inp(DART_CMD_PORT) & 0x20))		/* CTS? 		 */
#define inok		((inp(DART_CMD_PORT) & 0x01))		/* RXAVAILABLE?		 */
#define readdatak	(inp(DART_DATA_PORT - 2))		/* read keyboard data	 */
#define inokk		((inp(DART_CMD_PORT - 2) & 0x01))	/* Keyboard RXAVAILABLE? */

/* this should do the trick to buffer enough, if not, to bad :-) */
#define RSERBUFSIZE 128
#define TSERBUFSIZE 128

char HARDDRV[] = "Generic Z80DART driver Version 0.0.3";
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
        /*
        get a char and don't screw up the console...
        you WILL need to modify this on a cp/m system
        to match _your_ system!
         */
        for(; !inokk;);
        return(readdatak);
}

int fixkbhit(VOIDFIX) {
        return(inokk);
}

void writedart(regi, data)
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
unsigned int readdart(regi)
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
#define flowcontroll(onoff) wreg5 = (wreg5 & 0x7F) | onoff; controlw(5); controlw(wreg5);

/* clean out for overflows... because we can't keep up */
void zapbuffers(VOIDFIX) {
        if(serdata > (RSERBUFSIZE - 2)) {
                serhead = sertail = serdata = 0;
        }
}

/*
   Suck/spew data if exists into/from buffer.
   This assumes a proper uart on the server with AFE or properly emulated AFE.
 */
void pollinput(VOIDFIX) {

        int didsomething;

moredata:
        didsomething = 0;
        if((serdata <= (RSERBUFSIZE - 2))) {
                if(inok) {
                        serbuf[serhead] = readdata & 0xff;
                        serhead++;
                        serdata++;
                        if(serhead == RSERBUFSIZE) serhead = 0;
                        didsomething++;
                }
                flowcontroll(GO); /* spew! */
                flowcontroll(STOP); /* shut up! */
        }
        /* notice that polled output is also done here */
        if(ctsok && outok && (tserdata > 0)) {
                senddata(tserbuf[tsertail]);
                tsertail++;
                tserdata--;
                if(tsertail == TSERBUFSIZE) tsertail = 0;
                didsomething++;
        }

        if(didsomething) goto moredata;

}

/* check if there is data in the buffer */
int checkdata(VOIDFIX) {
        pollinput(); /* might as well check... */
        return(serdata);
}

int check_out(VOIDFIX) {
        pollinput(); /* might as well check, heck, you never know */
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
        /* baud rate is determined by a physical jumper, altho I think we can divide */
        /* assume the port is properly pre-set, seems the case on mine */
        serhead = sertail = serdata = tserhead = tsertail = tserdata = 0;
        /* main register */
        wreg0 =
                wreg1 = /* Don't touch this, it's used by the system for reboot! */
                wreg2 = 0; /* Don't touch this, it's used by the system for reboot! */
        writedart(0, wreg0);
        writedart(0, wreg0);
        wreg0 = 0x18; /* RESET register selects */
        writedart(0, wreg0);
        wreg0 = 0x10;
        writedart(0, wreg0);
        /* irq register */
        /* writedart(1, wreg1); */

        /* irq vector register */
        /* writedart(2, wreg2); */

        /* rx settings */
        wreg3 = 0xe1; /* RXenabled, 8bits/char  */
        writedart(3, wreg3);

        /* rx/tx common options */
        wreg4 = 0x08 | WREGCLK; /* NoParity, 1.5stop | clock */
        writedart(4, wreg4);

        /* tx settings & control */
        wreg5 = 0x6a; /* RTS, TXenabled, 8bits/char, DTR (DTR is really mapped to CTS) */
        writedart(5, wreg5);
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV);
}

#else /* USEZ80DART */

/* just make a pretend symbol */
void z80dart(VOIDFIX) {
        return;
}
#endif /* USEZ80DART */


/* tell link parser who we are and where we belong

1101 libznet.lib z80dart.obj xxLIBRARYxx

 */

