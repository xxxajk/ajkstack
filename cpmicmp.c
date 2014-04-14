/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include "config.h"

#if NO_OS
VOIDFIX NO_OS_cpmicmp(VOIDFIX) {
  return;
}

#else
/*
   This program doesn't actually do anything, it just loops.
   Ping and traceroute should work.
   Consider it as just a stack, that has no daemons listening.
   It's really to bad that cp/m has no real interrupt facility
   that is universal...
   else this could have been the actual stack scheduler...
   There is a possibly of wrapping each BDOS or BIOS call
   with a call to the scheduler tho.

   On multiuser systems, this demo code will be a good
   beginning point to implement a MUX.

*/

#define PROGRAM "CPMICMP"
#define VERSION "0.0.2"
#define EXECUTABLE "ICMP"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "in.h"
#include "netinet.h"
#include "socket.h"
/* various fixups, os dependent */
#include "compat.h"

int main(argc, argv)
int argc;
char *argv[];
{
	int dotcount;

	/* IMPORTANT: initialize the network! */
	initnetwork(argc, argv);

	/*
	Unfortunately there is no alarm(); on some platforms (CP/M)
	so we must basically make a simulation of it by sprinkling
	IO and transport calls arround :-(
	*/
	dotcount = 0;
	for(;;) {
		 /* this should be sprinkled througout the app code
		 especially after a time consuming loop for icmp latency */
		MAKEMEBLEED();
		dotcount++;
		/* one dot / sec timer test, it's not accurate :-) */
		if(dotcount == TIMETICK) {
			dotcount = 0;
			putchar('.');
			fflush(stdout);
		}
	}
#ifndef CPM
#ifndef z80
#ifndef HI_TECH_C
	exit(0);
#endif
#endif
#endif
}

#endif /* NO_OS */

/* tell link parser who we are and where we belong

0000 cpmicmp.com cpmicmp.obj xxPROGRAMxx

*/
