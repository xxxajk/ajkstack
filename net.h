/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* net.h */
#ifndef NET_H_SEEN
#define NET_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern int initnetwork K__P F__P(int, char *[])P__F;
extern void endnetwork K__P F__P(void)P__F;
#ifdef	__cplusplus
}
#endif

/* Internet IP Protocol */
#define AF_INET 2
#define PF_INET AF_INET

#define SOL_IP 0
#define SOL_TCP 6
#define SOL_UDP 17

#define TCP_NODELAY 1
#define TCP_MAXSEG 2

/* speed of cpu, only used for polled data in MHz */
#define CPUSPEED 4

/*
The grainyness of the cpu for 1MHz to make one second.
This is highly inaccurate and I simply  pulled the number out of my hat.
 */
#define GRAIN 100

/* this is the timer tick countdown value only used for retransmits */
#define TIMETICK (GRAIN*CPUSPEED)
#endif /* NET_H_SEEN */
