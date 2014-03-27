/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

TCP RFC 0793
Pre-optimized for the compiler via
keeping 0x00 stores together and other tricks to help the C optimizer
I know this makes it harder to follow, but it's worth it! HONEST!

Unfortunately writes are commited instantly due to small TPA constraints.
This may change in the future.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "in.h"
#include "netinet.h"
#include "socket.h"
#include "ioconfig.h"
#include "chcksum.h"
#include "timers.h"
#include "tcpx.h"
#include "compat.h"
#include "ip.h"

#ifdef TCP_ENABLED
#define DEBUGI 0
#define DEBUGOPTIONS 0
#define DEBUGMOVE 0
#define TCPM 0

void bumpid(VOIDFIX) {
        /* increment unique global outbound packet id */
        inc16word(pakid);
}

void freebuffer(bufnum)
int bufnum;
{
        FREE(bufptrs[bufnum]);
        bufptrs[bufnum] = NULL;
        txrxflags[bufnum] = 0x00;
}

unsigned int createtcp(buffer, ptr, datalen, pdata)
unsigned char *buffer;
unsigned char *ptr;
int datalen;
unsigned char *pdata;
{
        unsigned int x;
        unsigned int y; /* length of standard header in octets */
        unsigned int bar;

        y = STANDARDHEADER;
        bar = (buffer[2] * 256) + buffer[3];
        /*
                Only send MSS hints if we have room in the packet.
                the 4 option bytes for MSS will get overwritten with data.
         */
        x = bar + datalen + y;
        if((x) > MAXMTUMRU) {
                y = y - MSSOPTION; /* no MSS */
                x = bar + datalen + y;
        }
        /* set mss, MRU - header(40 or 36) */
        buffer[bar + 23] = ptr[SOKMSS(0, 1)]; /* mssLSB */
        buffer[bar + 22] = ptr[SOKMSS(0, 0)]; /* mssMSB */
        buffer[bar + 21] = 0x04;
        buffer[bar + 20] = 0x02;

        buffer[bar + 19] = buffer[bar + 18] = /* urgent? nahhh none of our data is urgent yet :-) */
                buffer[bar + 17] = /* checksum */
                buffer[bar + 16] = 0x00;
        buffer[bar + 15] = ptr[WINDOW(0, 1)]; /* window */
        buffer[bar + 14] = ptr[WINDOW(0, 0)]; /* window */
        buffer[bar + 13] = ptr[BUFSTA(0, 0)]; /* flags */
        buffer[bar + 12] = ((y / 4) & 0x0f) * 16; /* data offset in words << 4 */
        /* ack # */
        buffer[bar + 11] = ptr[ACKNUM(0, 3)];
        buffer[bar + 10] = ptr[ACKNUM(0, 2)];
        buffer[bar + 9] = ptr[ACKNUM(0, 1)];
        buffer[bar + 8] = ptr[ACKNUM(0, 0)];
        /* seq # */
        buffer[bar + 7] = ptr[SEQNUM(0, 3)];
        buffer[bar + 6] = ptr[SEQNUM(0, 2)];
        buffer[bar + 5] = ptr[SEQNUM(0, 1)];
        buffer[bar + 4] = ptr[SEQNUM(0, 0)];
        /* dest port */
        buffer[bar + 3] = ptr[DESPRT(0, 1)];
        buffer[bar + 2] = ptr[DESPRT(0, 0)];
        /* src port */
        buffer[bar + 1] = ptr[SRCPRT(0, 1)];
        buffer[bar ] = ptr[SRCPRT(0, 0)];


        /* attach data */
        memmove(buffer + (bar + y), pdata, datalen);

        /* add to octet counts */
        buffer[2] = (x / 256) & 0xff;
        buffer[3] = x & 0xff;

        return(datalen + y);
}

void tcpsumming(buffer)
unsigned char *buffer;
{
        unsigned int sum;
        unsigned int iplen;
        unsigned int tcplen;
        unsigned int totallen;
        unsigned char fakebuf[13];

        totallen = (buffer[2] * 256) + buffer[3];
        iplen = (buffer[0] & 0x0f) * 4;
        tcplen = totallen - iplen;

        fakebuf[0] = buffer[12];
        fakebuf[1] = buffer[13];
        fakebuf[2] = buffer[14];
        fakebuf[3] = buffer[15];

        fakebuf[4] = buffer[16];
        fakebuf[5] = buffer[17];
        fakebuf[6] = buffer[18];
        fakebuf[7] = buffer[19];

        fakebuf[8] = 0x00;
        fakebuf[9] = 0x06;
        fakebuf[10] = (tcplen / 256) & 0xff;
        fakebuf[11] = tcplen & 0xff;

        /* checksum ip */
        sum = checksum(buffer, iplen, NULL, (unsigned int) 0);
        buffer[10] = (sum / 256) & 0xff;
        buffer[11] = sum & 0xff;

        /* checksum tcp */
        sum = checksum(&buffer[iplen], tcplen, fakebuf, 12);

        buffer[iplen + 16] = (sum / 256) & 0xff;

        buffer[iplen + 17] = sum & 0xff;

}

/* build a fast reply */
int fasttcppack(fd, flagstype, state, datlen, datptr, extraflags)
int fd;
int flagstype;
int state;
int datlen;
unsigned char *datptr;
unsigned int extraflags;
{
        unsigned char *ptr;
        unsigned char *buffer;
        unsigned int hi;
        unsigned int ht;
        int i, freebuf;

        freebuf = -1;

        /*
        If we're in the established state, let's calculate the winsize.
        Hopefully atleast the local lan remote won't frag!
         */
        ptr = SOKAT(fd);
        /*
                This was used to catch the annoying RST bug on listening sockets.
                if(!(ptr[0] | ptr[1] | ptr[2] | ptr[3])) {
                        printf("\nZero address on socket %i, state %i->%i\n", fd, ptr[SOCKST(0, 0)] & 0xff, state);
                        exit(1);
                }
         */
        /* state change */
        ptr[BUFSTA(0, 0)] = flagstype & 0xff;
        ptr[SOCKST(0, 0)] = state & 0xff;

        if(!(ptr[0] | ptr[1] | ptr[2] | ptr[3])) {
                printf("\nSTACK ERROR\n\007Zero address on socket %i\n", fd);
                exit(1);
        }

        if(state == STATE_ESTABLISHED) {
                /* how much buffer left? */
                ptr[WINDOW(0, 1)] = (MAXTAILR - rbufcount[fd]) & 0xff;
                ptr[WINDOW(0, 0)] = ((MAXTAILR - rbufcount[fd]) / 256) & 0xff;
        }
        /* build an ip packet */
        for(i = 1; i < BUFFERS; i++) {
                if(!(txrxflags[i] & 0xff)) freebuf = i;
        }

        if(freebuf < 0) return(-1); /* out of buffers */

        /* malloc the buffer */
        bufptrs[freebuf] = MALLOC(datlen + STANDARDHEADER + IPHEADLEN);
        if(!bufptrs[freebuf]) {
                /* we're screwed... */
#if TCPM
                printf("tcpx.c: %i No RAM\n", __LINE__);
#endif
                return(-1);
        }
        txrxflags[freebuf] = (ISTX | FILLBUSY); /* lock the buffer */

        /* lay out ip */
        buffer = bufptrs[freebuf];
        hi = createip(buffer, ptr, 0x06);

        /* lay out tcp */
        buffer = bufptrs[freebuf];
        ht = createtcp(buffer, ptr, datlen, datptr);

        /* calculate sums */
        buffer = bufptrs[freebuf];
        tcpsumming(buffer);

        /* set the size of the entire package */
        rxtxpointer[freebuf] = hi + ht;

        /* toss it in the tx que */
        txrxflags[freebuf] = (ISTX | READY | extraflags) & 0xff;
        return(fd);
}

/* open */
int tcp_open(fd, remote, ip)
int fd;
unsigned int remote;
unsigned char *ip;
{
        register unsigned char *sktp;
        unsigned char *ptr;
        int i;
#if DEBUGI
        printf("Remote: %i\n", remote);
#endif
        ptr = SOKAT(fd);
        /* memset(ptr, 0, SOKDAT); */
        /* lay in the data */
        /* dest ip */
        ptr[IPDEST(0, 0)] = ip[0];
        ptr[IPDEST(0, 1)] = ip[1];
        ptr[IPDEST(0, 2)] = ip[2];
        ptr[IPDEST(0, 3)] = ip[3];
newport:

        /* src port start at 32768 and work up */
        /* is current port available? */
        for(i = 0; i < SOKALLOW; i++) {
                sktp = SOKAT(i);
                if((sktp[SRCPRT(0, 0)] == ((portcounter / 256) & 0xff)) && (sktp[SRCPRT(0, 1)] == (portcounter & 0xff))) {
                        portcounter++;
                        if(portcounter > 65000) portcounter = 32768;
                        goto newport;
                }

        }

        ptr[SRCPRT(0, 0)] = (portcounter / 256) & 0xff;
        ptr[SRCPRT(0, 1)] = portcounter & 0xff;
        portcounter++;

        if(portcounter > 65000) portcounter = 32768;
        /* dest port */
        ptr[DESPRT(0, 0)] = (remote >> 8) & 0xff;
        ptr[DESPRT(0, 1)] = remote & 0xff;
        /* initial seq/ack */
        sav32word(&ptr[SEQNUM(0, 0)], isnclock);
        sav32word(&ptr[ACKNUM(0, 0)], isnclock);

        /* window 1 buffersize */

        /* lets nuke local frags! */
        ptr[WINDOW(0, 0)] = ((MAXPKTSZ) / 256) & 0xff;
        ;
        ptr[WINDOW(0, 1)] = (MAXPKTSZ) & 0xff;

        /* mss */
        ptr[SOKMSS(0, 0)] = /* (DEFAULTMSS / 256) & 0xff; */ ptr[WINDOW(0, 0)];
        ptr[SOKMSS(0, 1)] = /* DEFAULTMSS & 0xff; */ ptr[WINDOW(0, 1)];

        /* fd */
        ptr[SOCKFD(0, 0)] = fd & 0xff;

        tcp_syn(fd);
        /* sock state */
        timertx[fd] = CHK_TIME1 + (loopcali * RETRYTIME);
        sockexpecting[fd] = 1;
        return(fd);
}

int matchsocket(port)
unsigned char *port;
{
        register unsigned char *sktp;
        int i;
        int match;

        match = -1;

        for(i = SOKALLOW - 1; i > 0; i--) {
                sktp = SOKAT(i);
#if DEBUGI
                printf("Socket: %i, State %i\n"
                        "%1.1x=%1.1x %1.1x=%1.1x\n"
                        "%1.1x=%1.1x %1.1x=%1.1x\n", i, sktp[SOCKST(0, 0)],
                        port[0], sktp[DESPRT(0, 0)],
                        port[1], sktp[DESPRT(0, 1)],
                        port[2], sktp[SRCPRT(0, 0)],
                        port[3], sktp[SRCPRT(0, 1)]);
#endif
                if(sktp[SOCKST(0, 0)] > STATE_NOTINIT) {
                        /* is socket active... */
                        if((port[0] == sktp[DESPRT(0, 0)]) && (port[1] == sktp[DESPRT(0, 1)])) {
                                if((port[2] == sktp[SRCPRT(0, 0)]) && (port[3] == sktp[SRCPRT(0, 1)])) {
#if DEBUGI
                                        printf("Exact Match %i\n", i);
#endif
                                        return(i); /* exact match */
                                }
                        }
                }

                if((((sktp[SOCKST(0, 0)] & 0xff) == STATE_LISTEN)
#if UDP_ENABLED
                        || ((sktp[SOCKST(0, 0)] & 0xff) == STATE_UDP)
#endif
                        ) && match == -1) {
                        /* is socket passive... */
                        if((port[2] == sktp[SRCPRT(0, 0)]) && (port[3] == sktp[SRCPRT(0, 1)])) {
                                match = i; /* "best fit" so far */
#if DEBUGI
                                printf("Socket: %i, State %i\n"
                                        "%1.1x=%1.1x %1.1x=%1.1x\n"
                                        "%1.1x=%1.1x %1.1x=%1.1x\n", i, sktp[SOCKST(0, 0)],
                                        port[0], sktp[DESPRT(0, 0)],
                                        port[1], sktp[DESPRT(0, 1)],
                                        port[2], sktp[SRCPRT(0, 0)],
                                        port[3], sktp[SRCPRT(0, 1)]);
#endif
                        }
                }
        }
#if DEBUGI
        printf("Fuzzy Match %i\n", match);
#endif
        return(match);
}

/* check ack match seq */
int checkack(ours, thiers)
unsigned char *ours;
unsigned char *thiers;
{
        if((((ours[0] == thiers[0]) && (ours[1] == thiers[1])) && ((ours[2] == thiers[2]) && (ours[3] == thiers[3])))) return(1);
        return(0);
}

void checkoptions(buffer, sock, iplen, tcplen)
unsigned char *buffer;
int sock;
int iplen;
int tcplen;
{
        register unsigned char *sktp;
        unsigned char *thiers;
        int width;

#if DEBUGOPTIONS
        int i;

        printf("OPTIONS { ");
#endif
        sktp = SOKAT(sock);
        /* if the remote mss is less, copy it */
        thiers = buffer + iplen + 20; /* options block, pick out the mss option */
        for(width = 0; width < tcplen - 20;) {
                if(!thiers[width]) { /* end options */
#if DEBUGOPTIONS
                        printf("END ");
#endif
                        width = tcplen;
                } else if(thiers[width] == 0x01) { /* NOP */
#if DEBUGOPTIONS
                        printf("NOP ");
#endif
                        width++;
                } else if(thiers[width] == 0x02) { /* MSS */
#if DEBUGOPTIONS
                        printf("MSS %i ", (thiers[width + 2] * 256) + thiers[width + 3]);
#endif
                        if(((sktp[SOKMSS(0, 0)] * 256) + sktp[SOKMSS(0, 1)]) > ((thiers[width + 2] * 256) + thiers[width + 3])) {
                                /* thier mss is smaller! */
                                sktp[SOKMSS(0, 0)] = thiers[width + 2];
                                sktp[SOKMSS(0, 1)] = thiers[width + 3];
                        }
                        width += thiers[width + 1];
                } else { /* unknown option... emmm lessee.... */
#if DEBUGOPTIONS
                        printf("OPTION ???: [ ");
                        for(i = 0; i < (thiers[width + 1] & 0xff); i++) printf("%2.2x ", thiers[width + i]);
                        putchar(']');
#endif
                        width += thiers[width + 1];
                }
        }
#if DEBUGOPTIONS
        printf("} ");
#endif
}

void bufferdatamove(databuffer, sock, datalen)
unsigned char *databuffer;
unsigned int sock;
unsigned int datalen;
{
        unsigned int width;
        unsigned char *bleh;
        unsigned char *there;
        bleh = databuffer;
        there = rdatapointers[sock];
#if DEBUGMOVE
        printf("INBOUND DATA Socket %i, %i Bytes [ ", sock, datalen);
#endif
        for(width = 0; width < datalen; width++) {
                there[rbufhead[sock]] = *bleh++;
#if DEBUGMOVE
                printf("%2.2x ", databuffer[width] & 0xff);
#endif
                rbufcount[sock]++;
                rbufhead[sock]++;
                if(rbufhead[sock] == MAXTAILR) rbufhead[sock] = 0;
        }
#if DEBUGMOVE
        printf("]\n");
#endif
}


#else

void tcpxnotused(VOIDFIX) {
        return(0);
}
#endif

/* tell link parser who we are and where we belong

0300 libznet.lib tcpx.obj xxLIBRARYxx

 */

