/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* icmp.h */
#ifndef ICMP_H_SEEN
#define ICMP_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

extern void tcpdump K__P F__P(unsigned char *, unsigned int)P__F;
extern void err_reply K__P F__P(unsigned int, unsigned int, unsigned int, int)P__F;
extern void icmp K__P F__P(unsigned int, unsigned int, unsigned int)P__F;
#endif /* ICMP_H_SEEN */
