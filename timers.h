/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* timers.h */
#ifndef TIMERS_H_SEEN
#define TIMERS_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

extern void settimer1 K__P F__P(int)P__F;
extern unsigned long chktimer1 K__P F__P(void)P__F;
extern void calibrate K__P F__P(void)P__F;
extern void show_loop_cal K__P F__P(void)P__F;

#define CAL_TIME show_loop_cal();
#define SET_TIME1(number) (settimer1(number))
#define CHK_TIME1 (chktimer1())
#define CALTIMER calibrate()
#endif /* TIMERS_H_SEEN */
