/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include "config.h"
#if NO_OS

VOIDFIX NO_OS_telnet(VOIDFIX) {
        return;
}

#else

/*
   Telnet client.
   Ping and traceroute (normal part of the stack).
   No daemons listening.
 */
/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* fixups for HI-TECH C, GCC, and probably other compilers. */
#ifdef HI_TECH_C
#include <sys.h>
#include <unixio.h>
#include <conio.h>
#else
#ifdef LINUX
#include <asm/io.h>
#endif
#endif

#include "in.h"
#include "netinet.h"
#include "socket.h"
/* various fixups, os dependent */
#include "compat.h"
#include "netutil.h"

#define PROGRAM "CPMTELNET"
#define VERSION "0.0.5"
#define EXECUTABLE "TELNET"

/* see if directive was seen, and what stage */
int ffseen = 0;
/* holding data for a telnet response */
unsigned char ffresponse[5];
struct sockaddr_in sato;

int sock = -1;

/* process the inbound queue data from a socket read */
void processbuf(buf, len)
unsigned char *buf;
int len;
{
        unsigned char *ptr;
        int spot;

        spot = 0;
        ptr = buf;

        while(spot != len) {
                if(ffseen == 1) {
                        if((*ptr > (unsigned char) 0xfa) && (*ptr != (unsigned char) 0xff)) {
                                ffresponse[1] = *ptr;
                                ffseen++;
                        } else {
                                putchar(*ptr);
                                ffseen = 0;
                        }
                } else if(ffseen == 2) {
                        if(ffresponse[1] == (unsigned char) 0xfd) {
                                ffresponse[1] = 0xfc;
                                ffresponse[2] = *ptr;
                                AJK_sockwrite(sock, ffresponse, 3);
                        }
                        ffseen = 0;
                } else {
                        if(*ptr == (unsigned char) 0xff) {
                                ffseen++;
                                ffresponse[0] = (unsigned char) 0xff;
                        } else {
                                putchar(*ptr);
                        }
                }
                ptr++;
                spot++;
        }
        fflush(stdout);
}

void usage(argv)
char *argv[];
{
#define USE "the.network.IP.address [port]"
        AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

int main(argc, argv)
int argc;
char *argv[];
{
        struct u_nameip *result;
        int zap, dotcount, smack, port, again, xkb;
        char netaddress[80];
        unsigned char destination[4];
        unsigned char iodata[81];
        unsigned char xiodata[5];


        if((argc < 2) || (argc > 3)) usage(argv);
        if(argc > 2) {
                sprintf(netaddress, "%s\n", argv[2]);
                port = atoi(netaddress) & 0xffff;
                if(port < 1) usage(argv);
        }

        zap = -1;
        dotcount = smack = 0;
        port = 23;

        /* IMPORTANT: initialize the network! */
        initnetwork(argc, argv);


        /* snag out the net address.... */
        result = findhost(argv[1], 0);
        if(!result) {
                printf("No such host\n");
        } else {

                /* take each one into a byte :-) */
                printf("\n\nTrying ");
                for(dotcount = 0; dotcount < 4; dotcount++) {
                        printf("%i.", (short) (result->hostip[dotcount] & 0xff));
                        destination[dotcount] = result->hostip[dotcount];
                }
                printf("\b (%s) port %i...", result->hostname, port);
                FREE(result->hostip);
                FREE(result->hostname);
                FREE(result);


                sock = AJK_socket(AF_INET, SOCK_STREAM, 0);
                if(sock > -1) {
                        memcpy(&sato.sin_addr, destination, 4);
                        sato.sin_port = port;
                        zap = AJK_connect(sock, (struct sockaddr_in *) &sato, sizeof(sato));
                }
        }
        if(zap > 0) {
                printf("\nConnected\n");
                iodata[0] = 0xff;
                iodata[1] = 0xfd; /* do */
                iodata[2] = 0x03; /* some thing */
                AJK_sockwrite(sock, iodata, 3); /* Supress go ahead */
        } else {
                printf("\nService Not available\n");
                sock = 0;
        }

        xkb = dotcount = 0;
        for(; sock > 0;) {
                /* the macro `MAKEMEBLEED' should be sprinkled througout the app code
                especially after a time consuming loop for latency reduction, like so:

               ( some c code, like a loop or something... )
               MAKEMEBLEED();
               ( more c code... )

               However if you are consistantly doing reads in a loop, this is done for you! :-)
               Writes _ALSO_ do this too, but that's because it's designed to and has to!
               A NULL write will cause the transport layer to hit once. SAME for a NULL read.
               THE SOCKET NEED NOT BE OPEN OR FUNCTIONING OR EVEN CORRECT.

               example:

               sockread(-1,NULL,0);
               is as valid as
               sockwrite(-1,NULL,0);

               The file descriptor number does _NOT_ matter for it to work a POLL operation.
               The latency involved will be greater, though.

               The best way to make interactive programs work better is to read and process large chunks.
               Simple loops of reading 1 from socket will cause huge amounts of cpu load and latency.

                 */
                if(sock) {
                        /* input from net */
                        zap = AJK_sockread(sock, iodata, 80);
                        if(zap < 0) {
                                sock = 0;
                        }
                        if(zap > 0) {
                                processbuf(iodata, zap);
                        }
                }

                if(sock) {
                        if(!smack) {
                                smack = 1;
                                iodata[0] = iodata[3] = 0xff;
                                iodata[1] = 0xfd; /* 253 do */
                                iodata[4] = 0xfb; /* 251 will */
                                iodata[2] = iodata[5] = 0x00;
                                AJK_sockwrite(sock, iodata, 6);
                        }
                        /* lame attempt at input from keyboard */
                        if(fixkbhit()) {
                                again = fixgetch();
                                xiodata[0] = again & 0xff;
                                zap = AJK_sockwrite(sock, &xiodata[0], 1);
                                if(xiodata[0] == 0xff) zap = AJK_sockwrite(sock, &xiodata[0], 1);
                                if(zap < 0) {
                                        sock = -1;
                                }
                        }
                }
        }
        /*
                if(!sock) {
                        printf("\n\nConection terminated\n");
                } else {
                        printf("\n\nConection terminated with an error\n");
                }
         */
#ifndef CPM
#ifndef z80
#ifndef HI_TECH_C
        exit(0);
#endif
#endif
#endif
}

#endif /* NO_OS */


/* tell link parser who we are and where we belong


0000 telnet.com telnet.obj xxPROGRAMxx

 */

