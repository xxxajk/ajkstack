/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include "config.h"
#if NO_OS
VOIDFIX NO_OS_nsl(VOIDFIX) {
  return;
}

#else
/*
	very simple nslookup.
*/

/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* fixups for HI-TECH C, GCC, and probably other compilers. */
#ifdef HI_TECH_C
#include <sys.h>
#include <unixio.h>
#include <conio.h>
#include <stat.h>
/* #include <cpm.h> */
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef LINUX
#include <asm/io.h>
#endif
#endif

#include "in.h"
#include "netinet.h"
#include "socket.h"
#include <ctype.h>

/* various fixups, os dependent */
#include "compat.h"
#include "netutil.h"

#define PROGRAM "CPMNSL"
#define VERSION "0.0.1"
#define EXECUTABLE "NSL"


int main(argc, argv)
int argc;
char *argv[];
{
	struct u_nameip *result;

/* IMPORTANT: initialize the network! */
	initnetwork(argc, argv);
	MAKEMEBLEED();
	printf("Looking up %s\n", argv[1]);
	result = findhost(argv[1], 0);
	if(!result) {
		printf("can't find it\n");
	} else {
		printf("   Name: %s\n", result->hostname);
		printf("Address: %i.%i.%i.%i\n", (short)(result->hostip[0] & 0xff), (short)(result->hostip[1] & 0xff), (short)(result->hostip[2] &0xff), (short)(result->hostip[3] & 0xff));
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


0000 nsl.com nsl.obj xxPROGRAMxx

*/
