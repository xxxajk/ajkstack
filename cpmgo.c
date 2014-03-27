/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

/* Startup and shut down sections.
 This code contains the FIRST CALL in your program,
 as well as the things to do at exit of your program.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "ioconfig.h"
#include "timers.h"
#include "ip.h"
#include "tcpip.h"
#include "in.h"
#include "netinet.h"
#include "socket.h"

/* waits for all network connections to complete */
void endnetwork(VOIDFIX) {
        int fd;
        unsigned char *sktp;
        int not_done;

        printf("\n\nShutting Down...\n");

        for(fd = 1; fd < BUFFERS; fd++) {
                sktp = SOKAT(fd);
                if((sktp[SOCKST(0, 0)] == (unsigned char) STATE_BOUND) || (sktp[SOCKST(0, 0)] == (unsigned char) STATE_UDP)) AJK_sockclose(fd);
        }

        for(fd = 1; fd < BUFFERS; fd++) {
                sktp = SOKAT(fd);
                if((sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED) || (sktp[SOCKST(0, 0)] == (unsigned char) STATE_CLOSING)) AJK_sockclose(fd);
        }

        not_done = BUFFERS;
        while(not_done) {
                schedulestuff();
                not_done = BUFFERS - 1;
                for(fd = 1; fd < BUFFERS; fd++) {
                        sktp = SOKAT(fd);
                        if((sktp[SOCKST(0, 0)] == (unsigned char) STATE_CLOSED) || (sktp[SOCKST(0, 0)] > (unsigned char) STATE_FINWAIT1)) {
                                not_done--;
                        } else {
                                printf("[%i %2.2x] ", fd, sktp[SOCKST(0, 0)] & 0xff);
                        }
                }
                printf("%i                     \r", not_done);
        }
}

/* extern void showalmap(void); */

int initnetwork(argc, argv)
int argc;
char **argv;
{


        GETCONFIG /* read config */


        INITINTERNALS /* init vars, hardware */


        CAL_TIME /* calibrate any timers */


#ifndef INIT_EXIT
                /* perform atexit() cleanup */
                atexit(endnetwork);
#endif

        return(0);
}


/* tell link parser who we are and where we belong

0000 libznet.lib cpmgo.obj xxLIBRARYxx

 */

