/* (C) 2001-2014 Andrew J. Kroll
 * see file README for license details
 *
 *
 *IP RFC 0791
 *
 * This is all the IP stuff
 *
 * Because this simple code is point to point initially using slip,
 * this code does not route code and does not do ARP :-)
 *
 * ARP belongs in a hardware driver.
 *
 * If you'd like to add routing, please make it a separate compile option.
 * Routing is just going to unnecessarily slow it down and bloat it on
 * smaller platforms where memory is already tight.
 *
 *      TO-DO:
 *              * Outbound IP fragments
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "tcpvars.h"
#include "chcksum.h"
#include "ip.h"
#include "netutil.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "tcpx.h"
#include "timers.h"
#include "compat.h"

#ifdef HI_TECH_C
#include <unixio.h>
#else
#include <fcntl.h>
#ifndef ARDUINO
#ifndef M5
#include <unistd.h>
#endif
#endif
#endif
#if IP_ENABLED

/* Create a standard IP header... */
unsigned int createip(buffer, ptr, type)
unsigned char *buffer;
unsigned char *ptr;
unsigned char type;
{
        /* keep 0x00 stores together to help the C optimizer */
        buffer[ 1] = 0x00;/* tos */
        buffer[ 7] =0x00;
        buffer[10] = 0x00;/* cksum */
        buffer[11] = 0x00;
        buffer[ 2] = 0x00; /* total --.  */
        buffer[ 3] = IPHEADLEN; /*         `-> length so far */
        buffer[ 9] = type; /* type */
        buffer[ 6] = 0x40; /* don't fragment, please */
        buffer[ 0] = 0x45;
        buffer[ 8] = 0xff; /* ttl */
        buffer[12] = myip[0]; /* source address (me) */
        buffer[13] = myip[1];
        buffer[14] = myip[2];
        buffer[15] = myip[3];
        buffer[16] = ptr[0]; /* destination address (them) */
        buffer[17] = ptr[1];
        buffer[18] = ptr[2];
        buffer[19] = ptr[3];
        buffer[ 4] = pakid[0]; /* packet id, unique */
        buffer[ 5] = pakid[1];
        /* since we do not fragment, ever, increment it */
        bumpid();

        /* no checksumming because we add more stuff ELSEWHERE */
        return((buffer[2] * 256) + buffer[3]); /* pointer to NEXT available data area */
}

/* decide what exactly to do with an IP packet that passes testing */
void ipdecide(bufnum, loh, lop)
unsigned int bufnum;
unsigned int loh;
unsigned int lop;
{
        register unsigned char *buffer;
        register int proto;
        int lod;

        lod = lop - loh;
        buffer = bufptrs[bufnum];
        proto = (buffer[9] & 0xff);
        switch(proto) {
                        /* what do we do with it? */
                case 0x01:
                        /* ICMP */
                        icmp(bufnum, loh, lod);
                        break;
                case 0x06:
                        /* TCP */
                        tcp(bufnum, loh, lod);
                        break;
                case 0x11:
                        /* UDP */
                        udp(bufnum, loh, lop, lod);
                        break;
                default:
                        /* huh? */
                        /* type 3, code 2 */
                        err_reply(bufnum, 3, 2, 0);
                        break;
        } /* decision complete */
}

#if HANDLEFRAGS

/* find matching fragment */
int findfrag(bufnum)
unsigned int bufnum;
{

        register unsigned char *a;
        register unsigned char *b;
        unsigned int i;

        /*
         match bytes 4,5,9,  12,13,14,15, 16,17,18,19
         and not ourself.
         */
        a = bufptrs[bufnum];

        for(i = 0; i < BUFFERS; i++) {
                b = bufptrs[i];
                if(b) {
                        if((i != bufnum) && ((b[6] & FRAGFLAG) == FRAGFLAG)) {
                                /*
                                We don't compare on our self or non frags,
                                christ this is some ugly shit...
                                Atleast the compiler won't screw up...
                                 */
                                if((a[4] == b[4]) && (a[5] == b[5])) {
                                        if((a[9] == b[9]) && (a[12] == b[12])) {
                                                if((a[13] == b[13]) && (a[14] == b[14])) {
                                                        if((a[15] == b[15]) && (a[16] == b[16])) {
                                                                if((a[17] == b[17]) && (a[18] == b[18]) && (a[19] == b[19])) {
                                                                        return(i);
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        return(-1);
}

/*

Check fragments and coalesce them into one buffer on each pass.
Completed buffers are marked as SWALLOWED for the final tcp pass.

Incomplete buffers which get the end frag mark will fail the
checksum in an upper layer and get discarded automatically.

 */

void coalescefrags(bufnum)
unsigned int bufnum;
{
        /*
        If the ID is not in the list,
        tag this buffer as the id, and exit
        this doesn't have to eat any space to do so.
        An easier more effective way is to simply compare
        the needed bits, and coalesce on match.
         */

        register unsigned char *inbuffer;
        register unsigned char *build;
        register unsigned char *frag;
        unsigned int it, was, this_one, yes;
        int here;

        inbuffer = bufptrs[bufnum];

        /* new package? */
        if(((inbuffer[6] & FRAGFLAG) == FRAGFLAG) && (!(inbuffer[6] & FLAGMASK)) && (!inbuffer[7])) {
                /* new fragment */
                here = findfrag(bufnum);
                /* has a new reassembly started? */
                if(here != -1) {
                        /* yea, discard the old one */
                        freebuffer((unsigned int) here);
                }
                /*
                        Is the to-be assembled size acceptable?
                 */
                it = (inbuffer[3] + (inbuffer[2] << 8)) - ((inbuffer[0] & 0x0f) * 4); /* packet length - header */
                if(it > MAXTAILR) {
                        /* no, ignore it */
                        freebuffer(bufnum);
                } else {
                        /* Yes, realloc buffer to maximum rwindow + max header size */
                        inbuffer = REALLOC(bufptrs[bufnum], MAXTAILR + 68);
                        if(inbuffer) {
                                bufptrs[bufnum] = inbuffer;
                        } else {
                                /* we've ran out of ram, don't pollute our arena, free it up */
                                freebuffer(bufnum);
                                /* perhaps on the next go, we'll have the resources :-( */
                        }
                }
                /* done with initial fragment */
                return;
        }
        /*
         Not a new one,
         Find alike previous package...
         */
        here = findfrag(bufnum);
        if(here != -1) {
                /* offset sane? */
                was = (inbuffer[0] & 0x0f) * 4; /* header len */
                it = (inbuffer[3] + (inbuffer[2] << 8)) - was; /* packet length - header */
                this_one = (inbuffer[7] + ((inbuffer[6] & FLAGMASK) << 8)) * 8; /* fragment offset */
                if((it + this_one) <= (unsigned int) MAXTAILR) {
                        /* yes, copy the data */
                        build = bufptrs[(unsigned int) here];
                        yes = (build[0] & 0x0f) * 4;
                        frag = bufptrs[(unsigned int) here] + this_one + yes;
                        build = inbuffer + was;
                        memmove(frag, build, it);
                        /* last one? */
                        if(!(inbuffer[6] & FRAGFLAG)) {
                                /* yes, fixup tl */
                                build = bufptrs[(unsigned int) here];
                                was = it + this_one + yes; /* total length */
                                build[2] = (char) ((was / 256) & 0xff);
                                build[3] = (char) (was & 0xff);
                                /* switch buffer status to swallowed */
                                txrxflags[(unsigned int) here] = SWALLOWED;
                                /* process coalesced packet, see above :-) */
                                /* tcpdump(build, was); */
                                ipdecide((unsigned int) here, yes, was);
                        } else {
                                /* no, put time back up */
                                fragtime[(unsigned int) here] = CHK_TIME1 + (loopcali * STARTFRAGTIME);
                        }
                }
        }
        freebuffer(bufnum);
}

#endif /* HANDLEFRAGS */

/* process a recieved IP datagram, part 1 */
void ip(bufnum)
unsigned int bufnum;
{
        register unsigned char *buffer;
        register unsigned char *srcip;
        unsigned char destip[4];
        unsigned char ttl;
        unsigned int sum;
        unsigned int bsum;
        unsigned int loh;
        unsigned int lop;

#if HANDLEFRAGS
        /*
                Short circuit path, because this buffer
                already was processed and passed IP checks.
         */
        if(txrxflags[bufnum] == FRAGMENT) {
                /* check the timer, if zero, kill the buffer */
                if(fragtime[bufnum] == CHK_TIME1) {
                        freebuffer(bufnum);
                }
                return; /* complete quick short circuit path */
        }
#endif
        buffer = bufptrs[bufnum];
        srcip = remip + 4 * bufnum;
        if((buffer[0] & 0xf0) == 0x70) buffer[0] = 0x40 | (buffer[0] & 0xf);
        if((buffer[0] & 0xf0) == 0x40) {
                /* valid IPv4 SOH... */
                loh = (buffer[0] & 0x0f) * 4; /* length in octets */
                /* tos=buffer[1];  ignore this -- type of service */
                lop = buffer[3] + (buffer[2] << 8); /* packet length */
                ttl = buffer[8];
                srcip[0] = buffer[12];
                srcip[1] = buffer[13];
                srcip[2] = buffer[14];
                srcip[3] = buffer[15];

                destip[0] = buffer[16];
                destip[1] = buffer[17];
                destip[2] = buffer[18];
                destip[3] = buffer[19];

                /*if (dodump) tcpdump(buffer, loh);*/
                bsum = (buffer[10] << 8) + (buffer[11]);

                buffer[10] = buffer[11] = 0x00;
                sum = checksum(buffer, loh, NULL, (unsigned int) 0);
                buffer[10] = ((bsum >> 8) & 0xff);
                buffer[11] = (bsum & 0xff);
                if(bsum == sum) {
                        /* checksum on header passes. */
                        if(ttl > 0) { /* TTL looks good. */

                                /* note we only check for OUR IP */
                                if((myip[0] == destip[0]) && (myip[1] == destip[1]) && (myip[2] == destip[2]) && (myip[3] == destip[3])) {
                                        /* this package is for us! */

#if HANDLEFRAGS
                                        /* fragment? */
                                        if(((buffer[6] & FRAGFLAG) == FRAGFLAG) || (((buffer[6] & FLAGMASK)) || (buffer[7]))) {
                                                /* yea, coalescee them if we can, and exit */
                                                txrxflags[bufnum] = FRAGMENT;
                                                fragtime[bufnum] = CHK_TIME1 + (loopcali * STARTFRAGTIME);
                                                coalescefrags(bufnum);
                                                return;
                                        }
                                        /* nope.... */
#endif
                                        ipdecide(bufnum, loh, lop);
                                } else {
                                        /*
                                        Else address not ours
                                        this stack can't route! But do I care
                                        to really do that on a CP/M box?
                                        No, Not today! :-)

                                        type 3, code 1
                                         */
                                        err_reply(bufnum, 3, 1, 0);
                                }
                        } else {
                                /* else ttl is dead */
                                /* type 11, code 0 */
                                err_reply(bufnum, 11, 0, 0);
                        }
                } else {
                        /* else bad checksum */
                        freebuffer(bufnum);
                }
        } else {
                /* else ignore it :-) it's not ipv4 */
                freebuffer(bufnum);
        }
}

char rcfile[] = CONFIGFILE;

void readhostip(VOIDFIX) {
        register char *inputbuf;
        register int fd;
        int xipa, xipb, xipc, xipd, xipe, dipa, dipb, dipc, dipd, gatea, gateb, gatec, gated;

        xipa = xipb = xipc = xipd = xipe = dipa = dipb = dipc = dipd = gatea = gateb = gatec = gated = -1;
        hostname[0] = 0x00;

        fd = open(rcfile, (int) 0);
        if(fd < 0) {
                printf("\nERROR:`%s` config file does not exist.\nERROR: Please create it!\n", rcfile);
                exit(2);
        }

        inputbuf = safemalloc(513);
        xipa = read(fd, inputbuf, 512);
        sscanf(inputbuf, "%s %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i", hostname, &xipb, &xipc, &xipd, &xipe, &dipa, &dipb, &dipc, &dipd, &gatea, &gateb, &gatec, &gated);
        if(xipa < 9 || dipd == -1 || xipe == -1 || !hostname[0]) {
                printf("ERROR: Missing data in configfile `%s`.\n%i\n", rcfile, xipa);
#ifdef ARDUINO
                for(;;);
#else
                exit(3);
#endif
        }
        FREE(inputbuf);
        close(fd);
        myip[0] = xipb & 0xff;
        myip[1] = xipc & 0xff;
        myip[2] = xipd & 0xff;
        myip[3] = xipe & 0xff;
        mydns[0] = dipa & 0xff;
        mydns[1] = dipb & 0xff;
        mydns[2] = dipc & 0xff;
        mydns[3] = dipd & 0xff;
        mygate[0] = gatea & 0xff;
        mygate[1] = gateb & 0xff;
        mygate[2] = gatec & 0xff;
        mygate[3] = gated & 0xff;
}

#if HANDLEFRAGS

/* process the saved tcpip buffers (fragment timers) */
void dotcpip(VOIDFIX) {
        register unsigned int i;

        for(i = 0; i < BUFFERS; i++) {
                if((txrxflags[i] & (SWALLOWED | FRAGMENT))) {
                        /* we have something to do */
                        ip(i);
                }
        }
}

#endif

#else /* USE_IP */

void notusingip(VOIDFIX) {
        return(0);
}
#endif /* USE_IP */

/* tell link parser who we are and where we belong

0500 libznet.lib ip.obj xxLIBRARYxx

 */

