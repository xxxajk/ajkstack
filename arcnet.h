/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* arcnet.h */
#ifndef ARCNET_H_SEEN
#define ARCNET_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#if USEARCNET /* Include this code? */

extern void initializedev K__P F__P(void)P__F;

extern char HARDDRV[];
/* For polled network stuffs, define IOPOLL to your polling routine */
/* as it will be sprinkled thruout the code. */
/* The main use is in heavy loops so no chars get dropped */
/* see useage example in all the other code. */
#define INITHARDWARE initializedev();

#define IOPOLL
#define TRANSPORT arcrxtx();
#define CHECKUP 4096
#endif /* USEARCNET */

#endif /* ARCNET_H_SEEN */
