/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
 * Microchip family CDC interface using MLA v2013_12_20.
 *
 * Only tested on PIC24FJ192GB106 Others should work too.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"

#if USE_PICxxxx_CDC
char HARDDRV[] = "Microchip CDC driver Version 0.0.1";
void initializedev(VOIDFIX) {
        USBCDCBegin();
}

/* Just in case we are too slow, we need to empty buffers */
void zapbuffers(VOIDFIX) {
        /* do a callback that cleans out the inbound buffer */
        while(USBCDCAvailable()) USBCDCgetc();
}

void pollinput(VOIDFIX) {
        /* does nothing, CDC is on an ISR */
}

/* check if there is data in the inbound buffer */
int checkdata(VOIDFIX) {
        return USBCDCAvailable();
}

/* check if there is data in the outbound buffer */
int check_out(VOIDFIX) {
        /* We can't do this :-( */
        return 0;
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* buffer is not under our control. */
        return(4096);
}

/* output data */
void outtodev(unsigned int c) {
        unsigned char a;
        a = c & 0xff;
        USBCDCputc(a);
}

/* wait for data and return it */
unsigned char infromdev(VOIDFIX) {
        unsigned char c;
        while(!USBCDCAvailable());
        c = USBCDCgetc();
        return(c);
}

#else
/* just make a pretend symbol */
void PICXXXX_CDC(VOIDFIX) {
        return;
}
#endif