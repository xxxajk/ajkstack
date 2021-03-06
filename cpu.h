/*
 * (C) 2001-2014 Andrew J. Kroll
 *  see file README for license details
 */

#ifndef AJKSTACK_CPU_H
#define	AJKSTACK_CPU_H

#ifdef ARDUINO
#include <Arduino.h>
/* Assumes we have some kind of RTC, but a fake one is available */
#define HAVE_RTC 1
 /* Assumes we have USB Mass Storage */
#define HAS_STORAGE 1
#define MY_BAUD_RATE 9600
#ifdef CORE_TEENSY
#ifdef __arm__
//#define USE_EEM 1
// Used for now until EEM/ECM is written.
#define USEARDUINO 1

#ifdef __MK20DX256__
#define CPU "ARM Cortex-M4"
#define OS "Teensyduino 3.1 no OS"
#endif
#ifdef __MK20DX128__
#define CPU "ARM Cortex-M4"
#define OS "Teensyduino 3.0 no OS"
#endif

#ifndef OS
#define CPU "ARM Cortex-M"
#define OS "Teensyduino no OS"
#endif

#else
#define USEARDUINO 1
/* steal stk500v2 names */
#include <../bootloaders/stk500v2/avr_cpunames.h>
#define CPU _AVR_CPU_NAME_
#define OS "Teensyduino no OS"
#endif /* __arm__ */

#else
#define USEARDUINO 1
#ifdef __arm__
#define CPU "ARM"
#define OS "Arduino no OS"
#else
/* steal stk500v2 names */
#include <../../bootloaders/stk500v2/avr_cpunames.h>
#define CPU _AVR_CPU_NAME_
#define OS "Arduino no OS"
#endif
#endif /* CORE_TEENSY */
#endif /* ARDUINO */


#ifdef __XC16_VERSION__
/*
 * Define CPU by family.
 * There are too many to go into, and the headers don't define anything better.
 */
#ifdef __dsPIC30F__
#define CPU "dsPIC30F"
#endif
#ifdef __dsPIC33F__
#define CPU "dsPIC33F"
#endif
#ifdef __PIC24F__
#define CPU "PIC24F"
#endif
#ifdef __PIC24FK__
#define CPU "PIC24FK"
#endif
#ifdef __PIC24H__
#define CPU "PIC24H"
#endif
#define OS "No OS"
#define USE_PICxxxx_CDC 1
#include <CDC.h>
#endif

/*****************************************************************************/

#ifdef CPM
#define OS "CP/M"
#define NO_OS 0
#define HAS_STORAGE 1
#endif

#ifdef linux
#define OS "Linux"
#define NO_OS 0
#define LACKS_ARGV0 0
#define HAS_STORAGE 1
#define HAVE_RTC 1
#define USELINUXTTYS 1
#endif

#ifdef __MSDOS__
#define CPU "8086 or compatable"
#define OS "DOS"
#define NO_OS 0
#define LACKS_ARGV0 0
#define HAS_STORAGE 1
#define USEDOSUART 1
#define HAVE_RTC 1
#endif

#ifdef __i386__
#define CPU "i386 or compatable"
#endif
#ifdef __AS386_16__
#define CPU "i386 16bit mode or compatable"
#endif
#ifdef __AS386_32__
#define CPU "i386 32bit mode or compatable"
#endif

#ifdef Z80
#define CPU "Z80"
#endif



/*****************************************************************************/

#ifndef LACKS_ARGV0
#define LACKS_ARGV0 1
#endif

#ifndef NO_OS
/* Hint to not use exit() */
#define NO_OS 1
#endif

#ifndef HAS_STORAGE
/* Hint to hardcode IP information */
#define HAS_STORAGE 0
#endif

#ifndef CPU
#define CPU "Unknown CPU"
#endif

#ifndef OS
#define OS "Unknown OS"
#endif


#endif	/* AJKSTACK_CPU_H */

