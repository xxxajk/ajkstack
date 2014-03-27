/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* socket.h */

#ifndef SOCKET_H_SEEN
#define SOCKET_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#include "in.h"

/* set this to non zero for debug facility */
#define DUMPSOCKSTATES 0

/* socket types */
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3
#define SOCK_RDM 4
#define SOCK_SEQPACKET 5
#define SOCK_PACKET 10
#define SOKDAT 32 /* 29 + some padding to align it, also allows for future flags */

/*
These are used instead of struct because
it remidies any sizeof() issues.
It may also make the code more readable.
To the compiler, it's like precalculated structures,
therefore generating tighter code.
*/

#define SOKAT(x) (sockets + (SOKDAT * x)) /* calculate socket offset */

/* socket "struct" */
#define IPDEST(x,y) ((SOKDAT * x) + y)	/*  4  bytes	0  1  2  3	*/
#define SRCPRT(x,y) (IPDEST(x,y) +  4)	/*  2  bytes	4  5		*/
#define DESPRT(x,y) (IPDEST(x,y) +  6)	/*  2  bytes	6  7		*/
#define SEQNUM(x,y) (IPDEST(x,y) +  8)	/*  4  bytes	8  9  10 11	*/
#define ACKNUM(x,y) (IPDEST(x,y) + 12)	/*  4  bytes 	12 13 14 15	*/
#define BUFUSE(x,y) (IPDEST(x,y) + 16)	/*  1  byte	16		*/
#define BUFSTA(x,y) (IPDEST(x,y) + 17)	/*  1  byte	17		*/
#define WINDOW(x,y) (IPDEST(x,y) + 18)	/*  2  bytes	18 19		*/
#define SOCKFD(x,y) (IPDEST(x,y) + 20)	/*  1  byte	20		*/
#define SOCKST(x,y) (IPDEST(x,y) + 21)	/*  1  byte	21		*/
#define STTYPE(x,y) (IPDEST(x,y) + 22)	/*  1  byte	22		*/
#define STPROT(x,y) (IPDEST(x,y) + 23)	/*  1  byte	23		*/
#define SOKMSS(x,y) (IPDEST(x,y) + 24)	/*  2  bytes	24 25		*/
#define SLANOD(x,y) (IPDEST(x,y) + 26)	/*  1  byte	26		*/
#define SPARET(x,y) (IPDEST(x,y) + 27)	/*  1  byte	27		*/
#define SPARPO(x,y) (IPDEST(x,y) + 28)	/*  1  byte	28		*/
/*                                         ---------           		*/
/*                                         29 bytes total		*/

#define SOKALLOW BUFFERS

#ifdef	__cplusplus
extern "C" {
#endif

#if DUMPSOCKSTATES
#define SOCKSTATS sockstats();
extern void sockstats K__P F__P(void)P__F;
#else
#define SOCKSTATS
#endif
#define CHECKSOKS check_sockets();
#define MAKEMEBLEED() schedulestuff()

typedef size_t socklen_t;

struct sockaddr {
	unsigned short	sa_family;	/* address family, AF_xxx	*/
	char 	sa_data[14];            /* 14 bytes of protocol address	*/
};

#define __SOCK_SIZE__   16              /* sizeof(struct sockaddr)	*/

struct sockaddr_in {
	unsigned short	sin_family;	/* Address family		*/
	unsigned short	sin_port;	/* Port number			*/
	struct in_addr	sin_addr;	/* Internet address		*/
	unsigned char	__pad[__SOCK_SIZE__ - sizeof(short int) - sizeof(unsigned short int) - sizeof(struct in_addr)];
};

extern void dumpsock K__P F__P(int)P__F;
extern void sendsocket K__P F__P(int)P__F;
extern int sockstatus K__P F__P(int)P__F;
extern int AJK_bindcount K__P F__P(int)P__F;
extern void check_sockets K__P F__P(void)P__F;
extern void schedulestuff K__P F__P(void)P__F;

/* Actual interfaces */

extern int AJK_socket K__P F__P(int, int, int)P__F;
extern int AJK_sockwrite K__P F__P(int, void *, unsigned int)P__F;
extern int AJK_sockread K__P F__P(int, void *, unsigned int)P__F;
extern int AJK_sockclose K__P F__P(int)P__F;
extern int AJK_sockconnect K__P F__P(int, unsigned int, unsigned char *)P__F;
extern int AJK_connect K__P F__P(int, struct sockaddr_in *, int)P__F;
extern int AJK_bind K__P F__P(int, struct sockaddr_in *, int)P__F;
extern int AJK_listen K__P F__P(int, int)P__F;
extern int AJK_acceptready K__P F__P(int)P__F;
extern int AJK_accept K__P F__P(int, struct sockaddr_in *, int *)P__F;
extern int AJK_sockdepth K__P F__P(int, int)P__F;
#if UDP_ENABLED
extern int AJK_sendto K__P F__P(int, const void *, size_t, unsigned int, const struct sockaddr *, socklen_t )P__F;
extern int AJK_recvfrom K__P F__P(int, void *, size_t, unsigned int, struct sockaddr *, socklen_t )P__F;
#endif /* UDP_ENABLED */

#ifdef	__cplusplus
}
#endif
#endif /* SOCKET_H_SEEN */
