/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#ifndef ELKSSER_H
#define ELKSSER_H

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#if USEELKSTTYS
extern void zapbuffers K__P F__P(void)P__F;
extern void pollinput K__P F__P(void)P__F;
extern int checkdata K__P F__P(void)P__F;
extern int check_out K__P F__P(void)P__F;
extern int check_up K__P F__P(void)P__F;
extern void outtodev K__P F__P(unsigned int)P__F;
extern unsigned char infromdev K__P F__P(void)P__F;
extern void initializedev K__P F__P(void)P__F;
extern char HARDDRV[];

#define GETDATA infromdev()
#define CHECKOUT check_out()
#define CHECKIN checkdata()
#define PUTDATA(x) outtodev(x)
#define CHECKUP check_up()

#endif /* USEELKSTTYS */
#endif /* ELKSSER_H */

