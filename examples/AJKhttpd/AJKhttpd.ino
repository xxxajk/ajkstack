/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include <Arduino.h>
// These are needed for auto-config and Arduino
#include <Wire.h>
#include <SPI.h>

#if defined(CORE_TEENSY) && defined(__arm__)
#include <spi4teensy3.h>
#else
#ifndef USE_MULTIPLE_APP_API
#define USE_MULTIPLE_APP_API 16
#endif
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


#define PROGRAM "CPMHTTPD"
#define VERSION "0.0.2"
#define EXECUTABLE "HTTPD"


#define VERBOSE 0
#define DEBUGIT	0

#define M501 "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<HTML><HEAD>\n<TITLE>501 Method Not Implemented</TITLE>\n</HEAD><BODY>\n<H1>Method Not Implemented</H1>\n%s not supported.<P>\n</BODY></HTML>\n"
#define M400 "Bad Request\r\n"
#define M404 "Not Found\r\n"
#define M414 "Request-URI Too Long\r\n"
#define M200 "HTTP/1.0 200 OK\r\n"
#define MESBADA "text/html; charset=iso-8859-1\r\n\r\n<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<HTML><HEAD><TITLE>"
#define MESBADB "</TITLE></HEAD><BODY>\n<H1>"
#define MESBADC "</H1>\"%s\"<P>\n%s<P>\n</BODY></HTML>\n"
#define MESBAD414 MESBADC, "Request-URI Too Long", "The URI information is to long."
#define MESBAD404 MESBADC, "Not Found", "File Does not exist on this server."
#define MESBAD400 MESBADC, "Bad Request", "Your browser sent a request that this server could not understand."


/* unix */
#define LIMTEDNAMES 0
#define SLASHTO "/cpmweb/"
#define INDEX_HTML "/index.html"
#define MAXSTREAMS 2
#define MAXMESSAGE 1024
#define MAXDATA DEFAULTMSS * 3
#define MAXNAME 256 /* is this right for unix? */

//#define MINDATA 48

char indexfile[] = INDEX_HTML;
int indexfilelen = sizeof (INDEX_HTML);
char slashto[] = SLASHTO;
int slashlen = sizeof (SLASHTO);
/*           01234567 */
char star[] = " .:. -+-";

struct sockaddr_in srv_addy;
int stream[MAXSTREAMS];
int fds[MAXSTREAMS];
char *info[MAXSTREAMS];
int ptr[MAXSTREAMS];
char *output;
char *infoptr;
char *scratch;
int active[MAXSTREAMS];
int sock;
int dotcount;
int avail;
int total;
int now;

int senddata(int fd, char *info, int len) {
        int i;
        int j;

        j = 0;

        while(j < len) {
                i = socket_write(fd, info, len - j);
                if(i < 0) return (-1);
                j += i;
                info += i;
        }
        /* force a scheduled socket event here */
        //socket_read(-1, NULL, 0);
        return (j);
}

int sendheader(int fd, char *info, int err) {
        int i;
        time_t now;

        /* since the other compiler barfs on switch... */
        if(err == 414) {
                sprintf(info, M414);
        }
        if(err == 404) {
                sprintf(info, M404);
        }
        if(err == 400) {
                sprintf(info, M400);
        }
        if(err == 200) {
                sprintf(info, M200);
        }

        i = senddata(fd, info, strlen(info));
        if(i < 0) return (i);
        now = time(NULL);
        sprintf(info, "Date: %sServer: %s/%s (%s, %s, stack revision %s)\r\nContent-Type: ", ctime(&now), PROGRAM, VERSION, OS, CPU, &STACKVERSION[0]);
        i = senddata(fd, info, strlen(info));
        return (i);
}

int sendbad(int fd, char *info, int err) {
        int i;

        i = sendheader(fd, info, err);

        if(i < 0) return (i);
        sprintf(info, MESBADA);
        i = senddata(fd, info, strlen(info));
        if(i < 0) return (i);
        sprintf(info, MESBADB);
        i = senddata(fd, info, strlen(info));
        if(i < 0) return (i);
        /* since the other compiler barfs on switch... */
        if(err == 414) {
                sprintf(info, MESBAD414);
        }
        if(err == 404) {
                sprintf(info, MESBAD404);
        }
        if(err == 400) {
                sprintf(info, MESBAD400);
        }

        i = senddata(fd, info, strlen(info));
        return (i);
}

void death(int i) {
        int j;

        /* take case of flushed socket close nicely */
        j = socket_select(stream[i], 1);

        if(j < 1) {
                /* still eats some time, but it is not as bad as it was before... */
#if VERBOSE
                printf("Closing socket %i(%i)...", i, stream[i]);
                fflush(stdout);
#endif
                socket_close(stream[i]);
#if VERBOSE
                printf("\nSocket %i(%i) closed\n", i, stream[i]);
#endif
                ptr[i] = 0;
                stream[i] = -1;
        }
}

int testend(int i) {
        int j;

        j = ptr[i] - 4;

        if(infoptr[j] != '\r') return (1);
        j++;
        if(infoptr[j] != '\n') return (1);
        j++;
        if(infoptr[j] != '\r') return (1);
        j++;
        if(infoptr[j] != '\n') return (1);
        return (0);
}

void findinfo(VOIDFIX) {
        now = 0;
        while(infoptr[now] != ' ' && infoptr[now] != '\n') {
                infoptr++;
        }
        while(infoptr[now] == ' ' && infoptr[now] != '\n') {
                infoptr++;
        }
        while(infoptr[now] != ' ' && infoptr[now] != '\n') {
                now++;
        }
#if VERBOSE
        printf("GOT :%s:\n", infoptr);
        printf("GOT :%s:\n", &infoptr[now]);
        /* now = "strlen" */
        printf("STRLEN %i\n", now);
#endif
}

int normalize(VOIDFIX) {
        int j;
        int k;

        /* this should be the file name */
        /* "GET " + " HTTP/1.0" */
        for(j = k = 0; j < now; j++, k++) {
                if(k > (MAXDATA - slashlen)) {
                        k = 0;
                        break;
                }
                output[k] = infoptr[j];
                if(infoptr[j] == '%') {
                        /* hex digits! uppercase them */
                        output[k + 1] = toupper(infoptr[j + 1]);
                        output[k + 2] = toupper(infoptr[j + 2]);
                        /* convert to value, if sane, and collapse */
                        if((output[k + 1] >= '0' && output[k + 1] <= '9') || (infoptr[j + 1] >= 'A' && infoptr[j + 1] <= 'F')) {
                                if((output[k + 2] >= '0' && output[k + 2] <= '9') || (infoptr[j + 2] >= 'A' && infoptr[j + 2] <= 'F')) {
                                        if(output[k + 1] >= 'A') {
                                                output[k + 1] -= 'A';
                                        } else {
                                                output[k + 1] -= '0';
                                        }
                                        output[k] = output[k + 1] << 4;
                                        if(output[k + 2] >= 'A') {
                                                output[k + 1] -= 'A';
                                        } else {
                                                output[k + 1] -= '0';
                                        }
                                        output[k] |= output[k + 1];
                                        j += 2;
                                }
                        } else {
                                /* else the url is probabbly bad anyway. Therefore, it will fail */
                                /* remove suspicious character */
                                output[k] = '_';
                        }
                }
        }
        output[k] = 0x00;
#if VERBOSE
        printf("CONVERTED :%s:\n", output);
#endif
        return (k);
}

int process_HEAD(int i) {
        int j;
        int k;

        /* find out file information, spew header, or error 404 */
        findinfo();

        scratch = infoptr;
        k = normalize();

        /* File name area converted to text, remove known bad things from it. */
        if(k < 1 || output[0] != '/') {
                /* first slash is required */
                return (400); /* 400 error */
        }

        /* change first slash */
        for(j = k; j > 0; j--) {
                output[j + slashlen - 2] = output[j];
        }

        for(j = 0; j < slashlen - 1; j++) {
                output[j] = slashto[j];
        }
        k = strlen(output);

        /*
           file name is in output now for len of k.
           URL is in scratch for len of NOW
         */
#if VERBOSE
        printf("CONVERTED :%s:\n", output);
#endif
#ifdef CPM
        /* is the last element a ":"?, if so, append indexfile */
        if(output[k - 1] == ':') {
#else
        /* is the last element a "/"?, if so, append indexfile */
        if(output[k - 1] == '/') {
#endif
                k--;
                for(j = 0; j < indexfilelen; j++, k++) {
                        output[k] = indexfile[j];
                }
        }
        /* terminate the string. */
        output[k] = 0x00;
#if VERBOSE
        printf("CONVERTED :%s:\n", output);
#endif
        /* remove bad shit like "/../" for dos and unix */
        /* kill illegal characters */
        /* if cpm and any slashes, bad file name. */
        for(j = slashlen; j < (int)strlen(output); j++) {
                if(j < (int)(strlen(output) + 3)) {
                        if(output[j] == '/' && output[j + 1] == '.' && output[j + 2] == '.' && output[j + 3] == '/') {
                                printf("\n/../ hack attempt\n");
                                return (400); /* 400 error */
                        }
                }
#ifdef __MSDOS__
                if(output[j] == '/') output[j] = '\\';
#endif
#if LIMTEDNAMES
                /* dos and cp/m */
                /* convert slashes and uppercase for dos and cp/m */
                output[j] = toupper(output[j] & 0xff) & 0xff;
                if(output[j] < '0' || output[j] > 'Z') {
                        /* possible corrupt... */
#ifdef __MSDOS__
                        if(output[j] == '\\') goto dosok;
#endif
                        if(output[j] == '.') goto dosok;
                        printf("\nLIMITED NAMES caught ->%c<-\n", output[j]);
                        return (400); /* 400 error */
                }
#ifdef LIMTEDNAMES
dosok:
                return;
#endif
#endif
        }
        /* If we get here, we've passed all the needed sanity checks. Check actual file. */
#if VERBOSE
        printf("FINAL :%s:\n", output);
#endif
        if((fds[i] = open(output, O_RDONLY)) < 1) {
                fds[i] = -1;
                return (404); /* can't find it. */
        }

        infoptr = info[i];
        infoptr += 5;
        sprintf(infoptr, " %s", output);
        infoptr++;
        sendheader(stream[i], &output[0], 200); /* found it */

        /* default to binary */
        sprintf(output, "application/octet-stream\r\n");
        if(strlen(infoptr) > 3) {
                infoptr += strlen(infoptr) - 4;
                if(*infoptr != '.') {
                        infoptr--;
                }
        } else goto binary;

        if((!strncasecmp(".txt", infoptr, 4))) {
                sprintf(output, "text/plain\r\n");
                goto text;
        }

        if((!strncasecmp(".htm", infoptr, 4)) || (!strncasecmp(".html", infoptr, 5))) {
                sprintf(output, "text/html\r\n");
                goto text;
        }

        if((!strncasecmp(".jpg", infoptr, 4)) || (!strncasecmp(".jpeg", infoptr, 5))) {
                sprintf(output, "image/jpeg\r\n");
        }

        if((!strncasecmp(".png", infoptr, 4))) {
                sprintf(output, "image/png\r\n");
        }


        if((!strncasecmp(".gif", infoptr, 4))) {
                sprintf(output, "image/gif\r\n");
        }

        goto binary;
text:
        // Not used, text is binary...
        //infoptr = info[i];
        //infoptr += 6;
        //freopen(infoptr, "r", fds[i]);
        binary :
                senddata(stream[i], &output[0], strlen(output)); /* found it */
        /* file dates? */
        /* How big is this file? */
        lseek(fds[i], 0L, SEEK_END);
        sprintf(output, "Content-Length: %lu\r\n\r\n", tell(fds[i]));
        senddata(stream[i], &output[0], strlen(output));
        infoptr = info[i];
        /* inactivity timer */
        active[i] = 0;
        if(strncmp("GET ", infoptr, 4)) {
                /* was just a HEAD request */
                close(fds[i]);
                fds[i] = -1;
#if VERBOSE
                printf("Socket %i(%i) closing (HEAD)\n", i, stream[i]);
#endif
                ptr[i] = -1;
        } else lseek(fds[i], 0L, SEEK_SET);
        return (0);
}

int more(int i) {
        int j;
        int k;

        infoptr = info[i];
        infoptr += ptr[i];
        j = socket_read(stream[i], infoptr, (MAXMESSAGE - 1) - ptr[i]);

        /* inactivity timer */
        if(!j) {
                active[i]++;
        } else {
                active[i] = 0;
        }
        ptr[i] += j;
        infoptr += j;
        if(j < 0) {
#if VERBOSE
                printf("Socket %i(%i) closing (read)\n", i, stream[i]);
#endif
                ptr[i] = -1;
                goto blink;
        }
        *infoptr = 0x00;
#if VERBOSE
        printf("data so far from %i(%i)\n:\n%s\n:\n", i, stream[i], info[i]);
#endif
        if(fds[i] < 1) {
                /*
                    File is not open.
                 */

                if(ptr[i] > 3) {
                        /* first check for \r\n\r\n at the end of the buffer. */
                        infoptr = info[i];
                        j = testend(i);
                        if(j) return (j);

                        /* have end of info */
                        j = 0;
                        while(j < ptr[i]) {
                                if(infoptr[j] == '\n' || infoptr[j] == '\r') break;
                                j++;
                        }
                        if(infoptr[j] == '\n' || infoptr[j] == '\r') {
                                infoptr[j] = 0x00;
#if VERBOSE
                                printf("GOT :%s:\n", info[i]);
#endif
                                /* have first termination line, is this HTTP/1.0? */
                                ptr[i] = j + 1;
                                /*  check method. */
                                if((!strncmp("HEAD ", infoptr, 5)) || (!strncmp("GET ", infoptr, 4))) {
                                        k = process_HEAD(i);
                                        if(k == 400) goto four00;
                                        if(k == 404) goto four04;
                                } else {
                                        /* bad method message */
                                        infoptr = info[i];
                                        while(*infoptr != ' ' && *infoptr != '\n') {
                                                infoptr++;
                                        }
                                        *infoptr = 0x00;
                                        sprintf(output, M501, info[i]);
                                }
                        } else if(ptr[i] == MAXMESSAGE) {
                                /* 414 error */
                                /* sloppy but I'm placing *ALL* the errors here except one above. */
                                /* four14: */
                                k = 414;
                                goto shoot;
four04:
                                k = 404;
                                goto shoot;
four00:
                                k = 400;
shoot:
                                sprintf(output, "HTTP/1.0 %i ", k);
                                j = senddata(stream[i], &output[0], strlen(output));
                                if(j < 0) goto closewrite;
                                j = sendbad(stream[i], &output[0], k);
closewrite:
#if VERBOSE
                                printf("Socket %i(%i) closing (write)\n", i, stream[i]);
#endif
                                ptr[i] = -1;
                        }
                }
        }
        return (0);
blink:
        return (2);
}

void killit(int i) {
        close(fds[i]);
        ptr[i] = 0;
        fds[i] = -1;
#if VERBOSE
        printf("End of data, Socket %i(%i) closing\n", i, stream[i]);
#endif
        ptr[i] = -1;
}

void tests(int div) {
        int i;
        int j;
        int k;

        /* new function */
        /* test each stream for data and process data/spew file */
        for(i = 0; i < total; i++) {

                /* force a scheduled socket event here */
                //socket_write(-1, NULL, 0);
                /* check for inactivity */
                if(active[i] == 5000) {
                        if(fds[i] > 0) killit(i);
                        ptr[i] = -1;
                }

                if(stream[i] > 0) {
                        if(ptr[i] == -1) {
                                death(i);
                                goto blink;
                        }
                        if((ptr[i] < MAXMESSAGE) && (fds[i] < 1)) {
                                j = more(i);
                                if(j == 1) goto zap;
                                if(j == 2) goto blink;
                        } else {
                                if(fds[i] < 1) goto blink;
#if VERBOSE
                                printf("Read data\n");
#endif
                                /* File is open. */
                                now = 0;
                                j = read(fds[i], &output[0], div);
                                if(j < 1) {
                                        j = 0;
                                        killit(i);
                                }
#if VERBOSE
                                printf("Send data %i \n", j);
#endif
                                while(now < j) {
                                        /* inactivity timer */
                                        active[i] = 0;
                                        k = senddata(stream[i], &output[now], j);
                                        if(k < 0) {
#if VERBOSE
                                                printf("Socket %i(%i) closing (write)\n", i, stream[i]);
#endif
                                                ptr[i] = -1;
                                                now = j + 1;
                                                close(fds[i]);
                                                fds[i] = -1;
                                        } else {
                                                now = now + k;
                                                j = j - k;
                                        }
                                }
#if VERBOSE
                                printf("Sent data %i \n", now);
#endif
                        }
                }
        }
        goto blink;
zap:
        /* here we "settle" the ptr to the end of the first line */
        j = 0;
        while(j < ptr[i]) {
                if(infoptr[j] == '\n' || infoptr[j] == '\r') break;
                j++;
        }
        if(infoptr[j] == '\n' || infoptr[j] == '\r') {
                j += 4; /* move ahead past garbage */
                if(ptr[i] < j + 5) goto blink; /* can't shrink it yet */
                k = j;
                j = ptr[i] - 5;

                while(j < ptr[i]) {
                        infoptr[k] = infoptr[j + 1];
                        j++;
                        k++;
                }
                ptr[i] = k; /* new "swallowed" pointer */
        }
blink:
        return;
}

void server(VOIDFIX) {
        char *addr;
        int i, j, twirl, dotcount, avail;

        twirl = dotcount = avail = 0;

        for(;;) {
                /* SOCKSTATS */
                if(fixkbhit()) {
                        now = fixgetch();
                        if(now == 3) {
                                printf("\n");
                                return;
                                //exit(0);
                        }
#if DEBUGIT
                        //if(now == 1) {
                                /* Print socket states */
                        //        dumpsock(sock);
                        //}
#endif
                }
                dotcount++;
                /* "It's alive" 1 sec timer tick, it's not accurate on any platform :-) */
                if(dotcount == TIMETICK) {
                        dotcount = 0;
                        twirl = (twirl + 1) & 0x07;
                        printf("%c\b", star[twirl]);
                        fflush(stdout);
                }
                now = bindcount(sock);
                if(now < 0) {
                        printf("bindcount() Error %i, Socket was %i\n", now, sock);
                        return;
                        //exit(now);
                }
#if VERBOSE
                if(now != avail) printf("%i/%i sockets available on socket %i.\n", now, total, sock);
#else
                if(now != avail) printf("\r%i/%i sockets available on socket %i --> <--   \b\b\b\b\b\b\b", now, total, sock);
#endif
                avail = now;
                now = accept_ready(sock);
                if(now < 0) {
                        printf("Acceptready() error %i\n", now);
                        return;
                        //exit(now);
                }
                if(now == 1) {
#if VERBOSE
                        printf("Accepting incoming connection.\n");
#endif
                        i = 0;
                        while(i < MAXSTREAMS) {
                                if(stream[i] == -1) break;
                                i++;
                        }
                        ptr[i] = 0;
                        stream[i] = accept(sock, (struct sockaddr*)&srv_addy, (socklen_t*)(&j));
                        if(stream[i] < 1) {
#if VERBOSE
                                printf("Accept nonfatal error %i\n", stream[i]);
#endif
                                stream[i] = -1;
                        } else {
                                addr = (char *)&srv_addy.sin_addr;
#if VERBOSE
                                printf("\nStream %i(%i). Connection from %i.%i.%i.%i\n", i, stream[i], addr[0] & 0xff, addr[1] & 0xff, addr[2] & 0xff, addr[3] & 0xff);
#endif
                        }
                }
                now = accept_ready(sock);
                if(!now) {
                        //now = (MAXDATA / ((MAXSTREAMS - avail) + 1));
                        //if(now < MINDATA) now = MINDATA;
                        tests(MAXDATA);
                }
        }
}

void httpd(void) {

        int i;
        output = (char *)safemalloc(MAXDATA);
        while(!network_booted)
#ifdef XMEM_MULTIPLE_APP
                xmem::Yield()
#endif
                ; // wait for networking to be ready.

        srv_addy.sin_port = 80; /* echo port */
        sock = socket(AF_INET, SOCK_STREAM, 0);
        dotcount = bind(sock, (struct sockaddr*)&srv_addy, sizeof (srv_addy));
        if(dotcount < 0) {
                printf("\nError on bind %i\n", dotcount);
                return;
        }

        dotcount = listen(sock, MAXSTREAMS);
        if(dotcount < 1) {
                printf("\nError on listen %i\n", dotcount);
                return;
        }

        printf("socket # %i. %i sockets available, Waiting for test connection.\n", sock, dotcount);
        avail = dotcount;
        total = avail;
        for(i = 0; i < total; i++) {
                fds[i] = -1;
                ptr[i] = 0;
                info[i] = (char *)safemalloc(MAXMESSAGE + 1);
                if(!info[i]) {
                        socket_close(sock);
                        printf("No RAM!\n");
                        return;
                        //exit(-1);
                }
        }

        for(i = 0; i < MAXSTREAMS; i++) {
                stream[i] = -1;
                active[i] = 0;
        }

        server();
        free(output);
        printf("Exit requested, quitting httpd task.\n");
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
#ifndef XMEM_MULTIPLE_APP
        USB_main();
#endif
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
#ifdef XMEM_MULTIPLE_APP
                xmem::Sleep(100);
#else
                delay(100);
#endif
        }
        printf_P(PSTR(" \b"));
        // File System is now ready. Initialize Networking
#ifdef XMEM_MULTIPLE_APP
        // Start the ip task using a 3KB stack, 29KB malloc arena
        uint8_t t1 = xmem::SetupTask(IP_task, 1024*3);
        xmem::StartTask(t1);
#else
        IP_main();
        httpd(); // never returns
#endif
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

#ifdef XMEM_MULTIPLE_APP
        uint8_t t1 = xmem::SetupTask(start_system);
        xmem::StartTask(t1);
        uint8_t t2 = xmem::SetupTask(httpd);
        xmem::StartTask(t2);
#endif
}

void loop(void) {
#ifdef XMEM_MULTIPLE_APP
        xmem::Yield(); // Don't hog, we are not used anyway.
#else
        start_system(); // Never returns
#endif
}
