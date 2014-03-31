/* (C) 2001-2014 Andrew J. Kroll
 *  see file README for license details
 *
 *      Configure your i/o method here, add in how your system works
 *      Comment out any that do not apply.
 *
 *      TO-DO:
 *              * Don't use defines for buffer sizes,
 *                instead make them dynamic and check the limits.
 */

#ifndef CONFIG_H_SEEN
#define CONFIG_H_SEEN
/*
--- Select ARCH dependencies here ------------------------------------------------------------------
 */

#ifdef __MSDOS__
#define CPU "IA86 or compatable"
#endif
#ifdef __i386__
#define CPU "IA86 or compatable"
#endif
#ifdef __AS386_16__
#define CPU "IA86 or compatable"
#endif
#ifdef __AS386_32__
#define CPU "IA86 or compatable"
#endif

#ifdef Z80
#define CPU "Z80"
#endif

#ifdef CPM
#define OS "CP/M"
#endif

#ifdef linux
#define OS "Linux"
#endif

#ifdef __MSDOS__
#define OS "DOS"
#endif

#ifdef ARDUINO
#include <Arduino.h>
#if __AVR__

#define CPU _AVR_CPU_NAME_
#endif
#ifdef CORE_TEENSY
/* steal stk500v2 names */
#include <../bootloaders/stk500v2/avr_cpunames.h>
#define OS "Teensyduino no OS"
#else
#include <../../bootloaders/stk500v2/avr_cpunames.h>
#define OS "Arduino no OS"
#endif
#endif

#ifndef CPU
#define CPU "Unknown CPU"
#endif

#ifndef OS
#define OS "Unknown OS"
#endif

/* set to 1 if your tight on ram, to make applications buffers smaller */
#define TIGHTRAM 0

#ifndef MY_BAUD_RATE
#define MY_BAUD_RATE 9600
#endif

/* (UNIX | OS/M | MP/M | CP/M 3.x | __MSDOS__  | Arduino) */
#define HAVE_RTC 1

/* Arduino C++ interface */
#define USEARDUINO 1

/* (OS/M | MP/M | CP/M 2.x | CP/M 3.x) on Z80DART */
#define USEZ80DART 0

/* CP/M3 (CP/M+) AUX: device */
#define USECPM3AUX 0

/* Linux on ttyS */
#define USELINUXTTYS 0

/* ELKS on ttyS */
#define USEELKSTTYS 0

/* MS-DOS on 8250 */
#define USEDOSUART 0

/* Xerox 820 and BigBoard I */
#define USEZ80SIO 0
#define USECOM8116 0

/* c128 cpm and "silver surfer" */
#define USESILVER 0

/* c128 with fixed ACIA and cp/m */
#define USEACIA 0

/*
--- Select stack dependencies here ----------------------------------------------------------------
 */

/* socket settings */
#define MAXBACKLOG 3

/* Standard packet size of 1500 eats shit loads of ram.
 * 514 is minimum for UDP DNS packets, and we can't fragment IP yet.
 * Add 40 for the header information... */
#define MAXMTUMRU (514 + 40)

/* set to 0 to disable fragment handling, only if you really need to.
 * set to 1 to enable inbound fragment handling
 */
#define HANDLEFRAGS 1

/* max write buffer size, 384 is 3 cp/m sectors,
 * If you can, use at least MAXMTUMRU, more is better, of course.
 */
#define MAXTAILW (MAXMTUMRU * 3)

/*
 * Max read tail size, must be at least MAXPKTSZ,
 * bigger is better if you can afford it.
 * More encourages the remote to stream...
 * As a smallest size rule, value set below 1501 can cause SWS problems,
 * so do not go below 1500.
 * http://en.wikipedia.org/wiki/Silly_window_syndrome
 * 3072 is a good value for a single connection.
 * Adjust as needed.
 */
#define MAXTAILR (MAXMTUMRU * 3)

/* WIRE PROTOCOLS */
#define USESIMPLESLIP 1
#define USEARCNET 0

/* Network protocols */
#define IP_ENABLED 1
#define TCP_ENABLED 1
#define UDP_ENABLED 1
#define ICMP_ENABLED 1

/* if no frags, this gets used. minimum of 3, maximum 255 */
#define LOWBUFFERS 3
/* otherwise this setting will be used. minimum of 6, maximum 255 */
#define HIGHBUFFERS 8

/* How many bytes in flight? 0 - MAXTAILW, 0 == disable */
#define WINDOWED MAXTAILW

/*
--- Nothing below here should need to be altered. --------------------------------------------------
 */











































/* fixups for GCC, and probably other compilers. */
#include "unproto.h"

#ifdef M5
/* NORMALIZE */
#define cli _disable
#define sti _enable
#define strncasecmp strnicmp
#endif /* M5 */

#ifdef HI_TECH_C
#include <sys.h>
#include <unixio.h>
#define volatile
#define const
#else /* HI_TECH_C */
#if !USEELKSTTYS
#ifndef M5
#ifndef ARDUINO
#include <asm/io.h>
#define inp     inb
#define outp    outb
#endif /* ARDUINO */
#endif /* M5 */
#endif /* !USEELKSTTYS */
#endif /* HI_TECH_C */

#if HANDLEFRAGS
#define BUFFERS HIGHBUFFERS
#else
#define BUFFERS LOWBUFFERS
#endif /* HANDLEFRAGS */

#define DEBUG 0

#define DEBUGSLIP 0


/* baud rates, hardware and timer specifics */
#include "hrdware.h"

#if HAVE_RTC
#define USE_TIME 1
#define RETRYTIME 3
#else
/*
 Set this to use serial port for time calibration.
 The number is the baud rate of the link, this is used
 ONLY if your system lacks the time() call and if the time()
 call fails. The actual baud rate is set in "hrdware.h".
 */
#define USE_TIME 0
#define _CPM_SERIAL_CALIBRATION_ MY_BAUD_RATE
/*
Divide by 10 on z80 seems to work very well.
Faster machines shouldn't have to adjust this
pending how well they are written for calibration.
 */
#define RETRYTIME 2
#endif
#define STANDARDHEADER 24
#define IPHEADLEN 0x14
#define MSSOPTION 4
#define FULLHEADER (IPHEADLEN + STANDARDHEADER)
#define SMALLHEADER (FULLHEADER - MSSOPTION) /* minus the MSS options.... */
#define DEFAULTMSS (MAXMTUMRU - FULLHEADER)
/*
Maximum data size hint.
This value is the initial window size,
It is no more than MAXMTUMRU-40
The idea is to deter the sender from sending fragments.
Providing the sender has a proper stack, it works...
 */
#define MAXPKTSZ (MAXMTUMRU - SMALLHEADER)


#define STACK_IS "00.01.22"

#endif /* CONFIG_H_SEEN */
