/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

c128acia.c

C128 with ACIA in CP/M mode with ACIAFIX

 ***Driver	07	c128acia
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USEACIA /* Include this code? */

#ifdef CPM
#include <cpm.h>
#endif
#include <conio.h>


#define ACIA 0xde00

int kickme = 0;

#define ACIA_DATA_PORT ACIA
#define ACIA_STA_PORT (ACIA + 1)
#define ACIA_CMD_PORT (ACIA + 2)
#define ACIA_CTL_PORT (ACIA + 3)

/* At higher speeds, this can be lowered. I think I will auto calibrate this */
#define TESTLOOP	6  /* loop to not drop out  */

/*
    Anything below shouldn't need modification.
 */

#define senddata(abyte)	(outp(ACIA_DATA_PORT, abyte))		/* transmit data	 */
#define readdata	(inp(ACIA_DATA_PORT))			/* read data		 */
#define controlw(abyte)	(outp(ACIA_CTL_PORT, abyte))		/* write to control port */
#define controlr	(inp(ACIA_CTL_PORT))			/* read control port	 */
#define statusr		(inp(ACIA_STA_PORT))			/* read status port	 */
#define commandw(abyte) (outp(ACIA_CMD_PORT, abyte))		/* write command port	 */
#define commandr	(inp(ACIA_CMD_PORT))			/* read command port	 */



#define outok		((inp(ACIA_STA_PORT) & 0x10))		/* TXEMPTY?		 */
#define inok		((inp(ACIA_STA_PORT) & 0x08))		/* RXAVAILABLE?		 */

/* this should do the trick to buffer enough, if not, to bad :-) */
#define RSERBUFSIZE 512
#define TSERBUFSIZE 128

char HARDDRV[] = "C128 ACIAFIX driver Version 0.0.2";
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
void pollinput(VOIDFIX) {
        register short didit;

        if(serdata < RSERBUFSIZE) kickme++;
        if(kickme > 255) {
                kickme = 0;
                didit = readdata; /* kick the aciafix board */
        }
more:
        didit = 0;

        while((serdata < RSERBUFSIZE) && inok) {
                kickme = 0;
                serbuf[serhead] = readdata & 0xff;
                serhead++;
                serdata++;
                if(serhead == RSERBUFSIZE) serhead = 0;
                didit++;
        }

        /* notice that polled output is also done here */
        while(outok && (tserdata > 0)) {
                senddata(tserbuf[tsertail]);
                tsertail++;
                tserdata--;
                if(tsertail == TSERBUFSIZE) tsertail = 0;
                didit++;
        }
        if(didit) goto more;
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
        serhead = sertail = serdata = tserhead = tsertail = tserdata = 0;

        /* for the moment, rely on device.com to set things up... */
        serbuf = safemalloc(RSERBUFSIZE);
        tserbuf = safemalloc(TSERBUFSIZE);
        printf("%s\n", &HARDDRV);
}

#else /* USEACIA */

/* just make a pretend symbol */
void c128acia(VOIDFIX) {
        return;
}
#endif /* USEACIA */


/* tell link parser who we are and where we belong

1107 libznet.lib c128acia.obj xxLIBRARYxx

 */

