/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#ifndef IN_H_SEEN
#define IN_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

/* Standard well-defined IP protocols.  */
#define IPPROTO_IP 0		/* Dummy protocol for TCP			*/
#define IPPROTO_ICMP 1		/* Internet Control Message Protocol		*/
#define IPPROTO_IGMP 2		/* Internet Group Management Protocol		*/
#define IPPROTO_IPIP 4		/* IPIP tunnels (older KA9Q tunnels use 94)	*/
#define IPPROTO_TCP 6		/* Transmission Control Protocol		*/
#define IPPROTO_EGP 8		/* Exterior Gateway Protocol			*/
#define IPPROTO_PUP 12		/* PUP protocol					*/
#define IPPROTO_UDP 17		/* User Datagram Protocol			*/
#define IPPROTO_IDP 22		/* XNS IDP protocol				*/
#define IPPROTO_RSVP 46		/* RSVP protocol				*/
#define IPPROTO_GRE 47		/* Cisco GRE tunnels (rfc 1701,1702)		*/
#define IPPROTO_IPV6 41		/* IPv6-in-IPv4 tunnelling			*/
#define IPPROTO_PIM 103		/* Protocol Independent Multicast		*/
#define IPPROTO_ESP 50		/* Encapsulation Security Payload protocol	*/
#define IPPROTO_AH 51		/* Authentication Header protocol		*/
#define IPPROTO_COMP 108	/* Compression Header protocol			*/
#define IPPROTO_RAW 255		/* Raw IP packets				*/

/* Internet address. */
struct in_addr {
        unsigned long s_addr; /* warning, assumes 32 bits	*/
};


#endif /* IN_H_SEEN */
