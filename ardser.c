/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

#include "ajkstack.h"

#if USEARDUINO
/* We are using the Arduino Serial */
#include <Arduino.h>
char HARDDRV[] = "Arduino Serial driver Version 0.0.1";

// Patch in to C++
//typedef struct SLIPDEV SLIPDEV;

void initializedev(VOIDFIX) {
        /* callback to C++ */
        SLIPDEVbegin(MY_BAUD_RATE);
}

/* Just in case we are too slow, we need to empty buffers */
void zapbuffers(VOIDFIX) {
        /* do a callback that cleans out the inbound buffer */
        while(SLIPDEVavailable()) SLIPDEVread();
}

void pollinput(VOIDFIX) {
        /* does nothing, serial is on an ISR */
}

/* check if there is data in the inbound buffer */
int checkdata(VOIDFIX) {
        return SLIPDEVavailable();
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
        SLIPDEVwrite(a);
}

/* wait for data and return it */
unsigned char infromdev(VOIDFIX) {
        unsigned char c;
        while(!checkdata());
        c = SLIPDEVread();
        return(c);
}

#endif /* USEARDUINO */
