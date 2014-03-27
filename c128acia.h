/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* c128acia.h */
#ifndef C128ACIA_H_SEEN
#define C128ACIA_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#if USEACIA /* Include this code? */
extern void zapbuffers K__P F__P(void)P__F;
extern void pollinput K__P F__P(void)P__F;
extern int checkdata K__P F__P(void)P__F;
extern int check_out K__P F__P(void)P__F;
extern int check_up K__P F__P(void)P__F;
extern void outtodev K__P F__P(unsigned int)P__F;
extern unsigned char infromdev K__P F__P(void)P__F;
extern void initializedev K__P F__P(void)P__F;
extern char HARDDRV[];
/* For polled network stuffs, define IOPOLL to your polling routine */
/* as it will be sprinkled thruout the code. */
/* The main use is in heavy loops so no chars get dropped */
/* see useage example in all the other code. */
#define INITHARDWARE initializedev();
/* #define IOPOLL pollinput(); */
/* #define XFLUSH_UART (check_out() > 0) */
#define CHECKOUT check_out()
#define CHECKIN checkdata()
#define GETDATA infromdev()
#define PUTDATA(x) outtodev(x)
#define CHECKUP check_up()

#endif /* USEACIA */
#endif /* C128ACIA_H_SEEN */
