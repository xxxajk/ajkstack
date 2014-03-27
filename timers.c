/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
   You will need to put in here some sort of timer.
   These are for use ONLY in protocol space!
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "ioconfig.h"


#if EXAMPLE
/*
 Here is a bogus timer set.
 You will need to make a proper one for your own system.
 This is here just incase you don't know how to make one yet.
 */

/* set clock 1 */
void settimer1(int seconds) {
        ticktockclock1 = seconds * loopcali;
        return;
}

/* what time is it Mr. fox? */
unsigned long chktimer1(VOIDFIX) {
        ticktockclock1++;
        return(ticktockclock1);
}

/* calibrate the timer */
void calibrate(VOIDFIX) {
        loopcali = 20; /* 20Hz */
        return;
}

#else /* EXAMPLE */

#if USE_TIME

#include <time.h>
#ifndef HI_TECH_C
#ifndef CPM
#ifndef M5
#ifndef ARDUINO
#include <sys/time.h>
#endif
#endif
#endif
#endif

#define settimer1(foo)

void calibrate(VOIDFIX) {
        loopcali = 1; /* 1Hz */
}

unsigned long chktimer1(VOIDFIX) {
        time_t ticks;
#ifdef ARDUINO
        RTC_systime();
#endif
        ticks = time(NULL);
        return((unsigned long) ticks);
}

#else /* USE_TIME */

/* set clock 1 */
void settimer1(seconds)
int seconds;
{
        ticktockclock1 = seconds * loopcali;
        return;
}

/* what time is it Mr. fox? */
unsigned long chktimer1(VOIDFIX) {
        ticktockclock1++;
        CHECKOUT; /* cause some I/O */
        return(ticktockclock1);
}

/* calibrate the timer */
void calibrate(VOIDFIX) {
        int i;

        /* zap any buffers so that we don't have to deal with any input */
drain:
#ifdef IOPOLL
        IOPOLL
#endif
                i = CHECKIN;

        while(i > 0) {
#ifdef IOPOLL
                IOPOLL
#endif
                        GETDATA;
                i = CHECKIN;
        }

        for(i = 0; i < 4; i++) {
#ifdef IOPOLL
                IOPOLL
#endif
                        /* make absolutely certain we are drained! */
                if((CHECKIN)) goto drain;
        }

        /* I need something to check against that has frequency.
          My link rate at 9600 baud, so I can time how long it
          takes to empty characters from the txqueue!
          This won't be heavily accurate, but accurate enough to work.
          baud / 11 should work for there is ~ 1bit of delay between chars
          this works providing:
          1: the txqueue is large enough
          2: the cpu is fast enough
          failing that, any number is better than no number at all!
         */
        i = 1 + (_CPM_SERIAL_CALIBRATION_ / 11);

        while(i > 0) {
                i--;
                PUTDATA(FRAME);
                loopcali++;
        }
        /* spew the characters till we are done. */
        settimer1(0);

        for(; CHECKOUT > 0; loopcali++) {
                chktimer1();
        }
        /* zap any buffers so that we don't have to deal with any bogus input */
#ifdef IOPOLL
        IOPOLL
#endif
                i = CHECKIN;
        while(i > 0) {
                GETDATA;
                i = CHECKIN;
        }
        return;
}


#endif /* USE_TIME */
#endif /* EXAMPLE */

#define CHK_TIME1 (chktimer1())

void show_loop_cal(VOIDFIX) {
        printf("Calibrating timer...\n");
        calibrate();
        printf("\nTimer calibrated to %lu Hz\n", loopcali);
}


/* tell link parser who we are and where we belong

1000 libznet.lib timers.obj xxLIBRARYxx

 */

