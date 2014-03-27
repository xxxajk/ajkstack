/*
 * File:   ardser.h
 * Author: xxxajk
 *
 * Created on March 4, 2014, 12:03 AM
 */

#ifndef ARDSER_H
#define	ARDSER_H
#if USEARDUINO
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

#ifdef __cplusplus
}
#endif
/* NOTE! Use teensy++ 2.0 USB Serial for best results! Put the console on Serial1! */
#ifndef SLIPDEV
#define SLIPDEV Serial
#endif
#define INITHARDWARE initializedev();
#define CHECKOUT check_out()
#define CHECKIN checkdata()
#define GETDATA infromdev()
#define PUTDATA(x) outtodev(x)
#define CHECKUP check_up()
#define INIT_EXIT /* we lack atexit() */
#endif /* USEARDUINO */
#endif	/* ARDSER_H */
