/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* tcp.h */
#ifndef TCP_H_SEEN
#define TCP_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif
extern void nuke_tcp_socket K__P F__P(int)P__F;
extern int tcp_open K__P F__P(int, unsigned int, unsigned char *)P__F;
extern int tcp_write K__P F__P(int, int, int, unsigned char *)P__F;
extern int tcp_read K__P F__P(int, int, int, unsigned char *)P__F;
extern void tcp K__P F__P(unsigned int, unsigned int, unsigned int)P__F;
#endif /* TCP_H_SEEN */
