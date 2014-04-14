/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include "config.h"
#if NO_OS
VOIDFIX NO_OS_update(VOIDFIX) {
  return;
}

#else

/*
   Automatic update/bootstrap.
   Ping and traceroute (normal part of the stack).
   No daemons listening.
*/
/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* fixups for HI-TECH C, GCC, and probably other compilers. */
#ifdef HI_TECH_C
#include <sys.h>
#include <unixio.h>
#include <conio.h>
#else
#ifdef LINUX
#include <asm/io.h>
#endif
#endif

#include "in.h"
#include "netinet.h"
#include "socket.h"
/* various fixups, os dependent */
#include "compat.h"
#include "netutil.h"
#include "tcpvars.h"

#define PROGRAM "UPDATE"
#define VERSION "0.0.2"
#define EXECUTABLE "UPDATE"

int sock = -1;
char urlbase[80];
char url[81];
char local[80];
char uri[96];
struct sockaddr_in sato;


int grabline(void)
{
	char *ptr;
	int zap;
	int i;

	zap = -1;
	i = 1;
	ptr = uri;

	while(i < 96) {
		zap = sockread(sock, ptr, 1);
		if(zap < 0) {
			break;
		}
		if(zap > 0) {
			if(*ptr == 0x0a) break;
			if(*ptr != 0x0d) {
				i++;
				ptr++;
			}
		}
	}

	*ptr = 0x00;
	return zap;
}

int grabfile()
{
	int zap;
	int i;
	FILE *dl;

	zap = -1;
	i = 1;

	while(strlen(uri)) {
		zap = grabline();
		if(zap < 0) break;
	}
	if(zap > -1) {
		/* read in file and save it */
		dl = fopen(local, "wb");
		if(!dl) {
			zap = -1;
			perror(local);
		} else {

			while(i > -1) {
				i = sockread(sock, uri, 96);
				if(i > 0) {
					printf(".");
					fflush(stdout);
					zap = fwrite(uri, 1, i, dl);
					if(zap < 0) break;
				}
			}
			fclose(dl);
			if(zap > -1) {
				printf("Done\n");
			}
		}
	}

	return(zap);
}

int doreq(void)
{
	int zap;
	char *ptr;

	zap = AJK_sockwrite(sock, uri, strlen(uri));
	if(zap > -1) {
		sprintf(uri, "User-Agent: %s/%s (AJKstack/%s, %s, %s)\n\n", PROGRAM, VERSION, STACK_IS, OS, CPU);
		zap = AJK_sockwrite(sock, uri, strlen(uri));
	}
	if(zap > -1) {
		zap = grabline();
	}
	if(zap > -1) {
		/* check the result, we must see a 200 here */
		ptr = uri;
		while(*ptr != ' ') ptr++;
		ptr++;
		zap = -1;
		if(*ptr == '2') {
			ptr++;
			if(*ptr == '0') {
				ptr++;
				if(*ptr == '0') {
					printf("%s\n", uri);
					zap = grabfile();
				}
			}
		}
	}
	return(zap);
}



int getfile(host, port, dest)
char *host;
int port;
unsigned char *dest;
{
	int zap;

	zap = -1;

	printf("Contacting %s...", host);
	zap = AJK_socket(AF_INET,SOCK_STREAM, 0);
	if(zap > -1) {
		sock = zap;
                memcpy(&sato.sin_addr, destination, 4);
                sato.sin_port = port;
                zap=AJK_connect(sock, (struct sockaddr_in *)&sato, sizeof (sato));
		if(zap > -1) {
			printf("\nRequesting file %s...", url);
			sprintf(uri, "GET %s HTTP/1.0\n", url);
			zap = doreq();
		}
		sockclose(sock);
	}

	return(zap);
}

void usage(argv)
char *argv[];
{
#define USE "dr.ea.ms [Y]\n\tthe Y switch implies that you absolutly want to download.\n\tOnly use it if this is your first install\n\tor really want to download again."
	AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

void erro(VOIDFIX)
{
	printf("Could not get file `%s'.\n", local);
}

int main(argc, argv)
int argc;
char *argv[];
{
	struct u_nameip *result;
	int zap;
	int dotcount;
	int port;
	int yes;
	FILE *ls;

	if(argc < 2) usage(argv);

	zap = -1;
	dotcount = yes = 0;
	port = 80;

	if(argc >2) {
		if(*argv[2] != 'Y') usage(argv);
		yes = 1;
	}
/* pre check rc file, if none, prompt to make one */

/* IMPORTANT: initialize the network! */
	initnetwork(argc, argv);


	/* snag out the net address.... */
	result = findhost(argv[1], 1);
	if(!result) {
		printf("No such host\n");
	} else {

		/* take each one into a byte :-) */
		sprintf(urlbase, "/update/%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x00000000/", USESIMPLESLIP, USEARCNET, HAVE_RTC, USEZ80DART, USECPM3AUX, USELINUXTTYS, USEELKSTTYS, USEDOSUART, USEZ80SIO, USECOM8116, USESILVER, USEACIA);
		sprintf(local, "current.txt");
		sprintf(url, "%s%s", urlbase, local);
		zap = getfile(result->hostname, port, result->hostip);
		if(zap < 0) erro();

		/* check current agenst this version of update */
		ls = fopen(local, "rb");
		if(!ls) {
			perror(local);
			goto bail;
		}
		zap = fscanf(ls, "%s ", local);
		fclose(ls);
		/* if newer available, grab the new version and run it with the Y switch */
		if(strcmp(local, STACKVERSION)) yes = 1;
		/* else display the internal and external versions and */
		/* ask user if they want to upgrade IF the Y switch was not seen and IF there is no new version! */
		if(!yes) {
			printf("Current version matches the UPDATE program.\n");
			printf("Do you wish to continue to download? (y/n)");
			do {
				if(fixkbhit()) {
					yes = tolower(fixgetch());
				}
			} while (yes != (int)'y' && yes != (int)'n');
			if(yes == (int)'n') {
				printf("n\nAborted.\n");
				exit(0);
			}
			putchar('y');
		}

		printf("\nContinuing update\n");
		sprintf(local, "files.txt");
		sprintf(url, "%s%s/%s", urlbase, STACKVERSION, local);


		zap = getfile(result->hostname, port, result->hostip);
		if(zap < 0) erro();

		ls = fopen(local, "rb");
		if(!ls) {
			perror(local);
			goto bail;
		}

		while(zap > -1) {
			if(dotcount) break;
			zap = fscanf(ls, "%s\n", local);
			if(zap > -1) {
				if(!strncmp(local, "update", 6)) dotcount = 1;
				if(!strcmp(local, "END")) {
					zap = -1;
				}
			}
			if(zap > -1) {
				zap = -1;
				sprintf(url, "%s%s/%s", urlbase, STACKVERSION, local);
				do {
					zap = getfile(result->hostname, port, result->hostip);
				} while(zap < 0);
			}
		}
		fclose(ls);
		if(dotcount) {
			printf("\n\n\n\n\n\n\n\n\n\nUpdate updated, restarting new update...\n");
			/* setfcb(DFCB, local); */
			local[6] = 0x00;
			execl(local, local, argv[1], "Y", (char *)NULL);
			printf("Can't execute %s\n", local);
		}
	}
bail:
		FREE(result->hostip);
		FREE(result->hostname);
		FREE(result);

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


0000 update.com update.obj xxPROGRAMxx

*/

