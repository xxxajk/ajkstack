/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
 * Microchip family CDC interface using MLA v2013_12_20.
 *
 * Only tested on PIC24FJ192GB106 Others should work too.
 *
 */

#ifndef PICXXCDC_H
#define	PICXXCDC_H
#ifndef CONFIG_H_SEEN
#include "config.h"
#endif
#if USE_PICxxxx_CDC

#include <CDC.h>
#ifdef __cplusplus
extern "C" {
#endif

extern void zapbuffers K__P F__P(void)P__F;
extern void pollinput K__P F__P(void)P__F;
extern int checkdata K__P F__P(void)P__F;
extern int check_out K__P F__P(void)P__F;
extern int check_up K__P F__P(void)P__F;
extern void outtodev K__P F__P(unsigned int)P__F;
extern unsigned char infromdev K__P F__P(void)P__F;
extern void initializedev K__P F__P(void)P__F;
extern char HARDDRV[];
#define INITHARDWARE initializedev();
#define CHECKOUT check_out()
#define CHECKIN checkdata()
#define GETDATA infromdev()
#define PUTDATA(x) outtodev(x)
#define CHECKUP check_up()
#define INIT_EXIT /* we lack atexit() */

#ifdef __cplusplus
}
#endif

#endif
#endif	/* PICXXCDC_H */

