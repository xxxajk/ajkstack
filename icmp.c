/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

RFC 0792 ICMP

and packet dump routine

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "chcksum.h"
#include "in.h"
#include "netinet.h"
#include "ioconfig.h"
#include "ip.h"
#include "tcpx.h"

#if IP_ENABLED

/* len is in *OCTETS* */
void tcpdump(buffer, len)
unsigned char *buffer;
unsigned int len;
{
        register unsigned char *bufptr;
        unsigned int zipper;
        int x;
        char bleh[16];

        bufptr = buffer;
        for(zipper = 0; zipper < len; zipper++) {

                if(!(zipper & 0x0f)) {
                        if(zipper) {
                                for(x = 0; x < 16; x++) {
                                        putchar(bleh[x]);
                                }
                        }
                        printf("\n0x%4.4X\t", zipper);
                }
                printf("%2.2X%2.2X ", (bufptr[0] & 0xff), (bufptr[1] & 0xff));
                if(isprint(bufptr[0]) && (bufptr[0] > 31)) {
                        bleh[zipper & 0x0f] = bufptr[0];
                } else {
                        bleh[zipper & 0x0f] = '.';
                }
                bufptr++;
                zipper++;
                if(isprint(bufptr[0]) && (bufptr[0] > 31)) {
                        bleh[zipper & 0x0f] = bufptr[0];
                } else {
                        bleh[zipper & 0x0f] = '.';
                }
                bufptr++;
        }
        if((len & 0x01) == 0x01) {
                printf("\b\b\b   ");
        }
        while((zipper & 0x0f)) {
                printf("     ");
                bleh[zipper & 0x0f] = ' ';
                zipper++;
                bleh[zipper & 0x0f] = ' ';
                zipper++;
        }

        for(x = 0; x < 16; x++) {
                putchar(bleh[x]);
        }
        putchar('\n');
}

/* error replies RFC 0792, reuses same buffer... */
void err_reply(bufnum, type, code, pointer)
unsigned int bufnum;
unsigned int type;
unsigned int code;
int pointer;
{
        /* reply back using icmp in the same recycled buffer */
        /* ***NOTICE*** _requires_ -UNMODIFED- buffer!!! */
        /* put back *ALL ORIGINAL* checksums before calling this routine!! */
        /* DO NOT FREE THE BUFFER AFTER CALLING THIS */

        register unsigned char *buffer;
        register unsigned char *srcip;
        unsigned int i;
        unsigned int loh;
        unsigned int tl;
        unsigned int sum;

        srcip = remip + 4 * bufnum;
        buffer = bufptrs[bufnum];
        loh = (buffer[0] & 0x0f) * 4;
        tl = loh + 36;

        txrxflags[bufnum] = (FILLBUSY | ISTX | SENDNOCARE);
        for(i = 0; i < (tl - 27); i++) {
                buffer[tl - i] = buffer[tl - (i + 28)];
        }
        buffer[0] = 0x45;
        buffer[1] = 0xc0; /* tos */
        buffer[2] = ((tl >> 8) & 0xff); /* total */
        buffer[3] = (tl & 0xff);
        buffer[4] = pakid[0]; /* packet id, unique */
        buffer[5] = pakid[1];
        /* since _we_ do not fragment, ever, increment it */
        bumpid();
        buffer[6] = 0x40; /* don't fragment, please */
        buffer[8] = 0xff; /* ttl */
        buffer[9] = 0x01; /* icmp */
        buffer[7] =
                buffer[10] = /* cksum */
                buffer[11] = 0x00;
        buffer[12] = myip[0]; /* source address (me) */
        buffer[13] = myip[1];
        buffer[14] = myip[2];
        buffer[15] = myip[3];
        buffer[16] = srcip[0]; /* destination address (them) */
        buffer[17] = srcip[1];
        buffer[18] = srcip[2];
        buffer[19] = srcip[3];
        sum = checksum(buffer, (unsigned int) 20, NULL, (unsigned int) 0);
        buffer[10] = ((sum >> 8) & 0xff);
        buffer[11] = (sum & 0xff);
        buffer[20] = type & 0xff; /* type of icmp */
        buffer[21] = code & 0xff; /* error code */
        buffer[22] =
                buffer[23] =
                buffer[25] =
                buffer[26] =
                buffer[27] = 0x00;
        buffer[24] = pointer & 0xff; /* pointer to error data */
        sum = checksum(buffer + 20, tl - 20, NULL, (unsigned int) 0);
        buffer[22] = ((sum >> 8) & 0xff);
        buffer[23] = (sum & 0xff);
        rxtxpointer[bufnum] = tl;
        txrxflags[bufnum] = (READY | ISTX | SENDNOCARE);
}

#if ICMP_ENABLED

/* received ICMP RFC 0792 */
void icmp(bufnum, loh, lod)
unsigned int bufnum;
unsigned int loh;
unsigned int lod;
{
        register unsigned char *buffer;
        register unsigned char *srcip;
        unsigned char c;
        unsigned int sum;
        unsigned int bsum;

        buffer = bufptrs[bufnum];
        srcip = remip + 4 * bufnum;
        c = buffer[loh] & 0xff;
#if DEBUGI
        printf("ICMP");
#endif
        if((c == 0x08) || (c == 0x0d) || (c == 0x0f)) { /* ping timestamp info */
                bsum = ((buffer[loh + 2] & 0xff) << 8) + (buffer[loh + 3] & 0xff);
                buffer[loh + 2] = buffer[loh + 3] = 0x00;
                sum = checksum(buffer + loh, lod, NULL, (unsigned int) 0);
                if(sum == bsum) {
                        /* recycle and fill it in */
                        txrxflags[bufnum] = (FILLBUSY | ISTX);
                        buffer[8] = 0xff; /* TTL */
                        buffer[10] = /* ck */
                                buffer[11] = 0x00; /* ck */

                        buffer[12] = myip[0]; /* sip */
                        buffer[13] = myip[1]; /* sip */
                        buffer[14] = myip[2]; /* sip */
                        buffer[15] = myip[3]; /* sip */

                        buffer[16] = srcip[0]; /* dip */
                        buffer[17] = srcip[1]; /* dip */
                        buffer[18] = srcip[2]; /* dip */
                        buffer[19] = srcip[3]; /* dip */
                        sum = checksum(buffer, loh, NULL, (unsigned int) 0);
                        buffer[10] = ((sum >> 8) & 0xff);
                        buffer[11] = (sum & 0xff);

                        if(c == 0x0f) {
                                /* info reply */
                                buffer[loh] = 0x10;
                        }
                        if(c == 0x08) {
                                /* echo reply */
                                buffer[loh] = 0x00;
                        }
                        if(c == 0x0d) {
                                /* timestamp reply */
                                buffer[loh] = 0x14;
                        }

                        sum = checksum(buffer + loh, lod, NULL, (unsigned int) 0);
                        buffer[loh + 2] = (sum >> 8) & 0xff;
                        buffer[loh + 3] = sum & 0xff;
                        txrxflags[bufnum] = (READY | ISTX | SENDNOCARE);
                } else {
                        /* huh? */
                        freebuffer(bufnum);
#if DEBUGI
                        printf("UNKNOWN ICMP? %02.2x", c & 0xff);
#endif
                }
#if DEBUGI
                putchar('\n');
#endif
        } else {
                freebuffer(bufnum);
        }
}

#else /* ICMP_ENABLED */

#endif /* ICMP_ENABLED */
#else /* IP_ENABLED */

void notusingicmp(VOIDFIX) {
        return(0);
}

#endif /* IP_ENABLED */

/* tell link parser who we are and where we belong

0800 libznet.lib icmp.obj xxLIBRARYxx

 */

