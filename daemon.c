/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

#include "config.h"

#if NO_OS
VOIDFIX NO_OS_daemon(VOIDFIX) {
  return;
}

#else
/*
   This program is an echo daemon on port 7.
*/
#define PROGRAM "CPMDAEMON"
#define VERSION "0.0.0"
#define EXECUTABLE "DAEMON"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "in.h"
#include "netinet.h"
#include "socket.h"
/* various fixups, os dependent */
#include "compat.h"


#define MAXSTREAMS 100

int main(argc, argv)
int argc;
char *argv[];
{
	struct sockaddr_in srv_addy;
	char *addr;
	char *info;
	int dotcount;
	int sock;
	int avail;
	int now;
	int i;
	int j;
	int k;
	int stream[MAXSTREAMS];

	/* IMPORTANT: initialize the network! */
	initnetwork(argc, argv);

	info = safemalloc(256);
	if(!info) {
		printf("No RAM!\n");
		exit(-1);
	}
	srv_addy.sin_port = 7; /* echo port */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	dotcount = bind(sock, (struct sockaddr_in *)&srv_addy, sizeof(srv_addy));
	if(dotcount < 0) {
		printf("\nError on bind %i\n", dotcount);
		return(-1);
	}

	dotcount = listen(sock, MAXSTREAMS);
	if(dotcount < 1) {
		 printf("\nError on listen %i\n", dotcount);
		 return(-1);
	}

	printf("socket # %i. %i sockets available, Waiting for test connection.\n", sock, dotcount);
	avail = dotcount;

	for(i = 0; i < MAXSTREAMS; i++) {
		stream[i] = -1;
	}

	dotcount = 0;

	for(;;) {
		if(fixkbhit()) {
			now = fixgetch();
			if(now == 3) {
				putchar('\n');
				exit(0);
			}
		}
		dotcount++;
		/* one dot / sec timer test, it's not accurate :-) */
		if(dotcount == TIMETICK) {
			dotcount = 0;
			putchar('.');
			fflush(stdout);
		}
		now = bindcount(sock);
		if(now < 0) {
			printf("bindcount() Error %i\n", now);
			exit(now);
		}
		if( now != avail) printf("\n%i sockets available on socket %i.\n", now, sock);
		avail = now;
		now = acceptready(sock);
		if(now < 0) {
			printf("Acceptready() error %i\n", now);
			exit(now);
		}
		if(now == 1) {
			printf("Accepting incoming connection.\n");
			i = 0;
			while(i < MAXSTREAMS) {
				if(stream[i] == -1) break;
				i++;
			}
			stream[i] = accept(sock, (struct sockaddr_in *)&srv_addy, &j);
			if(stream[i] < 1) {
				printf("Accept nonfatal error %i\n", stream[i]);
				stream[i] = -1;
			} else {
				addr=(char *)&srv_addy.sin_addr;
				printf("\nStream %i(%i). Connection from %i.%i.%i.%i\n", i, stream[i], addr[0] & 0xff, addr[1] & 0xff, addr[2] & 0xff, addr[3] & 0xff);
			}
		}
		/* test each stream for data and echo it back */
		for(i = 0; i < MAXSTREAMS; i++) {
			if(stream[i] > 0) {
				j = sockread(stream[i], info, 256);
				if(j < 0) {
					printf("Socket %i closed (read)\n", i);
					sockclose(stream[i]);
					stream[i] = -1;
				} else {
					now = 0;
					while(now < j) {
						k = sockwrite(stream[i], info + now, j - now);
						if(k < 0) {
							printf("Socket %i closed (write)\n", i);
							sockclose(stream[i]);
							stream[i] = -1;
							now = j + 1;
						} else {
							now = now + k;
						}
					}
				}
			}
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


*/
