/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
 * TO-DO: port directory stuff, model after CP/M.
 *
 */

#include <Arduino.h>
// These are needed for auto-config and Arduino
#include <Wire.h>
#include <SPI.h>

#ifndef USE_MULTIPLE_APP_API
#define USE_MULTIPLE_APP_API 16
#endif

#include <xmem.h>
#include <RTClib.h>
#include <usbhub.h>
#include <masstorage.h>
#include <stdio.h>
#include <xmemUSB.h>
#include <xmemUSBFS.h>
#include <Storage.h>
#include <ajkstack.h>
#include <alloca.h>

USB Usb;
USBHub Hub(&Usb);


#define PROGRAM "CPMTELNET"
#define VERSION "0.0.5"
#define EXECUTABLE "TELNET"
#define USE "the.network.IP.address [port]"
#define INITIAL_MAXARGC 8
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/* some ugly globals to speed up things */
unsigned char keyboard[1025];
char clibuf[40];

void killline(void) {
        memset(clibuf, 0, 40);
        memset(clibuf, ' ', 38);
        printf("\r%s\r", clibuf);
}

void restoreline(int kbptr) {
        int add;

        add = 0;
        killline();
        if(kbptr > 38) add = kbptr - 38;
        memset(clibuf, 0, 40);
        strncat(clibuf, (char *)&keyboard[add], 38);
        printf("\r%s", clibuf);
}



#define PROGRAM "CPMFTP"
#define VERSION "0.0.5"
#define EXECUTABLE "FTP"

int sock = -1; /* command socket */
int outsock = -1; /* outbound data socket */
int insock = -1; /* inbound data socket */
int servstate = -1;
int port = 21;
unsigned short passport = 0;
int nopassport = 20; /* listen */
int was = 0; /* what command group */
int is = 0; /* the actual reply code */
int zap = -1;
int lpointer = 0;
int dav = 0; /* server data line ready */
int xferfile; /* i/o file fd */
int sizecount = 0;
int hash = 0; /* hash prints on 128 bytes... */
int outx = 0;
int depth = 0;
char *netaddress;
char *cmdq; /* what the user typed */
char *iodata; /* what the server typed */
char *smackit; /* line of what the server typed */
char *xferdata; /* data to/from server, one sector */
char *cmdo; /* data TO the server */
unsigned char destination[4];
int xyz;

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
        fc = (struct fcb *)netaddress;
        setfcb(fc, spec);
        /* walking dirs is slow and sucks! */
        while((j != -1) && (pos > 0)) {
                pos--;
                j = bdos((!first) ? CPMFFST : CPMFNXT, fc);
                first = 1;
        }
        return (j);
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

        fc = (struct fcb *)netaddress;
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
        fc = (struct fcb *)iodata;
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
        return (j);
}
#define DODIR 1
#endif /* CPM */

#endif /* NANO */


#ifdef DODIR
#define DIRPRESENT 1
#else
#define DIRPRESENT 0
#endif

void doio(VOIDFIX) {
        int i;
        int j;
        int k;

        /* inbound command socket */
        if(sock > -1) {
                zap = socket_read(sock, iodata + lpointer, (BIGGEST - 1) - lpointer);
                if(zap < 0 && !lpointer) {
                        socket_close(sock);
                        sock = -1;
                        return;
                }
                if(zap > 0) lpointer += zap;
        }
        if((lpointer > 0) && (!dav)) {
                for(i = 0; i < lpointer; i++) {
                        if((iodata[i] == 0x0d) || (iodata[i + 1] == 0x0a)) {
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
                i = socket_read(insock, xferdata, XSN);
                if(i < 0) {
                        socket_close(insock);
                        insock = -1;
                        return;
                }
                if((i > 0) && (xferfile > -1)) {
                        j = 0;
                        while(j != i) {
                                k = write(xyz, xferdata + j, i - j);
                                if(k < 0) {
                                        socket_close(insock);
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
                i = read(xyz, xferdata, XSN);
                if(i < 1) {
                        socket_close(outsock);
                        outsock = -1;
                        return;
                }
                if(xferfile > -1) {
                        j = 0;
                        while(j != i) {
                                k = socket_write(outsock, xferdata + j, i - j);
                                if(k < 0) {
                                        socket_close(outsock);
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

void docmd(VOIDFIX) {
        int i;
        int j;
        int k;

        i = 0;
        j = strlen(cmdo);
        while(i != j) {
                k = socket_write(sock, cmdo + i, j - i);
                if(k < 0) {
                        socket_close(sock);
                        sock = -1;
                        return;
               }
                i = i + k;
        }
}

void cmddirect(VOIDFIX) {
        docmd();
        servstate = -1;
}

/* buffered keystrokes */
void dokeyin(VOIDFIX) {
        int key;

        if(depth < 126) {
                if(fixkbhit()) {
                        key = fixgetch();
                        keyboard[depth] = key & 0xff;
                        depth++;
                }
        }
}

void sucknum(VOIDFIX) {
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
void waitio(VOIDFIX) {
        is = servstate;
        servstate = -1;

        while((servstate < 1)) {
                while(!dav) {
                        doio();
                        if(sock < 0) return;
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
                        socket_close(sock);
                        sock = -1;
                        return;
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

char keystroke(VOIDFIX) {

        char c;

        while(!depth) {
                doio();
                dokeyin();
        }

        doio();
        c = keyboard[0];
        depth--;
        memmove(keyboard, keyboard + 1, depth);
        return (c);
}

void doconin(int quiet) {
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

void dopasv(VOIDFIX) {
        sprintf(cmdo, "PASV\r\n");
        cmddirect();
}

void dohash(VOIDFIX) {
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

void cqtmp(VOIDFIX) {
        sprintf(cmdq, "%s", tempfile);
}

void dogetput(int dir) {
        int i;
        int j;
        struct sockaddr_in to;

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
        j = socket(AF_INET, SOCK_STREAM, 0);
        if(j < 0) {
                return;
        }
        to.sin_port = passport;
        memcpy(&to.sin_addr, destination, 4);
        i = connect(j, (struct sockaddr *)&to, sizeof (to));
        if(i < 0) {
                return;
        }
        docmd();
        waitio();
        if(servstate != 150) {
                socket_close(j);
                return;
        }
        i = hash;
        sprintf(cmdo, "/%s", cmdq);
        if(!dir) {
                outsock = j;
                if((xyz = open(cmdo, O_RDONLY)) < 1) {
                        printf("Can't open file %s\n", cmdo);
                        xferfile = -1;
                        /* close socket caught below... */
                } else {
                        xferfile = xyz;
                }
                if(xferfile < 0) {
                        socket_close(outsock);
                        outsock = -1;
                }
        }
        if(dir == 1) {
                insock = j;
                if((xyz = open(cmdo, O_WRONLY | O_CREAT)) < 1) {
                        printf("Can't create file %s\n", cmdo);
                        xferfile = -1;
                        /* close socket caught below... */
                } else {
                        xferfile = xyz;
                }
                if(xferfile < 0) {
                        socket_close(insock);
                        insock = -1;
                }
        }
        if(dir == 2) {
                insock = j;
                xferfile = 1; /* stdout */
                xyz = 1;
                hash = 0;
        }
        while(insock > 0 || outsock > 0) {
                doio();
        }
        hash = i;
        putchar('\n');
        if(dir != 2) close(xyz); /* don't close stdout */
        servstate = -1;
        xyz = -1;
}

int readaline(int fd, char *buf) {
        int r = 1;
        int c = 0;
        *buf = 0x00;
        while(r > 0) {
                r = read(fd, buf, 1);
                if(r < 1) {
                        *buf = 0x00;
                        return c;
                } else {
                        if(*buf == 0x0d || *buf == 0x0a) {
                                *buf = 0x00;
                                return c;
                        }
                        c = 1;
                        buf++;
                }
        }

        return c;
}

void dofio(int dir) {
        int pdq;
        int i;
        int j;
        int k;

        sprintf(netaddress, "%s", cmdq);
        if((pdq = open(netaddress, O_RDONLY))) {
                printf("No file list\n");
                return;
        }
        i = 0;
        k = 0;
        close(pdq);

        /* read in lines, submit them as cmdq */
        while(i > -1) {
                pdq = open(netaddress, O_RDONLY);
                if(pdq < 1) return;
                j = 0;
                while(j < k) {
                        i = readaline(pdq, cmdq);
                        j++;
                }
                k++;
                i = readaline(pdq, cmdq);
                close(pdq);
                if(i > 0) {
                        dogetput(dir);
                        if(servstate < 0) waitio();
                }
        }
}

void killtemp(VOIDFIX) {
        unlink(tempfile);
}

#if DIRPRESENT

/*
 * mput ...
 */
void dofwio(VOIDFIX) {
        FILE *pdq;
        pdq = fopen(tempfile, "w");
        dodir(cmdq, pdq);
        fclose(pdq);
        cqtmp();
        dofio(0);
        killtemp();
}
#endif

void stripspaces(VOIDFIX) {
        while(isspace(cmdq[0]) && (cmdq[0])) {
                memmove(cmdq, cmdq + 1, strlen(cmdq));
        }
}

void quit(VOIDFIX) {
        sprintf(cmdo, "QUIT\r\n");
        docmd();
        waitio(); /* get end message */
        waitio(); /* wait for socket to die off */
}

void mget(VOIDFIX) {
        dogetput(3);
        if(servstate < 0) waitio();
        cqtmp();
        dofio(1);
        killtemp();
}

#if DIRPRESENT

void dodirfile(VOIDFIX) {
        dodir(cmdq, NULL);
}
#endif

struct coms {
        char *text;
        int want;
        void *(*func)(int, char*);
        int pass;
        char *str;
};


char LSCMD[] = "-la %s";
char CWD[] = "CWD %s\r\n";

/* all pass cmdq, which is global */
struct coms comms[] = {
        { "mget", 1, (void* (*)(int, char*))mget, 0, NULL},
        { "get", 1, (void* (*)(int, char*))dogetput, 1, NULL},
        { "put", 1, (void* (*)(int, char*))dogetput, 0, NULL},
        { "cd", 1, (void* (*)(int, char*))cmddirect, 0, CWD},
        { "cwd", 1, (void* (*)(int, char*))cmddirect, 0, CWD},
#if DIRPRESENT
        { "mput", 1, (void* (*)(int, char*))dofwio, 0, NULL},
        { "lcd", 1, (void* (*)(int, char*))lcd, 0, NULL},
        { "lls", 0, (void* (*)(int, char*))dodirfile, 0, NULL},
#endif
#if FILELISTS
        { "fput", 0, (void* (*)(int, char*))dofio, 0, NULL},
        { "fget", 0, (void* (*)(int, char*))dofio, 1, NULL},
#endif
        { "hash", 0, (void* (*)(int, char*))dohash, 0, NULL},
        { "quit", 0, (void* (*)(int, char*))quit, 0, NULL},
        { "bye", 0, (void* (*)(int, char*))quit, 0, NULL},
        { "exit", 0, (void* (*)(int, char*))quit, 0, NULL},
        { "pwd", 0, (void* (*)(int, char*))cmddirect, 0, "PWD\r\n"},

        { "ls", 0, (void* (*)(int, char*))dogetput, 2, LSCMD},
        { "dir", 0, (void* (*)(int, char*))dogetput, 2, LSCMD},

        { NULL, 0, NULL, 0, NULL}
};

void doparse(VOIDFIX) {
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
                memmove(cmdq, cmdq + i, l - i + 1);
                /* strip leading spaces, again */
                stripspaces();

                l = strlen(cmdq);
                /* lower case typed command */
                for(i = 0; i < (int)strlen(cmdo); i++) {
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

void doprompt(VOIDFIX) {
#ifdef CPM
        register int un;
        register int dl;
        un = bdos(CPMSUID, 0xff);
        dl = bdos(CPMIDRV, 0) + 'A';
        printf("[%i:%c]ftp>", un, dl);
#else
        printf("ftp>");
#endif
        fflush(stdout);
        doconin(0);
        doparse();
}

void usage(char *argv[]) {
        AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

int ftpmain(int argc, char *argv[]) {
        register struct u_nameip result;
        struct sockaddr_in to;
        register int xkb;
        unsigned short port;
        int i;
        netaddress = NULL;
        cmdq = NULL;
        xferdata = NULL;
        cmdo = NULL;
        iodata = NULL;
        smackit = NULL;
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
        printf("ARGC = %i\n", argc);
        if(argc < 2 || argc > 3)  {
                usage(argv);
                goto bail;
        }
        netaddress = (char *)safemalloc(80);
        if(argc > 2) {
                sprintf(netaddress, "%s\n", argv[2]);
                port = atoi(netaddress) & 0xffff;
                if(port < 1) usage(argv);
                goto bail;
        }

        /* snag out the net address.... */
        i = resolve(argv[1], &result);
        if(i) {
                printf("No such host\n");
                goto bail;
        } else {

                memcpy(destination, result.hostip, 4);
                memcpy(&to.sin_addr, destination, 4);
                FREE(result.hostip);
                FREE(result.hostname);
                to.sin_port = port;
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if(sock > -1) {
                        printf("Trying to connect...");
                        zap = connect(sock, (struct sockaddr *)&to, sizeof (to));
                } else goto bail;
        }
        if(zap > -1) {
                printf("\nConnected.\n");
        } else {
                printf("\nService Not available\n");
                socket_close(sock);
                sock = -1;
                goto bail;
        }

        cmdq = (char *)safemalloc(160);
        xferdata = (char *)safemalloc(XSN);
        cmdo = (char *)safemalloc(160);
        iodata = (char *)safemalloc(BIGGEST);
        smackit = (char *)safemalloc(BIGGEST);

        if((!cmdq) || (!xferdata) || (!cmdo) || (!iodata) || (!smackit)) {
                printf("Not enough memory\n");
                for(;;);
        }
        xkb = 0;
        while(sock > -1) {
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
bail:
        if(netaddress)free(netaddress);
        if(cmdq)free(cmdq);
        if(xferdata)free(xferdata);
        if(cmdo)free(cmdo);
        if(iodata)free(iodata);
        if(smackit)free(smackit);

        return (0);
}

void freeargv(char **vector) {
        register char **scan;

        if(vector != NULL) {
                for(scan = vector; *scan != NULL; scan++) {
                        free(*scan);
                }
                free(vector);
        }
}

char **buildargv(char *input, int *rargc) {
        char *arg;
        char *copybuf;
        int squote = 0;
        int dquote = 0;
        int bsquote = 0;
        int maxargc = 0;
        char **argv = NULL;
        char **nargv;
        int argc = 0;

        if(input != NULL) {
                copybuf = (char *)alloca(strlen(input) + 1);
                do {
                        /* Pick off argv[argc] */
                        while(isspace(*input)) {
                                input++;
                        }
                        if((maxargc == 0) || (argc >= (maxargc - 1))) {
                                /* argv needs initialization, or expansion */
                                if(argv == NULL) {
                                        maxargc = INITIAL_MAXARGC;
                                        nargv = (char **)malloc(maxargc * sizeof (char *));
                                } else {
                                        maxargc *= 2;
                                        nargv = (char **)realloc(argv, maxargc * sizeof (char *));
                                }
                                if(nargv == NULL) {
                                        if(argv != NULL) {
                                                freeargv(argv);
                                                argv = NULL;
                                        }
                                        break;
                                }
                                argv = nargv;
                                argv[argc] = NULL;
                        }
                        /* Begin scanning arg */
                        arg = copybuf;
                        while(*input != '\0') {
                                if(isspace(*input) && !squote && !dquote && !bsquote) {
                                        break;
                                } else {
                                        if(bsquote) {
                                                bsquote = 0;
                                                *arg++ = *input;
                                        } else if(*input == '\\') {
                                                bsquote = 1;
                                        } else if(squote) {
                                                if(*input == '\'') {
                                                        squote = 0;
                                                } else {
                                                        *arg++ = *input;
                                                }
                                        } else if(dquote) {
                                                if(*input == '"') {
                                                        dquote = 0;
                                                } else {
                                                        *arg++ = *input;
                                                }
                                        } else {
                                                if(*input == '\'') {
                                                        squote = 1;
                                                } else if(*input == '"') {
                                                        dquote = 1;
                                                } else {
                                                        *arg++ = *input;
                                                }
                                        }
                                        input++;
                                }
                        }
                        *arg = '\0';
                        argv[argc] = strdup(copybuf);
                        if(argv[argc] == NULL) {
                                freeargv(argv);
                                argv = NULL;
                                break;
                        }
                        argc++;
                        argv[argc] = NULL;

                        while(isspace(*input)) {
                                input++;
                        }
                } while(*input != '\0');
        }
        *rargc = argc;
        return (argv);
}

void cli_sim(void) {
        int mainargc;
        char **mainargv;
        int stat;
        int kbptr;
        /* set up wanted irc data globals :-) */
        while(!network_booted) xmem::Yield(); // wait for networking to be ready.
        for(;;) {
                printf("\r\nEnter command line now.\r\n");
                memset(keyboard, 0, 1023);
                kbptr = 0;
                for(;;) {
                        while(fixkbhit()) { /* loop to fast empty keyboard buffer on slow machines */
                                stat = fixgetch(); /* get key */
                                if((stat == 0x08) || (stat == 0x7f)) {
                                        if(kbptr > 0) {
                                                kbptr--;
                                                keyboard[kbptr] = 0x00;
                                        }
                                } else {
                                        keyboard[kbptr] = stat & 0xff;
                                        if(kbptr < 1022) kbptr++;
                                        keyboard[kbptr] = 0x00;
                                        /* output RAW to net */
                                        if((stat == 0x0d) || (stat == 0x0a)) {
                                                kbptr--;
                                                keyboard[kbptr] = 0x00;
                                                kbptr++;
                                                killline();
                                                goto process;
                                        }
                                }
                                restoreline(kbptr);
                        }
                }
process:
                mainargv = buildargv((char *)keyboard, &mainargc);
                ftpmain(mainargc, mainargv);
                freeargv(mainargv);
        }
}

void start_system(void) {
        uint8_t fd = _VOLUMES;
        uint8_t slots = 0;
        uint8_t *ptr;
        uint8_t spin = 0;
        uint8_t last = _VOLUMES;
        char fancy[4];

        fancy[0] = '-';
        fancy[1] = '\\';
        fancy[2] = '|';
        fancy[3] = '/';
        printf_P(PSTR("\nStart SLIP on your server now!\007"));
        printf_P(PSTR("\nWaiting for '/' to mount..."));
        while(fd == _VOLUMES) {
                slots = fs_mountcount();
                if(slots != last) {
                        last = slots;
                        if(slots != 0) {
                                printf_P(PSTR(" \n"), ptr);
                                fd = fs_ready("/");
                                for(uint8_t x = 0; x < _VOLUMES; x++) {
                                        ptr = (uint8_t *)fs_mount_lbl(x);
                                        if(ptr != NULL) {
                                                printf_P(PSTR("'%s' is mounted\n"), ptr);
                                                free(ptr);
                                        }
                                }
                                if(fd == _VOLUMES) printf_P(PSTR("\nWaiting for '/' to mount..."));
                        }
                }
                printf_P(PSTR(" \b%c\b"), fancy[spin]);
                spin = (1 + spin) % 4;
                xmem::Sleep(100);
        }
        printf_P(PSTR(" \b"));
        // File System is now ready. Initialize Networking
        // Start the ip task using a 3KB stack, 29KB malloc arena
        uint8_t t1 = xmem::SetupTask(IP_task, 1024 * 3);
        xmem::StartTask(t1);
}

void setup(void) {
        USB_Module_Calls Calls[2];
        {
                uint8_t c = 0;
                Calls[c] = GenericFileSystem::USB_Module_Call;
                c++;
                Calls[c] = NULL;
        }
        USB_Setup(Calls);
        uint8_t t1 = xmem::SetupTask(start_system);
        xmem::StartTask(t1);
        uint8_t t2 = xmem::SetupTask(cli_sim);
        xmem::StartTask(t2);
}

void loop(void) {
        xmem::Yield(); // Don't hog, we are not used anyway.
}
