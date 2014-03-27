/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* slip.h */

#ifndef SLIP_H_SEEN
#define SLIP_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#if USESIMPLESLIP
extern void sliptxone K__P F__P(unsigned int c)P__F;
extern int sliprxone K__P F__P(void)P__F;
extern void sliptx K__P F__P(int bufnum)P__F;
extern void sliprx K__P F__P(void)P__F;
extern void sliprxtx K__P F__P(void)P__F;
#define TRANSPORT sliprxtx();

#define FRAME 0xC0
#define NOTFRAME1 0xDB
#define NOTFRAME2 0xDC
#define NOTFRAME3 0xDD

#endif /* USESIMPLESLIP */

#endif /* SLIP_H_SEEN */
