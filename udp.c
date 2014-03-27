/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

UDP RFC 0768
Checksum is the 16-bit one's complement of the one's complement sum of a
pseudo header of information from the IP header, the UDP header, and the
data,  padded  with zero octets  at the end (if  necessary)  to  make  a
multiple of two octets.

To calculate UDP checksum a "pseudo header" is added to the UDP header.
This pseudo header includes:

IP Source Address	4 bytes
IP Destination Address	4 bytes
Protocol		2 bytes
UDP Length  		2 bytes

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "in.h"
#include "netinet.h"
#include "icmp.h"
#include "tcpx.h"
#include "chcksum.h"

#if IP_ENABLED
#if UDP_ENABLED
#define DEBUGUDP 0

/* rx UDP RFC 0768 */

void udp(bufnum, iplen, totallen, udplen)
unsigned int bufnum;
unsigned int iplen;
unsigned int totallen;
unsigned int udplen;
{
	unsigned char *buffer;
	unsigned char *bufflags;
	unsigned char *srcip;
	unsigned char a;
	unsigned char b;
	unsigned char s1;
	unsigned char s2;
	unsigned char s3;
	unsigned char s4;
	int sum;
	int usum;
	int sock;

	sock = -1;
	buffer = bufptrs[bufnum];

	if(udplen < 8) goto black_hole;
	srcip = &buffer[iplen];
	bufflags = txrxflags + bufnum;
	a = buffer[iplen + 6];
	b = buffer[iplen + 7];
	sum = (a * 256) + b; /* inbound udp sum */
#if DEBUGUDP
	printf("\nUDP ");
#endif

	/*
	   3 steps:
	   1: Test checksum.
	   2: Match inbound port.
	   3a: If there is no room, black hole the packet.
	   3b: Place package into the queue.
	 */
	/* 1: Test checksum, if there is one.... */
	if(sum) {
		/* there is a checksum */

		/* manipulate the ip header area for our own use */
		s1 = buffer[8]; /* save */
		s2 = buffer[9]; /* the */
		s3 = buffer[10]; /* old */
		s4 = buffer[11]; /* values */
		buffer[9] = 0x11;
		buffer[10] = (unsigned char)((udplen >> 8) & 0x00ff);
		buffer[11] = (unsigned char)(udplen & 0x00ff);
		buffer[8] =
		buffer[iplen + 6] =       /* these are already */
		buffer[iplen + 7] = 0x00; /* restored if needed */
		usum = checksum(&buffer[iplen], udplen, &buffer[8], 12);

		if(!usum) usum = 0xffff;
		buffer[8] = s1; /* restore */
		buffer[9] = s2; /* the */
		buffer[10] = s3; /* old */
		buffer[11] = s4; /* values */


#if DEBUGUDP
		printf("LEN %i CHECKSUM I:O %4.4x %4.4x ", udplen, sum, usum);
#endif
		if(sum != usum) {
#if DEBUGUDP
			printf("BAD ");
#endif
			goto black_hole;

#if DEBUGUDP
		} else {
			printf("OK ");
#endif
		}
		buffer[iplen + 6] = a;
		buffer[iplen + 7] = b;
#if DEBUGUDP
	} else {
		printf("NO CHECKSUM, OK ");
#endif
	}
	/* 2: Match inbound port. */
	sock = matchsocket(srcip);
	if(sock < 0) {
		/* type 3, code 3 port unreachable */
		err_reply(bufnum, 3, 3, 0);
#if DEBUGUDP
		printf("NO SOCKET MATCHED\n");
#endif
		return;
	} else {
#if DEBUGUDP
		printf("SOCKET MATCHED %i ", sock);
#endif
		/* 3a: If there is no room, black hole the packet. */
		if((MAXTAILR - rbufcount[sock]) < (int)totallen) {
#if DEBUGUDP
			printf("NO ROOM ");
#endif
			goto black_hole;
		}
#if DEBUGUDP
		printf("SAVING PACKET "); fflush(stdout);
#endif
		/* 3b: Place package into the queue. */
		buffer[0] = 0x00;
		bufferdatamove(&buffer[0], sock, 1); /* protocol byte */
		buffer[0] = AF_INET;
		bufferdatamove(&buffer[0], sock, 1); /* protocol byte */
		bufferdatamove(&buffer[iplen + 3], sock, 1); /* source port */
		bufferdatamove(&buffer[iplen + 2], sock, 1); /* source port */
		bufferdatamove(&buffer[12], sock, 4); /* source IP */
		buffer[1] = (unsigned char)((unsigned short)(udplen - 8) & 0x00ff);
		buffer[0] = (unsigned char)((((unsigned short)(udplen - 8) & 0xff00) >> 8) & 0xff);
		bufferdatamove(&buffer[0], sock, 2); /* byte in packet */
		if((udplen - 8) > 0) {
			bufferdatamove(&buffer[iplen + 8], sock, udplen - 8); /* UDP information as-is */
		}
#if DEBUGUDP
		printf("SAVED, now %i Bytes", rbufcount[sock]); fflush(stdout);
#endif
	}

black_hole:
	freebuffer(bufnum);
#if DEBUGUDP
	printf("DONE.\n");
#endif
	return;
}


#else /* UDP_ENABLED */
void udp(bufnm, iplen, totallen, udplen)
unsigned int bufnum;
unsigned int iplen;
unsigned int totallen;
unsigned int udplen;
{
	/* type 3, code 3 protocol unreachable */
	err_reply(bufnum, 3, 2, 0);
	return;
}
#endif /* UDP_ENABLED */

#else /* IP_ENABLED */
void udpnotused(VOIDFIX)
{
	return(0);
}
#endif /* IP_ENABLED */
/* tell link parser who we are and where we belong

0600 libznet.lib udp.obj xxLIBRARYxx

*/

