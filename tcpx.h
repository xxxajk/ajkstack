/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#ifndef TCPX_H_SEEN
#define TCPX_H_SEEN

#include "config.h"

#include "ip.h"
#define DOITHERE 0

extern void bumpid K__P F__P(void)P__F;
extern void checkoptions K__P F__P(unsigned char *, int, int, int)P__F;
extern void bufferdatamove K__P F__P(unsigned char *, unsigned int, unsigned int)P__F;
extern void tcpsumming K__P F__P(unsigned char *)P__F;
extern int fasttcppack K__P F__P(int, int, int, int, unsigned char *, unsigned int)P__F;
extern void freebuffer K__P F__P(int)P__F;

/* inlines for speed, ODD CASE: reduces memory as a side effect */
#define tcp_rst(x) fasttcppack(x, RST,STATE_CLOSED, 0, (unsigned char *)NULL, 0);
#define tcp_finack(x) fasttcppack(x, FIN | ACK, STATE_CLOSED, 0, (unsigned char *)NULL, SENDNOCARE);
#define tcp_fin(x) fasttcppack(x, FIN, STATE_CLOSED, 0, (unsigned char *)NULL, SENDNOCARE);
#define tcp_synandack(x) fasttcppack(x, SYN | ACK ,STATE_SYN_RECEIVED ,0 ,(unsigned char *)NULL, 0);
#define tcp_acksyn(x) fasttcppack(x, ACK, STATE_ESTABLISHED, 0, (unsigned char *)NULL, SENDNOCARE);
#define tcp_syn(x) fasttcppack(x, SYN,STATE_SYN_SENT, 0, (unsigned char *)NULL, 0);
#define tcp_psh(x) fasttcppack(x, PSH, STATE_ESTABLISHED, len, ptr, SENDNOCARE);

extern int tcp_open K__P F__P(int, unsigned int, unsigned char *)P__F;
extern int matchsocket K__P F__P(unsigned char *)P__F;
extern int checkack K__P F__P(unsigned char *, unsigned char *)P__F;

extern unsigned int createtcp K__P F__P(unsigned char *, unsigned char *, int, unsigned char *)P__F;

#endif /* TCPX_H_SEEN */
