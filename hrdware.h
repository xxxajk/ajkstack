/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* extra hardware specific definitions. */
#ifndef HRDWARE_H_SEEN
#define HRDWARE_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

/* Where CP/M (all versions) store the config file. */
#ifdef CPM
#if USEARCNET
#define CONFIGFILE "0:A:TCP.RCA"
#else
#define CONFIGFILE "0:A:TCP.RC"
#endif
#endif

/* Where DOS stores the config file */
#ifdef __MSDOS__
#define CONFIGFILE "C:\\TCP.RC"
#endif

/* assume where unix would store the config file */
#ifndef CONFIGFILE
#define CONFIGFILE "/etc/tcp.rc"
#endif

#if USESIMPLESLIP
/* Default settings... */

/* init the UART's baud? */
#define INIT_BAUD 1

/* unix (elks/linux) */
#define SERIALDEVICE "/dev/ttyS0"

#if USEDOSUART
/* DOS uart settings, default COM1: 0x3F8/4 */
#define BASE_8250 0x3F8
#define IRQ_8250 4 /* one of 2, 3, 4, 5, 7 */
/* Set to 1 to enable polled UART, 0 to use IRQ */
#define POLLED_DOS_UART 0
#endif

#if USEZ80DART

/* Z80 DART */
#define DART_DATA_PORT 0x02
#define DART_CMD_PORT 0x03

/*
        These macros assume 19200 UART clock rate.
        Adjust as needed.
 */

/* 1X clock */
#if MY_BAUD_RATE == 19200
#define WREGCLK 0x00
#endif

/* 16X clock */
#if MY_BAUD_RATE == 9600
#define WREGCLK 0x40
#endif

/* 32X clock */
#if MY_BAUD_RATE == 4800
#define WREGCLK 0x80
#endif

/* 64X clock */
#if MY_BAUD_RATE == 2400
#define WREGCLK 0xc0
#endif

#ifndef WREGCLK
error_may_need_to_edit_hrdware_h_macros = BROKEN;
#endif

#endif /* USEZ80DART */


#if USEDOSUART
#if INIT_BAUD

#if MY_BAUD_RATE == 50
#define BAUD_UART_HV 0x09
#define BAUD_UART_LV 0x00
#endif

#if MY_BAUD_RATE == 110
#define BAUD_UART_HV 0x04
#define BAUD_UART_LV 0x17
#endif

#if MY_BAUD_RATE == 150
#define BAUD_UART_HV 0x03
#define BAUD_UART_LV 0x00
#endif

#if MY_BAUD_RATE == 300
#define BAUD_UART_HV 0x01
#define BAUD_UART_LV 0x80
#endif

#if MY_BAUD_RATE == 600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0xc0
#endif

#if MY_BAUD_RATE == 1200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x60
#endif

#if MY_BAUD_RATE == 1800
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x40
#endif

#if MY_BAUD_RATE == 2000
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x3a
#endif

#if MY_BAUD_RATE == 2400
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x30
#endif

#if MY_BAUD_RATE == 3600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x20
#endif

#if MY_BAUD_RATE == 4800
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x18
#endif

#if MY_BAUD_RATE == 7200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x10
#endif

#if MY_BAUD_RATE == 9600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x0c
#endif

#if MY_BAUD_RATE == 19200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x06
#endif

#if MY_BAUD_RATE == 38400
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x03
#endif

#if MY_BAUD_RATE == 115200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x01
#endif

#ifndef BAUD_UART_HV
error_invlaid_8250_baud_rate_see_hrdware_h = BROKEN;
#endif

#endif /* INIT_BAUD */
#endif /* USEDOSUART */

/* Z80 SIO, set for Xerox 820-II and BigBoard I */
#if USEZ80SIO
#define SIO_DATA_PORT 0x04
#define SIO_CMD_PORT 0x06
/* COM 8116 baudrate generator */
#if USECOM8116

#define COM_8116 0x00
/* 8116 standard baud rates */

#if MY_BAUD_RATE == 19200
#define BAUD_UART 0x0f
#define WREGCLK 0x44
#endif

#if MY_BAUD_RATE == 9600
#define BAUD_UART 0x0E
#define WREGCLK 0x44
#endif

#if MY_BAUD_RATE == 4800
#define BAUD_UART 0x0c
#define WREGCLK 0x44
#endif

#if MY_BAUD_RATE == 2400
#define BAUD_UART 0x0a
#define WREGCLK 0x44
#endif

#if MY_BAUD_RATE == 1200
#define BAUD_UART 0x07
#define WREGCLK 0x44
#endif


#else
/* these settings assume the master clock is set to 9600 */

/* 1X clock */
#if MY_BAUD_RATE == 9600
#define WREGCLK 0x04
#endif

/* 16X clock */
#if MY_BAUD_RATE == 4800
#define WREGCLK 0x44
#endif

/* 32X clock */
#if MY_BAUD_RATE == 2400
#define WREGCLK 0x84
#endif

/* 64X clock */
#if MY_BAUD_RATE == 1200
#define WREGCLK 0xc4
#endif


#endif /* USECOM8116 */
#ifndef WREGCLK
error_may_need_to_edit_hrdware_h_macros = BROKEN;
#endif
#endif /* USEZ80SIO */

#if USESILVER
#define BASE_SS 0xDE08


#if MY_BAUD_RATE == 50
#define BAUD_UART_HV 0x09
#define BAUD_UART_LV 0x00
#endif

#if MY_BAUD_RATE == 110
#define BAUD_UART_HV 0x04
#define BAUD_UART_LV 0x17
#endif

#if MY_BAUD_RATE == 150
#define BAUD_UART_HV 0x03
#define BAUD_UART_LV 0x00
#endif

#if MY_BAUD_RATE == 300
#define BAUD_UART_HV 0x01
#define BAUD_UART_LV 0x80
#endif

#if MY_BAUD_RATE == 600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0xc0
#endif

#if MY_BAUD_RATE == 1200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x60
#endif

#if MY_BAUD_RATE == 1800
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x40
#endif

#if MY_BAUD_RATE == 2000
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x3a
#endif

#if MY_BAUD_RATE == 2400
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x30
#endif

#if MY_BAUD_RATE == 3600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x20
#endif

#if MY_BAUD_RATE == 4800
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x18
#endif

#if MY_BAUD_RATE == 7200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x10
#endif

#if MY_BAUD_RATE == 9600
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x0c
#endif

#if MY_BAUD_RATE == 19200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x06
#endif

#if MY_BAUD_RATE == 38400
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x03
#endif

#if MY_BAUD_RATE == 115200
#define BAUD_UART_HV 0x00
#define BAUD_UART_LV 0x01
#endif

#ifndef BAUD_UART_HV
error_invlaid_silversurfer_baud_rate_see_hrdware_h = BROKEN;
#endif

#endif

#endif /* USESIMPLESLIP */

#endif /* HRDWARE_H_SEEN */

