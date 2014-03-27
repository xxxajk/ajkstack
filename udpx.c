/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

UDP RFC 0768

Pre-optimized for the compiler via
keeping 0x00 stores together and other tricks to help the C optimizer
I know this makes it harder to follow, but it's worth it! HONEST!

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
#include "udpx.h"
#include "compat.h"
#include "ip.h"


#ifdef UDP_ENABLED
#define DEBUGU 0

unsigned int createudp(buffer, ptr, datalen, pdata, sktp)
unsigned char *buffer;
unsigned char *ptr;
int datalen;
unsigned char *pdata;
unsigned char *sktp;
{
        register unsigned int bar; /* current pointer */
        register unsigned int y; /* length of minimum header in octets */

        bar = (buffer[2] * 256) + buffer[3]; /* current pointer */
        y = 8; /* length of minimum header in octets */


        /* src port */
        buffer[bar ] = sktp[SRCPRT(0, 0)];
        buffer[bar + 1] = sktp[SRCPRT(0, 1)];
        /* dest port */
        buffer[bar + 2] = ptr[ 1];
        buffer[bar + 3] = ptr[ 0];
        /* checksum */
        buffer[bar + 6] = buffer[bar + 7] = 0x00;

        /* attach data */
        memmove(buffer + bar + y, pdata, datalen);

        /* add to octet counts */
        y += datalen;
        /* length */
        buffer[bar + 4] = (y / 256) & 0xff;
        buffer[bar + 5] = y & 0xff;
        y += bar;
        buffer[2] = (y / 256) & 0xff;
        buffer[3] = y & 0xff;

        return(y);
}

void udpsumming(buffer)
unsigned char *buffer;
{
        register unsigned int sum;
        register unsigned int usum;
        unsigned int iplen;
        unsigned int udplen;
        unsigned int totallen;
        unsigned char s1;
        unsigned char s2;

        totallen = (buffer[2] * 256) + buffer[3];
        iplen = (buffer[0] & 0x0f) * 4;
        udplen = totallen - iplen;

        /* checksum ip */
        sum = checksum(buffer, iplen, NULL, (unsigned int) 0);

        /* checksum udp */
        s1 = buffer[8]; /* save */
        s2 = buffer[9]; /* these */

        buffer[8] = 0x00; /* protocol number... */
        buffer[9] = 0x11; /* is 17 */

        buffer[10] = (unsigned char) ((udplen >> 8) & 0x00ff);
        buffer[11] = (unsigned char) (udplen & 0x00ff);

        buffer[iplen + 6] = 0x00; /* zero */
        buffer[iplen + 7] = 0x00; /* out */

        usum = checksum(&buffer[iplen], udplen, &buffer[8], 12);

        if(!usum) usum = 0xffff; /* the fun exception... */
        buffer[8] = s1; /* restore */
        buffer[9] = s2; /* these */

        /* lay in the checksums */
        buffer[10] = (sum / 256) & 0xff;
        buffer[11] = sum & 0xff;
        buffer[iplen + 6] = (usum / 256) & 0xff;
        buffer[iplen + 7] = usum & 0xff;
}

#else

void udpxnotused(VOIDFIX) {
        return(0);
}
#endif /* UDP_ENABLED */

/* tell link parser who we are and where we belong

02FF libznet.lib udpx.obj xxLIBRARYxx

 */

