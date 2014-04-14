/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include "config.h"

#if NO_OS
VOIDFIX NO_OS_ftp(VOIDFIX) {
  return;
}

#else

/*
   FTP client.
   Ping and traceroute (normal part of the stack).
   No daemons listening.
*/
#define PROGRAM "CPMFTP"
#define VERSION "0.0.5"
#define EXECUTABLE "FTP"

/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"

/* fixups for HI-TECH C, GCC, and probably other compilers. */
#ifdef HI_TECH_C
#include <sys.h>
#include <unixio.h>
#include <conio.h>
#include <stat.h>
#include <cpm.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef LINUX
#include <asm/io.h>
#endif
#endif

#include "in.h"
#include "netinet.h"
#include <ctype.h>

/* various fixups, os dependent */
#include "compat.h"
#include "netutil.h"
int sock = -1; /* command socket */
int outsock = -1; /* outbound data socket */
int insock = -1; /* inbound data socket */
int servstate = -1;
int port = 21;
unsigned int passport = 0;
int nopassport = 20; /* listen */
int was = 0; /* what command group */
int is = 0; /* the actual reply code */
int zap = -1;
int lpointer = 0;
int dav = 0;	/* server data line ready */
int xferfile; /* i/o file fd */
int sizecount = 0;
int hash = 0; /* hash prints on 128 bytes... */
int outx = 0;
int depth = 0;
char *netaddress;
char *keyboard; /* buffered keystrokes */
char *cmdq; /* what the user typed */
char *iodata; /* what the server typed */
char *smackit; /* line of what the server typed */
char *xferdata; /* data to/from server, one sector */
char *cmdo; /* data TO the server */
unsigned char destination[4];
struct sockaddr_in sato;
FILE *xyz;

/* smaller buffer sizes? */
/* #if ((MAXTAILW) < 1501)
#define BACKUP 1
#else
#define BACKUP 0
#endif
*/

/*
#if ((HIGHBUFFERS) < 8)
#ifndef NANO
#define NANO 1
#endif

#else

#ifndef NANO
#define NANO 0
#endif

#endif
*/

#ifndef NANO
#define NANO 0
#endif

#ifndef BACKUP
#define BACKUP 0
#endif
/* set this to 1 for handy but nonstandard command */
#define FILELISTS 0

#if BACKUP
#define XSN	128
#define BIGGEST 160
#else
#define XSN	256
#define BIGGEST 320
#endif

#ifdef CPM
static char tempfile[] = "0:A:D.$";

#if !NANO
/*
	CP/M makes us work hard.
	According to my manuals, you need to restart
	directory listings between file accesses.
	This may or maynot be true for some versions,
	so this routine makes absolutly certain we don't
	end up with any problems.
*/
int locate(spec, dma, pos)
char * spec;
char * dma;
int pos;
{
	struct fcb * fc;
	int j;
	int first;

	first = j = 0;
	bdos(CPMSDMA, dma); /* set DMA */
	pos++;
	fc = (struct fcb *) netaddress;
	setfcb(fc, spec);
	/* walking dirs is slow and sucks! */
	while((j != -1) && (pos > 0)) {
		pos--;
		j = bdos((!first) ? CPMFFST : CPMFNXT, fc);
		first = 1;
	}
	return(j);
}
#endif

#else
static char tempfile[] = "DATA.FTP";
#endif

#if NANO

#else
#ifdef CPM


/*
	Change directory (change default work user/disk)
	broken, fixme.
*/
void lcd(dir)
char *dir;
{
	struct fcb * fc;
	int un;
	int dl;

	dl = bdos(CPMIDRV, 0) + 'A';

	fc = (struct fcb *) netaddress;
	if(!setfcb(fc, dir)) {
		un = fc->uid;
		if(fc->dr) dl = fc->dr;

		bdos(CPMLGIN, dl);
		bdos(CPMSUID, un);
	}
	/* print the currently logged drive */
	un = bdos(CPMSUID, 0xff);
	dl = bdos(CPMIDRV, 0) + 'A';
	printf("Work drive %i:%c:\n", un, dl);
}



/*

Might as well reuse our own mallocs,
altho we COULD steal the system FCB/buffer...

Make certain you have enough disk space to hold the text of the listing.
A full floppy will cause this to bail!

I should probably make the drive/user assignable within the prompt system.

*/

void stri(p, it)
FILE * p;
char it;
{
	register int cf;

	cf = (int)(it & 0x7f);
	if(cf != 32) fprintf(p, "%c", cf);
}

int dodir(name, f)
char * name;
FILE * f;
{
	struct fcb *fc;
	char *mf;
	char *nptr;
	int i;
	int a;
	int b;
	int j;
	int cf;
	int dl;
	int un;

	nptr = xferdata;
	sprintf(xferdata, "*.*");
	un = bdos(CPMSUID, 0xff);
	fc = (struct fcb *) iodata;
	if(((name[1] == ':') && (strlen(name) < 3)) || ((name[3] == ':') && (strlen(name) < 5))) {
		sprintf(xferdata, "%s*.*", name);
	} else {
		if(strlen(name)) {
			nptr = name;
		}
	}
	dl = 0;
	j = -1;
	if(!f) {
		printf("\nDirectory of %s\n", nptr);
		f = stdout;
	}
	if(!setfcb(fc, nptr)) {
		/* we won't support non disk */
		if(fc->dr) dl = fc->dr + '@';
		i = a = b = j = 0;
		while(i != -1) {
			i = locate(nptr, cmdo, a);
			if(i != -1) {
				mf = cmdo + (i * 32);
				if(((mf[1] & 0x7f) == 'D') && ((mf[2] &0x7f) == ' ') && ((mf[9] & 0x7f) == '$') && ((mf[10] &0x7f) == ' ')) {
					a++;
				} else {
					b++;
					a++;
					if(dl) {
						cf = mf[0] & 0x3f;
						fprintf(f, "%d:%c:", cf, dl);
					}
					for(j = 1; j < 9; j++) {
						stri(f, mf[j]);
					}
					cf = mf[j] & 0x7f;
					if(cf != 32) {
						fprintf(f, ".");
					}
					for(; j < 12; j++) {
						stri(f, mf[j]);
					}
					fprintf(f, "\n");
				}
			}
		}
	}
	printf("%i Files\n", b);
	return(j);
}
#define DODIR 1
#endif /* CPM */

#endif /* NANO */


#ifdef DODIR
#define DIRPRESENT 1
#else
#define DIRPRESENT 0
#endif

void doio(VOIDFIX)
{
	int i;
	int j;
	int k;

	/* inbound command socket */
	if((sock > 0)) {
		zap = AJK_sockread(sock, iodata + lpointer, (BIGGEST - 1) - lpointer);
		if(zap < 0 && !lpointer) {
			sock = -1;
			exit(1);
		}
		if(zap > 0) lpointer += zap;
	}
	if((lpointer > 0) && (!dav)) {
		for (i = 0; i < lpointer; i++) {
			if((iodata[i] == 0x0d) || (iodata[i+1] == 0x0a)) {
				memset(smackit, 0, BIGGEST);
				memmove(smackit, iodata, i); /* copy string */
				smackit[i] = 0x00;
				dav++;
				i++;
				i++;
				lpointer = lpointer - i;
				memmove(iodata, iodata + i, lpointer);
				return; /* bail out here */
			}
		}
	}
	/* NOTE: once read() and write() are unified, this can become one routine */
	/* inbound data */
	while(insock > 0) {
		i = AJK_sockread(insock, xferdata, XSN);
		if(i < 0) {
			AJK_sockclose(insock);
			insock = -1;
			return;
		}
		if((i > 0) && (xferfile > -1)) {
			j = 0;
			while(j != i) {
				k = fwrite(xferdata + j, 1, i - j, xyz);
				if(ferror(xyz)) {
					AJK_sockclose(insock);
					insock = -1;
					return; /* let the caller close the file */
				}
				j = j + k;
			}
			if(hash) {
				sizecount += i;
				while(sizecount > 127) {
					sizecount -= 128;
					putchar('#');
					fflush(stdout);
				}
			}
		}
	}
	/* outbound data */
	while(outsock > 0) {
		i = fread(xferdata, 1, XSN, xyz);
		if(!i) {
			AJK_sockclose(outsock);
			outsock = -1;
			return;
		}
		if(xferfile > -1) {
			j = 0;
			while(j != i) {
				k = AJK_sockwrite(outsock, xferdata + j, i - j);
				if(k < 0) {
					AJK_sockclose(outsock);
					insock = -1;
					return;
				}
				j = j + k;
			}
			if(hash) {
				sizecount += i;
				while(sizecount > 127) {
					sizecount -= 128;
					putchar('#');
					fflush(stdout);
				}
			}
		}
	}
}

void docmd(VOIDFIX)
{
	int i;
	int j;
	int k;

	i = 0;
	j = strlen(cmdo);
	while(i != j) {
		k = AJK_sockwrite(sock, cmdo + i, j - i);
		if(k < 0) {
			doio(); /* this should kill the socket and program */
			exit(-2); /* just incase it doesn't */
		}
		i = i + k;
	}
}

void cmddirect(VOIDFIX)
{
	docmd();
	servstate = -1;
}


/* buffered keystrokes */
void dokeyin(VOIDFIX)
{
	int key;

	if(depth < 126) {
		if(fixkbhit()) {
			key = fixgetch();
			keyboard[depth] = key & 0xff;
			depth++;
		}
	}
}



void sucknum(VOIDFIX)
{
	while(smackit[0] != ',' && smackit[1] && smackit[0] != '(') {
		memmove(smackit, smackit + 1, strlen(smackit) - 1);
	}
	memmove(smackit, smackit + 1, strlen(smackit) - 1);
}

/*
  wait for a non continuation message, printing all messages that apear,
  exit processing
  port passive processing
*/
void waitio(VOIDFIX)
{
	is = servstate;
	servstate = -1;

	while((servstate < 1)) {
		while(!dav) {
			doio();
			dokeyin();
		}
		printf("%s\n", smackit);
		if((smackit[3] != '-') && (strlen(smackit) > 2)) { /* acording to the spec, space is an option */
			servstate = atoi(smackit);
		}
		dav = 0; /* clear data available on wait */
	}
	/* grab pasv here */

	if((servstate == 221) || (servstate == 421) || (servstate == 120)) {
		AJK_sockclose(sock);
		exit(0);
	}
	if(servstate == 227) {
		/* scan till ( */
		sucknum();
		/* manually read in remote address and port (helps broken implementations) */

		/* sscanf(smackit, "%i,%i,%i,%i,%i,%i", h1, h2, h3, h4, p1, p2); */

		destination[0] = atoi(smackit);
		sucknum();
		destination[1] = atoi(smackit);
		sucknum();
		destination[2] = atoi(smackit);
		sucknum();
		destination[3] = atoi(smackit);
		sucknum();
		passport = 256 * atoi(smackit);
		sucknum();
		passport = passport + atoi(smackit);
	}

}

char keystroke(VOIDFIX)
{

	char c;

	while(!depth) {
		doio();
		dokeyin();
	}

	doio();
	c = keyboard[0];
	depth--;
	memmove(keyboard, keyboard + 1, depth);
	return(c);
}

void doconin(quiet)
int quiet;
{
	int i;
	int j;
	int cr;
	char key;

	i = j = cr = 0;
	cmdq[0] = 0x00;


	while(!cr) {
		key = keystroke();
		if((key == 0x08) || (key == 0x7f)) {
			if(i > 0) {
				i--;
				if(!quiet) printf("\b \b");
				fflush(stdout);
			}
		} else {
			if((key == 0x0d) || (key == 0x0a)) {
				cr = 1;
			} else {
				if(i < 126) {
					cmdq[i] = key & 0xff;
					i++;
					if(!quiet) putchar(key);
					fflush(stdout);
				}
			}
		}
	}
	cmdq[i] = 0x00;
	/* whew, we now have the data! */
	putchar('\n');
}

void dopasv(VOIDFIX)
{
	sprintf(cmdo, "PASV\r\n");
	cmddirect();
}

void dohash(VOIDFIX)
{
	hash++;
	hash = 1 & hash; /* cheap xor for crap compiler :-( */
	printf("Hash printing now o");
	if(!hash) {
		printf("ff\n");
	} else {
		printf("n (128 bytes/hash mark)\n");
	}
	fflush(stdout);
	servstate = 200;
}

void cqtmp(VOIDFIX)
{
	sprintf(cmdq, "%s", tempfile);
}

void dogetput(dir)
int dir;
{
	int i;
	int j;

	sizecount = 0;
	sprintf(&cmdo[0], "TYPE I\r\n");
	if(dir > 1) sprintf(&cmdo[0], "TYPE A\r\n");
	cmddirect();
	waitio();
	dopasv();
	waitio();
	if(servstate != 227) return;
	sprintf(cmdo, "RETR %s\r\n", cmdq);
	if(!dir) sprintf(cmdo, "STOR %s\r\n", cmdq);
	if(dir == 2) sprintf(cmdo, "LIST %s\r\n", cmdq);
	if(dir == 3) {
		sprintf(cmdo, "NLST %s\r\n", cmdq);
		dir = 1;
		cqtmp();
	}
	/* open passive connection */
	j = AJK_socket(AF_INET, SOCK_STREAM, 0);
	if(j < 0) {
		return;
	}
        sato.sin_port = passport;
        i=AJK_connect(j, (struct sockaddr_in *)&sato, sizeof (sato));

	if(i < 0) {
		return;
	}
	docmd();
	waitio();
	if(servstate != 150) {
		AJK_sockclose(j);
		return;
	}
	i = hash;
	if(!dir) {
		outsock = j;
		if(!(xyz = fopen(cmdq, "rb"))) {
			printf("Can't open file %s\n", cmdq);
			xferfile = -1;
			/* close socket caught below... */
		} else {
			xferfile = fileno(xyz);
		}
		if(xferfile < 0) {
			AJK_sockclose(outsock);
			outsock = -1;
		}
	}
	if(dir == 1) {
		insock = j;
		if(!(xyz = fopen(cmdq, "wb"))) {
			printf("Can't create file %s\n",cmdq);
			xferfile = -1;
			/* close socket caught below... */
		} else {
			xferfile = fileno(xyz);
		}
		if(xferfile < 0) {
			AJK_sockclose(insock);
			insock = -1;
		}
	}
	if(dir == 2) {
		insock = j;
		xferfile = fileno(stdout); /* stdout */
		xyz = stdout;
		hash = 0;
	}
	while(insock > 0 || outsock > 0) {
		doio();
	}
	hash = i;
	putchar('\n');
	if(!dir || dir == 1) fclose(xyz); /* don't close stdout */
	servstate = -1;
	xyz = NULL;
}

void dofio(dir)
int dir;
{
	FILE *pdq;
	int i;
	int j;
	int k;

	sprintf(netaddress, "%s", cmdq);
	if(!(pdq = fopen(netaddress, "ra"))) {
		printf("No file list\n");
		return;
	}
	i = k = 0;
	fclose(pdq);

	/* read in lines, submit them as cmdq */
	while (i > -1) {
		pdq = fopen(netaddress, "ra");
		j = 0;
		while(j < k) {
			i = fscanf(pdq, "%s", cmdq);
			j++;
		}
		k++;
		i = fscanf(pdq, "%s", cmdq);
		fclose(pdq);
		if(i > 0) {
			dogetput(dir);
			if(servstate < 0) waitio();
		}
	}
}

void killtemp(VOIDFIX)
{
	unlink(tempfile);
}

#if DIRPRESENT
/*
 * mput ...
 */
void dofwio(VOIDFIX)
{
	FILE *pdq;
	pdq = fopen(tempfile, "w");
	dodir(cmdq, pdq);
	fclose(pdq);
	cqtmp();
	dofio(0);
	killtemp();
}
#endif

void stripspaces(VOIDFIX)
{
	while(isspace(cmdq[0]) && (cmdq[0])) {
		memmove(cmdq, cmdq + 1, strlen(cmdq));
	}
}

void quit(VOIDFIX)
{
	sprintf(cmdo, "QUIT\r\n");
	docmd();
	waitio(); /* get end message */
	waitio(); /* wait for socket to die off */
}

void mget(VOIDFIX)
{
	dogetput(3);
	if(servstate < 0) waitio();
	cqtmp();
	dofio(1);
	killtemp();
}
#if DIRPRESENT
void dodirfile(VOIDFIX)
{
	dodir(cmdq, NULL);
}
#endif
struct coms {
	char *text;
	int want;
	void *(*func)();
	int pass;
	char *str;
};


char LSCMD[]="-la %s";
char CWD[]="CWD %s\r\n";

/* all pass cmdq, which is global */
struct coms comms[] = {
	{ "mget",	1,	(void *)mget,		0,	NULL },
	{ "get",	1,	(void *)dogetput,	1,	NULL },
	{ "put",	1,	(void *)dogetput,	0,	NULL },
	{ "cd",		1,	(void *)cmddirect,	0,	CWD },
	{ "cwd",	1,	(void *)cmddirect,	0,	CWD },
#if DIRPRESENT
	{ "mput",	1,	(void *)dofwio,		0,	NULL },
	{ "lcd",	1,	(void *)lcd,		0,	NULL },
	{ "lls",	0,	(void *)dodirfile,	0,	NULL },
#endif
#if FILELISTS
	{ "fput",	0,	(void *)dofio,		0,	NULL },
	{ "fget",	0,	(void *)dofio,		1,	NULL },
#endif
	{ "hash",	0,	(void *)dohash,		0,	NULL },
	{ "quit",	0,	(void *)quit,		0,	NULL },
	{ "bye",	0,	(void *)quit,		0,	NULL },
	{ "exit",	0,	(void *)quit,		0,	NULL },
	{ "pwd",	0,	(void *)cmddirect,	0,	"PWD\r\n" },

	{ "ls",		0,	(void *)dogetput,	2,	LSCMD },
	{ "dir",	0,	(void *)dogetput,	2,	LSCMD },

	{ NULL,		0,	NULL,		0,	NULL }
};

void doparse(VOIDFIX)
{
	register int i;
	register int l;
	/*
	   chop off first part by changing space to a 0x00,
	   after we know what/where the command is we memmove the data
	*/
	/* strip leading spaces */
	stripspaces();
	l = strlen(cmdq);
	if(l) {
		cmdq[l + 1] = 0x00;
		i = 0;
		while((!isspace(cmdq[i])) && (cmdq[i])) {
			i++;
		}
		l++;
		cmdq[i] = 0x00; /* chop it */
		sprintf(cmdo, "%s", cmdq);
		i++;
		memmove(cmdq, cmdq + i,l - i + 1);
		/* strip leading spaces, again */
		stripspaces();

		l = strlen(cmdq);
		/* lower case typed command */
		for (i = 0; i < (int)strlen(cmdo); i++) {
			cmdo[i] = tolower(cmdo[i]);
		}

		i = 0;
		while(comms[i].text) {
			if(!strcmp(cmdo, comms[i].text)) {
				if((!l) && (comms[i].want)) {
					doconin(0);
				}
				if(comms[i].str) {
					sprintf(cmdo, comms[i].str, cmdq);
					sprintf(cmdq, cmdo);
				}
				(comms[i].func)(comms[i].pass, NULL);
				break;
			}
			i++;
		}
	}

}

void doprompt(VOIDFIX)
{
#ifdef CPM
	register int un;
	register int dl;
	un = bdos(CPMSUID, 0xff);
	dl = bdos(CPMIDRV, 0) + 'A';
	printf("[%i:%c]ftp>",un, dl);
#else
	printf("ftp>");
#endif
	fflush(stdout);
	doconin(0);
	doparse();
}

void usage(argv)
char *argv[];
{
#define USE "the.network.IP.address nick [port]"
	AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

int main(argc, argv)
int argc;
char *argv[];
{
	register struct u_nameip *result;
	register int xkb;
	register unsigned int port;

	port = 21;
#if NANO
	/* we don't do anything extra */
#else
#if DIRPRESENT
	/* no warning */
#else
#if !BACKUP
	printf("WARNING: lcd, lls and mput disabled.\nYou need to code dodirfile() to list your disks.\n");
#endif /* !BACKUP */
#endif /* DIRPRESENT */
#endif /* NANO */
	if(argc < 2 || argc > 3) usage(argv);
	netaddress = safemalloc(80);
	if(argc > 2) {
		sprintf(netaddress, "%s\n", argv[2]);
		port = atoi(netaddress) & 0xffff;
		if(port < 1 ) usage(argv);
	}
/* IMPORTANT: initialize the network! */
	initnetwork(argc, argv);

/* need a smaller alternative here for NANO */
	/* snag out the net address.... */
	result = findhost(argv[1],1);
	if(!result) {
		printf("No such host\n");
	} else {

		memcpy(destination,result->hostip,4);
		FREE(result->hostip);
		FREE(result->hostname);
		FREE(result);

		sock = AJK_socket(AF_INET, SOCK_STREAM, 0);
		if(sock == -1) {
			printf("No socket!\n");
		}
		if(sock > -1) {
                                memcpy(&sato.sin_addr, destination, 4);
                                sato.sin_port = port;
                                zap=AJK_connect(sock, (struct sockaddr_in *)&sato, sizeof (sato));
                }
	}
	if(zap > 0) {
		printf("Connected.\n");
	} else {
		printf("Service Not available\n");
	}

	keyboard = safemalloc(10);
	cmdq = safemalloc(160);
	xferdata = safemalloc(XSN);
	cmdo = safemalloc(160);
	iodata = safemalloc(BIGGEST);
	smackit = safemalloc(BIGGEST);

	if((!keyboard ) || (!cmdq) || (!xferdata) || (!cmdo) || (!iodata) || (!smackit)) {
		printf("Not enough memory\n");
		exit(1);
	}
	xkb = 0;
	while(sock > 0) {
		/*

		The macro `MAKEMEBLEED' should be sprinkled througout the app code
		especially after a time consuming loop for latency reduction, like so:

		( some c code, like a loop or something... )
		MAKEMEBLEED();
		( more c code... )

		However if you are consistantly doing reads in a loop, this is done for you! :-)
		Writes _ALSO_ do this too, but that's because it's designed to and has to!
		A NULL write will cause the transport layer to hit once. SAME for a NULL read.
		THE SOCKET NEED NOT BE OPEN OR FUNCTIONING OR EVEN CORRECT.

		example:

		sockread(-1, NULL, 0);
		is as valid as
		sockwrite(-1 ,NULL, 0);

		The file descriptor number does _NOT_ matter for it to work a POLL operation.
		The latency involved will be greater, though. (an extra call and compare is done)

		The best way to make interactive programs work better is to read and process large chunks.
		Simple loops of reading 1 from socket will cause huge amounts of cpu load and latency.

		*/
		if(servstate < 0) waitio();
		if((servstate == 220) || (servstate == 530)) {
			printf("User Name:");
			fflush(stdout);
			doconin(0);
			sprintf(cmdo, "USER %s\r\n", cmdq);
			cmddirect();
		} else if(servstate == 230) {
			dohash();
			sprintf(cmdo, "SYST\r\n");
			cmddirect();
		} else if(servstate == 331) {
			printf("Password:");
			fflush(stdout);
			doconin(1);
			sprintf(cmdo, "PASS %s\r\n", cmdq);
			cmddirect();
		} else if(servstate == 332) {
			printf("Account Password:");
			fflush(stdout);
			doconin(1);
			sprintf(cmdo, "ACCT %s\r\n", cmdq);
			cmddirect();
		} else if(servstate > 199) {
			doprompt();
		}
	}
/*
	if(!sock) {
		printf("\n\nConection terminated\n");
	} else {
		printf("\n\nConection terminated with an error\n");
	}
*/
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


0000 ftp.com ftp.obj xxPROGRAMxx

*/
