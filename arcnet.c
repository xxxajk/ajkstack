/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

arcnet.c
com90c66 revision D arcnet driver

1 space for transmit
3 spaces for recieve

RFC 1051 "SIMPLE" protocol (no exception packets)

F0 for IP packet
F1 for ARP packet
   ARCNET supports packet formats containing 1-253 octets of data
   (normal format) and 257-508 octets of data (extended format),
   inclusive of system code.  Note that there exists a range of data
   lengths (254-256) which are 'forbidden'.  IP packets within this
   range should be padded (with octets of zero) to meet the minimum
   extended packet size of 257 data octets.  This padding is not part of
   the IP packet and is not included in the total length field of the IP
   header.


 ***Driver	07	arcnet
 */

#define DEBUGARC	1
#define DUMPS		0
#define DOTESTS		0
#define MANUAL 		1 /* use manual incrementors */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "tcpvars.h"
#include "ioconfig.h"

#if USEARCNET /* Include this code? */
#include "ip.h"
#include "net.h"
#include "arcnet.h"
#include "tcpx.h"
#include "compat.h"

#ifdef CPM
#include <cpm.h>
#endif
#include <conio.h>


#define ARCPORT 0xDEA0
/* read registers */
#define RSTAT	(ARCPORT)
#define RDIAG	(ARCPORT + 0x01)
#define RCONF	(ARCPORT + 0x02)
#define RIOSE	(ARCPORT + 0x03)
#define RMEMS	(ARCPORT + 0x04)
#define RNID	(ARCPORT + 0x05)
#define RRESET	(ARCPORT + 0x08)
#define RDLOW	(ARCPORT + 0x0C)
#define RDHI	(ARCPORT + 0x0D)
#define RALOW	(ARCPORT + 0x0E)
#define RAHI	(ARCPORT + 0x0F)

/* write registers */
#define WMASK	(ARCPORT)
#define WCOM	(ARCPORT + 0x01)
#define WCONF	(ARCPORT + 0x02)
#define WNID	(ARCPORT + 0x05)
#define WEXT	(ARCPORT + 0x07)
#define WRESET	(ARCPORT + 0x08)
#define WDLOW	(ARCPORT + 0x0C)
#define WDHI	(ARCPORT + 0x0D)
#define WALOW	(ARCPORT + 0x0E)
#define WAHI	(ARCPORT + 0x0F)

struct arcarp {
        unsigned char ar_hrdH; /* format of hardware address	*/
        unsigned char ar_hrdL; /* 	''		''	*/
        unsigned char ar_proH; /* format of protocol address	*/
        unsigned char ar_proL; /*	''		''	*/
        unsigned char ar_hln; /* length of hardware address	*/
        unsigned char ar_pln; /* length of protocol address	*/
        unsigned char ar_opH; /* ARP opcode (command)		*/
        unsigned char ar_opL; /*	''		''	*/
        unsigned char ar_sha; /* sender hardware address	*/
        unsigned char ar_sip[4]; /* sender IP address		*/
        unsigned char ar_tha; /* target hardware address	*/
        unsigned char ar_tip[4]; /* target IP address		*/
};

/* this should do the trick to buffer enough, if not, to bad :-) */
char HARDDRV[] = "POLLED 90C66D driver Version 0.0.1";
/* our node ID */
unsigned char nid = 0; /* our node ID */
unsigned char did = 0; /* where we send ALL our packets */
/* address pointers helpers */
int adh;
int adl;

int fixgetch(VOIDFIX) {
        return(getch());
}

int fixkbhit(VOIDFIX) {
        return(kbhit());
}

void setadd(VOIDFIX) {
#if MANUAL
        outp(WAHI, adh);
#else
        outp(WAHI, adh | 0x40);
#endif
        outp(WALOW, adl);
}

int getbuf(VOIDFIX) {
#if MANUAL
        int a;
#if 1
        int b;

        a = 1;
        b = 2;
        while(a != b) {

                setadd();
                a = inp(RDLOW);
                b = inp(RDLOW);
        }
#else
        setadd();
        a = inp(RDLOW);
#endif
        adl = (adl + 1) & 0xff;
        if(!adl) {
                adh = (adh + 1) & 0x07;
        }

        return(a);
#else
        return(inp(RDLOW));
#endif
}

void putbuf(data)
int data;
{
#if MANUAL
#if 0
        unsigned char a;
        unsigned char b;
        a = data + 1;
        while(a != data) {
                setadd();
                outp(WDLOW, data);
                b = a + 1;
                while(a != b) {
                        a = inp(RDLOW);
                        b = inp(RDLOW);
                }
        }
#else
        setadd();
        outp(WDLOW, data);
#endif

        adl = (adl + 1) & 0xff;
        if(!adl) {
                adh = (adh + 1) & 0x07;
        }
#else
        outp(WDLOW, data);
#endif
}

#if DUMPS

/* clear the rx area of ram */
void clearram(VOIDFIX) {
        int j;

        adh = adl = 0;
        setadd();
        for(j = 0; j < 1024; j++) {
                putbuf(0x00);
        }
}
#endif

void txok(VOIDFIX) {
        int i;

        i = 0;
        while(!(inp(RSTAT) & 1)) {
                /* wait for transmitter, but only for so long */
                i++;
                if(i > 32000) outp(WCOM, 0x01); /* send cancel */
        }
}

/* send packet */
void arctx(bufnum) {
        register unsigned int i;
        unsigned char *buf;
        unsigned char a;
        unsigned char b;
        unsigned char os;

        a = b = 0x00;
        if(did) {
                txok();
                txrxflags[bufnum] = ((txrxflags[bufnum] & (~READY)) | BUSY) & 0xff;
                adh = 6;
                adl = 0; /* buffer 3 is used for tx */
                setadd();

                putbuf(nid);
                putbuf(did);

                i = rxtxpointer[bufnum];
#if 0
                printf("Transmitting IP packet %i bytes\n", i);
#endif
                i++; /* add one for type */
                if(i < 253) {
                        os = (256 - i) & 0xff;
                        a = os;
                } else {
                        if(i < 257) {
                                /* force packet to be atleast 257 bytes */
                                i = i + 4;
                        }
                        os = (512 - i) & 0xff;
                        b = os;
                }
                putbuf(a);
                putbuf(b);

                adl = os;
                setadd();
                putbuf(0xf0);
                buf = bufptrs[bufnum];
                for(i = 0; i < rxtxpointer[bufnum]; i++) {
                        putbuf(*buf);
#if 0
                        printf("%2.2x ", *buf);
#endif
                        buf++;
                }
#if 0
                printf("\n");
#endif
                i = rxtxpointer[bufnum];
                if(i > 252) {
                        if(i < 257) {
                                /* pad it */
                                for(i = 0; i < 4; i++) putbuf(0x00);
                        }
                }
                outp(WCOM, 0x1b); /* fire, and forget */
        }
        freebuffer(bufnum);
}

/* process inbound arp packet */
void proc_arp(bufnum, i)
int bufnum;
int i;
{


        /*
        should always be a short packet of 0x12 bytes

        0102 0304 05 06 0708 09 0a0b0c0d 0e 0f101112
        hrd  pro  hl pl  op  sh sourceip th targetip comments
        0007 0800 01 04 0001 01 c0a80301 00 c0a8038d request (who has {0001})
        0007 0800 01 04 0002 8d c0a8038d 01 c0a80301 reply (is at {0002})

         */
        struct arcarp *pack;
        unsigned char *buf;

        buf = bufptrs[bufnum];
        buf += i;
        pack = (struct arcarp *) buf;
        buf = bufptrs[bufnum];
#if 0
        printf("Arp packet %2.2x%2.2x %2.2x%2.2x %2.2x%2.2x %2.2x%2.2x\n",
                pack->ar_hrdH, pack->ar_hrdL, pack->ar_proH, pack->ar_proL,
                pack->ar_hln, pack->ar_pln, pack->ar_opH, pack->ar_opL);
#endif
        if(pack->ar_hrdH) return; /* not arcnet */
        if(pack->ar_hrdL != 0x07) return; /* not arcnet */
        if(pack->ar_proH != 0x08) return; /* not IP */
        if(pack->ar_proL) return; /* not IP */
        if(pack->ar_hln != 0x01) return; /* wrong length */
        if(pack->ar_pln != 0x04) return; /* wrong length */
        if(pack->ar_opH) return; /* unknown command */
        if(pack->ar_opL == 0x01) {
#if 0
                printf("Who has %i.%i.%i.%i Tell %i.%i.%i.%i\n",
                        pack->ar_tip[0], pack->ar_tip[1], pack->ar_tip[2],
                        pack->ar_tip[3], pack->ar_sip[0], pack->ar_sip[1],
                        pack->ar_sip[2], pack->ar_sip[3]);
#endif
                /* who has */
                /* if the IP matches ours, reply, else, be silent. */
                if(pack->ar_tip[0] != myip[0]) return; /* not us */
                if(pack->ar_tip[1] != myip[1]) return; /* not us */
                if(pack->ar_tip[2] != myip[2]) return; /* not us */
                if(pack->ar_tip[3] != myip[3]) return; /* not us */
                /* us... */
#if 0
                printf("Our IP, sending reply.\n");
#endif
                pack->ar_tip[0] = pack->ar_sip[0];
                pack->ar_tip[1] = pack->ar_sip[1];
                pack->ar_tip[2] = pack->ar_sip[2];
                pack->ar_tip[3] = pack->ar_sip[3];
                pack->ar_tha = pack->ar_sha;

                pack->ar_sip[0] = myip[0];
                pack->ar_sip[1] = myip[1];
                pack->ar_sip[2] = myip[2];
                pack->ar_sip[3] = myip[3];
                pack->ar_sha = nid;
                pack->ar_opL = 0x02;
                *buf = nid;
                buf++;
                *buf = pack->ar_tha;
                /* now transmit this particular buffer */
                buf--;
                txok();

                adh = 0x06;
                adl = 0;
                setadd();
                i = 256;
                while(i) {
                        putbuf(*buf);
                        buf++;
                        i--;
                }
                outp(WCOM, 0x1b); /* fire, and forget */
                return;
        }

        if(did) return; /* we already have the gateway */

        if(pack->ar_opL == 0x02) {
                /* is at */
                /* if the IP matches ours, it's good. */
                if(pack->ar_tip[0] != myip[0]) return; /* not us */
                if(pack->ar_tip[1] != myip[1]) return; /* not us */
                if(pack->ar_tip[2] != myip[2]) return; /* not us */
                if(pack->ar_tip[3] != myip[3]) return; /* not us */
                /* us... we only need the station address */
                did = pack->ar_sha; /* that's it */
        }
}

/* recieve packet, process it if we get one. */
void arcrx(VOIDFIX) {
        int i;
        int j;
        unsigned char ca;
        unsigned char cb;
        int bufnum;
        unsigned char *buf;

        bufnum = -1;
        /* is there a packet? */
        j = inp(RSTAT);
#if 0
        printf("Status: %x\n", j);
#endif
        if(j < 128) return; /* nothing yet, leave now */
        /* something is in the buffer... */

        /* what buffer is in-use or free? */
        for(i = 0; i < BUFFERS; i++) {
                if(!(txrxflags[i] & 0xff)) {
                        bufnum = i;
                }
        }

        if(bufnum > -1) {
                bufptrs[bufnum] = MALLOC(512); /* a page = 512 bytes */
                if(!bufptrs[bufnum]) {
                        /* ran out of RAM, so we have to wait :-( */
                        return;
                }
                txrxflags[bufnum] = SWALLOWING;
                rxtxpointer[bufnum] = 0;
                /*
                Test packet type.
                If it's IP packet, we only need to swallow in the datagram,
                for arp, we want the entire package as-is
                 */


                /* debugging, dump buffer contents */
#if DUMPS
                adh = adl = 0x00;
                setadd();
                printf("Arcnet buffer 0:\n00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
                for(j = 0; j < 32; j++) {
                        for(i = 0; i < 16; i++) {
                                printf("%2.2x ", getbuf());
                        }
                        printf("\n");
                }
#endif
                /* point to the start of the page +2 */
                adh = 0x00;
                adl = 0x02;
                setadd();

                ca = (getbuf() & 0xff);
                cb = (getbuf() & 0xff);
#if 0
                printf("%2.2x %2.2x\n", ca, cb);
#endif
                if(ca) {
#if 0
                        printf("Short packet\n");
#endif
                        j = ca;
                        i = 256 - ca;
                } else {
#if 0
                        printf("Long packet\n");
#endif
                        j = cb;
                        i = 512 - cb;
                }

                /* check the packet type */
                adh = 0x00;
                adl = j;
                cb = (j & 0xff);
                cb++;
                setadd();
                j = getbuf(); /* packet type */
#if 0
                printf("Packet type %2.2x length %i\n", j, i);
#endif
                i--;
                if(j == 0xf1) {
#if 0
                        printf("Arp packet\n");
#endif
                        buf = bufptrs[bufnum];
                        /* arp packet */
                        adh = adl = 0;
                        setadd();
                        j = 256;
                        while(j) {
                                *buf = getbuf();
                                buf++;
                                j--;
                        }
                        proc_arp(bufnum, cb);
                        freebuffer(bufnum);
                } else if((j == 0xf0) && did) { /* IP packet, did is set */
#if 0
                        printf("IP packet\n");
#endif
                        /* IP, resize and swallow the buffer */
                        FREE(bufptrs[bufnum]);
                        bufptrs[bufnum] = MALLOC(i);
                        rxtxpointer[bufnum] = i;
                        buf = bufptrs[bufnum];
                        while(i) {
                                *buf = getbuf();
#if 0
                                printf("%2.2x ", *buf);
#endif
                                buf++;
                                i--;
                        }
#if 0
                        printf("\n");
#endif
                        txrxflags[bufnum] = SWALLOWED;
                        RECEIVED(bufnum); /* process it */
                } else {
                        /* unknown packet, discard it */
                        freebuffer(bufnum);
                }
#if DUMPS
                clearram(); /* clear all ram to 0x00 for debugging dumps */
#endif
                outp(WCOM, 0x84); /* tell 9066 to accept a packet, into buffer #0 */
        }
}

/*
        A silly but effective method to find the gateway hw address.
        Once found, it stays for the entire life of the running stack.
 */
void findgateway(VOIDFIX) {

        int i;
        int j;
        /*
        should always be a short packet of 19 bytes

        0102 0304 05 06 0708 09 0a0b0c0d 0e 0f101112
        hrd  pro  hl pl  op  sh sourceip th targetip comments
        0007 0800 01 04 0001 01 c0a80301 00 c0a8038d request (who has {0001})
        0007 0800 01 04 0002 8d c0a8038d 01 c0a80301 reply (is at {0002})

        0007 0800 01 04 0001 01 c0a80301 00 c0a803fa

         */

        for(j = 0; j < 32000; j++) {
                /* we abuse our own ip to find the gateway. */
                txok();
                /* make a bogus arp request */
                adh = 0x06;
                adl = 0x00;
                setadd();

                putbuf(nid);
                putbuf(0x00); /* broadcast */
                putbuf(0xed); /* 19 bytes */
                putbuf(0x00); /* nothing */

                adh = 0x06;
                adl = 0xed;
                setadd();

                putbuf(0xf1); /* 1 arp packet */
                putbuf(0x00); /* 2 */
                putbuf(0x07); /* 3 arcnet HT 0x07 */

                putbuf(0x08); /* 4 */
                putbuf(0x00); /* 5 protocol IP 0x0800 */

                putbuf(0x01); /* 6 hlen of 1 */

                putbuf(0x04); /* 7 ip len 4 */

                putbuf(0x00); /* 8 */
                putbuf(0x01); /* 9 opcode 0x0001, who-has */

                putbuf(nid); /* 10 our node id */

                putbuf(myip[0]); /* 11 */
                putbuf(myip[1]); /* 12 */
                putbuf(myip[2]); /* 13 */
                putbuf(myip[3]); /* 14 our ip */

                putbuf(0x00); /* 15 null */

                putbuf(myip[0]); /* 16 fake ip */
                putbuf(myip[1]); /* 17 */
                putbuf(myip[2]); /* 18 */
                putbuf(0x01); /* 19 target ip */

                outp(WCOM, 0x1b); /* fire, and forget */


                for(i = 0; i < 60; i++) {
                        arcrx(); /* try to get the did */
                        if(did) return;
                }
        }
        /* could not find the gateway :-( */
        printf("ERROR! Can't find gateway.\n");
        exit(1);
}

/* polling point */
void arcrxtx(VOIDFIX) {
        int i;
        int gorx;

        gorx = 0;

        if(!did) findgateway(); /* find the gateway before going any farther */
        /*
                If we didn't find any gateway
                just broadcast any waiting packet
                next time we may find one...
                if there is a change in a route,
                dead transmissions on open can reset did again...
         */

        /* walk thru all tx buffers */
        for(i = BUFFERS - 1; i > 0; i--) {
                /* something to tx? */
                if((txrxflags[i] & (ISTX | READY)) == (ISTX | READY)) {
                        /* send it out */
                        arctx(i);
                }
#if HANDLEFRAGS
        }
        /* process fragments and anything else swallowed */
        PROCFRAGS
#else
                if((txrxflags[i] & (SWALLOWED)) == (SWALLOWED)) {
                        gorx = 1;
                        RECEIVED(i);
                }
        }
#endif /* HANDLEFRAGS */
        if(!gorx) arcrx();
}

#if DOTESTS

void bad(location, should, is)
int location;
int should;
int is;
{
        printf("BAD RAM @ %4.4X, should be %2.2X, reads as %2.2X\n", location, should, is);
        exit(1);
}
#endif

void initializedev(VOIDFIX) {
        int j;
#if DOTESTS
        int k;
#endif
        /* check if we need a "cold restart", steal bit 5 for this */
        j = inp(RCONF) & 0x20; /* decode mode */
        nid = inp(RNID);
        if((!j) || (nid == 0xff) || (!nid)) {
                inp(RRESET);
                inp(RRESET);
                for(j = 0; j < 255; j++);
                outp(WCONF, 0x3a);
                inp(RRESET);
                for(j = 0; j < 255; j++);
                inp(RSTAT);
                for(j = 0; j < 255; j++);
                inp(RSTAT);
        }
        outp(WCONF, 0x3a); /* turn on nic, no command chaining */
        nid = inp(RNID);
        /* setup is complete */
        printf("%s\nNode ID =%2.2X\n", &HARDDRV[0], nid);
        /* clear any previous rx and tx */
        /*	outp(WCOM, 0x00);  clear all transmit */
        outp(WCOM, 0x01); /* cancel any current xmit */
        outp(WCOM, 0x02); /* cancel any previous rx */
        outp(WCOM, 0x0d); /* enable long packets */
        outp(WCOM, 0x1e); /* clear initial power on crap */
#if DOTESTS
        /* test ram */
        adh = adl = 0;
        setadd();
        for(j = 0; j < 1024; j++) {
                putbuf(0x55);
                putbuf(0xAA);
        }

        adh = adl = 0;
        setadd();
        for(j = 0; j < 1024; j++) {
                k = getbuf();
                if(k != 0x55) bad(j, 0x55, k);
                k = getbuf();
                if(k != 0xAA) bad(j, 0xAA, k);
        }

        adh = adl = 0;
        setadd();
        for(j = 0; j < 1024; j++) {
                putbuf(0xAA);
                putbuf(0x55);
        }

        adh = adl = 0;
        setadd();
        for(j = 0; j < 1024; j++) {
                k = getbuf();
                if(k != 0xAA) bad(j, 0xAA, k);
                k = getbuf();
                if(k != 0x55) bad(j, 0x55, k);
        }
#endif
#if DUMPS
        clearram(); /* clear all ram to 0x00 for debugging dumps */
#endif

        /* tell 9066 to accept a packet, into buffer #0 */
        outp(WCOM, 0x84);
        /* finally find our gateway, this will try forever */
        findgateway();
        return;
}

#else /*  */

/* just make a pretend symbol */
void com90c66d(VOIDFIX) {
        return;
}

#endif /* USEDOSUART */


/* tell link parser who we are and where we belong

1107 libznet.lib arcnet.obj xxLIBRARYxx

 */

