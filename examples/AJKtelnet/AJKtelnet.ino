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

int sock;
/* see if directive was seen, and what stage */
int ffseen = 0;
/* holding data for a telnet response */
unsigned char ffresponse[5];

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

/* process the inbound queue data from a socket read */
void processbuf(unsigned char *buf, int len) {
        unsigned char *ptr;
        int spot;

        spot = 0;
        ptr = buf;

        while(spot != len) {
                if(ffseen == 1) {
                        if((*ptr > (unsigned char)0xfa) && (*ptr != (unsigned char)0xff)) {
                                ffresponse[1] = *ptr;
                                ffseen++;
                        } else {
                                putchar(*ptr);
                                ffseen = 0;
                        }
                } else if(ffseen == 2) {
                        if(ffresponse[1] == (unsigned char)0xfd) {
                                ffresponse[1] = 0xfc;
                                ffresponse[2] = *ptr;
                                socket_write(sock, ffresponse, 3);
                        }
                        ffseen = 0;
                } else {
                        if(*ptr == (unsigned char)0xff) {
                                ffseen++;
                                ffresponse[0] = (unsigned char)0xff;
                        } else {
                                putchar(*ptr);
                        }
                }
                ptr++;
                spot++;
        }
        fflush(stdout);
}

void usage(char *argv[]) {
        AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

int telnetmain(int argc, char *argv[]) {
        struct u_nameip result;
        struct sockaddr_in to;
        int zap;
        int dotcount;
        int smack;
        int port;
        int again;
        int xkb;
        char netaddress[80];
        unsigned char destination[4];
        unsigned char iodata[81];
        unsigned char xiodata[5];
        int i;


        if((argc < 2) || (argc > 3)) {
                usage(argv);
                goto bail;
        }
        if(argc > 2) {
                sprintf(netaddress, "%s\n", argv[2]);
                port = atoi(netaddress) & 0xffff;
                if(port < 1) {
                        usage(argv);
                        goto bail;
                }
        }

        zap = -1;
        dotcount = smack = 0;
        port = 23;
        /* snag out the net address.... */
        i = resolve(argv[1], &result);
        if(i) {
                printf("No such host\n");
                goto bail;
        } else {

                /* take each one into a byte :-) */
                printf("\n\nTrying ");
                for(dotcount = 0; dotcount < 4; dotcount++) {
                        printf("%i.", (short)(result.hostip[dotcount] & 0xff));
                        destination[dotcount] = result.hostip[dotcount];
                }
                printf("\b (%s) port %i...", result.hostname, port);
                FREE(result.hostip);
                FREE(result.hostname);
                to.sin_port = port;
                memcpy(&to.sin_addr, destination, 4);
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if(sock>-1) {
                        zap = connect(sock, (struct sockaddr *)&to, sizeof (to));
                } else goto bail;
        }

        if(zap > -1) {
                printf("\nConnected\n");
                iodata[0] = 0xff;
                iodata[1] = 0xfd; /* do */
                iodata[2] = 0x03; /* some thing */
                socket_write(sock, iodata, 3); /* Supress go ahead */
        } else {
                printf("\nService Not available\n");
                socket_close(sock);
                sock = -1;
                goto bail;
        }

        xkb = dotcount = 0;
        for(; sock > -1;) {
                if(sock > -1) {
                        /* input from net */
                        zap = socket_read(sock, iodata, 80);
                        if(zap < 0) {
                                socket_close(sock);
                                sock = -1;
                                goto bail;
                        }
                        processbuf(iodata, zap);
                }

                if(sock > -1) {
                        if(!smack) {
                                smack = 1;
                                iodata[0] = iodata[3] = 0xff;
                                iodata[1] = 0xfd; /* 253 do */
                                iodata[4] = 0xfb; /* 251 will */
                                iodata[2] = iodata[5] = 0x00;
                                socket_write(sock, iodata, 6);
                        }
                        /* lame attempt at input from keyboard */
                        if(fixkbhit()) {
                                again = fixgetch();
                                xiodata[0] = again & 0xff;
                                zap = socket_write(sock, &xiodata[0], 1);
                                if(zap < 0) {
                                        socket_close(sock);
                                        sock = -1;
                                        goto bail;
                                }
                                if(xiodata[0] == 0xff) zap = socket_write(sock, &xiodata[0], 1);
                                if(zap < 0) {
                                        socket_close(sock);
                                        sock = -1;
                                        goto bail;
                                }
                        }
                }
        }
bail:
        return 0;

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
        while(!network_booted)
#ifdef XMEM_MULTIPLE_APP
                xmem::Yield()
#endif
                ; // wait for networking to be ready.
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
                telnetmain(mainargc, mainargv);
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
        cli_sim(); // never returns
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
        uint8_t t2 = xmem::SetupTask(cli_sim);
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
