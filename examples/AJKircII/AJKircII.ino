/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
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

#define PROGRAM "CPMIRC"
#define VERSION "0.1.3"
#define EXECUTABLE "IRC"
#define USER "oldfart"
#define USE "the.network.IP.address nick [port [PASSWORD]]"
#define INITIAL_MAXARGC 8
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/* some ugly globals to speed up things */
unsigned char iodata[1025];
unsigned char keyboard[1025];
char nick[40];
char channel[128];
char pass[40];
char *server;
char *ircbuf;
char *replied;

int auth = 0;
int inchannel = 0;

void killline(void) {
        memset(pass, 0, 40);
        memset(pass, ' ', 38);
        printf("\r%s\r", pass);
}

void restoreline(int kbptr) {
        int add;

        add = 0;
        killline();
        if(kbptr > 38) add = kbptr - 38;
        memset(pass, 0, 40);
        strncat(pass, (char *)&keyboard[add], 38);
        printf("\r%s", pass);
}

int shipit(int sock, char *bleh) {
        int stat;
        int kbptr;
        char *keys;

        keys = bleh;
        kbptr = strlen(keys);
        stat = 0;
        while((kbptr > 0) && (sock > 0)) {
                stat = socket_write(sock, keys, strlen(keys));
                if(stat < 0) {
                        sock = -1;
                        /* auto bail on error */
                } else {
                        keys += stat;
                        kbptr -= stat;
                }
        }
        return (stat);
}

void dumpmsg(char *message, char *who, char *extra) {
        int cc;
        int i;

        cc = 0;
        for(i = 0; i < (int)strlen(who); i++) {
                if((who[i] == '!') && (!cc)) {
                        cc = i;
                }
        }
        if(cc > 0) {
                who[cc] = 0x00;
        }
        printf(message, who, extra);
        putchar('\n');
}

void domode(char *argv[], int argc) {
        int i;

        if((!strncmp(argv[0], nick, strlen(argv[0]))) && (!strncmp(argv[0], nick, strlen(nick)))) {
                argv[argc - 1]++;
                printf("*** Mode change on yourself '%s' by yourself\n", argv[argc - 1]);
        } else {
                printf("*** Mode change '");
                for(i = 3; i < argc; i++) {
                        printf("%s ", argv[i]);
                }
                printf("' on channel %s by ", channel);
                dumpmsg("%s%s", argv[0], ".");
        }
        return;
}

void donick(char *argv[]) {
        if(!strncmp(argv[0], nick, strlen(nick))) {
                printf("*** Your nick is now %s\n", argv[2]);
                sprintf(nick, "%s", argv[2]);
        } else {
                dumpmsg("*** %s is now known as %s", argv[0], argv[2]);
        }
        return;
}

void dopart(char *argv[]) {
        if(!strncmp(argv[0], nick, strlen(nick))) {
                inchannel = 0;
                printf("*** You left %s\n", argv[2]);
        } else {
                dumpmsg("*** %s has left channel %s", argv[0], channel);
        }
        return;
}

void dojoin(char *argv[]) {
        int i;

        if(!strncmp(argv[0], nick, strlen(nick))) {
                inchannel = 1;
                sprintf(channel, "%s", argv[2]);
                for(i = 0; i < (int)strlen(channel); i++) {
                        channel[i] = channel[i + 1];
                }
                printf("*** You have joined channel %s\n", channel);
        } else {
                dumpmsg("*** %s has joined channel %s", argv[0], channel);
        }
        return;
}

void doquit(char *argv[], int argc, char *replies) {
        argv[argc - 1]++;
        sprintf(replies, "*** Signoff: %%s (%s)", argv[argc - 1]);
        dumpmsg(replies, argv[0], argv[argc - 1]);
        return;
}

void doprivmsg(char *argv[], int argc, int sock) {
        int i;
        int cc;
        char *ptr;
        char *aptr;
        char *bptr;

        ptr = argv[0];
        cc = 0;
        for(i = 0; i < (int)strlen(ptr); i++) {
                if((ptr[i] == '!') && (!cc)) {
                        cc = i;
                }
        }
        argv[argc - 1]++;
        if(cc > 0) {
                ptr[cc] = 0x00;
                /* parse out actions, ACTION VERSION PING */
                aptr = argv[argc - 1];
                if(aptr[0] != 0x01) {
                        printf("<%s> %s\n", ptr, argv[argc - 1]);
                } else {
                        bptr = aptr;
                        aptr++;
                        /* find the other end of the ^A, zero it */
                        do {
                                bptr++;
                        } while((bptr[0] != 0x01) && (bptr[0]) && (bptr[0] != ' '));
                        bptr[0] = 0x00;
                        bptr++;
                        if(!strcmp(aptr, "ACTION")) {
                                /* strip off the ending ^A */
                                aptr = bptr;
                                while(((aptr[0] != 0x01) && (aptr[0]))) {
                                        aptr++;
                                }
                                aptr[0] = 0x00;
                                printf("* %s %s\n", ptr, bptr);
                                return;
                        }
                        if(!strcmp(aptr, "VERSION")) {
                                printf("*** %s requested your version\n", ptr);
                                sprintf(replied, "NOTICE %s :\001VERSION %s version %s, stack revision %s (%s, %s)\001\n", ptr, PROGRAM, VERSION, &STACKVERSION[0], OS, CPU);
                                shipit(sock, replied);
                                return;
                        }
                        if(!strcmp(aptr, "PING")) {
                                printf("*** PING request from %s.\n", ptr);
                                sprintf(replied, "NOTICE %s :\001PING %s\n", ptr, bptr);
                                shipit(sock, replied);
                                return;
                        }
                        printf("*** Unknown CTCP from %s, CTCP %s\n", ptr, aptr);
                }
        } else {
                printf("*** %s\n", argv[argc - 1]);
        }
        return;

}

void dodata(char *argv[], int argc, char *ptr, int sock, char *replies) {
        int i;
        int foo;

        foo = 0;

        if(!strncmp("MODE", argv[1], 4)) {
                domode(argv, argc);
                foo = 1;
        }


        if(!strncmp("NICK", argv[1], 4)) {
                argv[2]++;
                donick(argv);
                foo = 1;
        }

        if(!strncmp("PART", argv[1], 4)) {
                dopart(argv);
                foo = 1;
        }

        if(!strncmp("JOIN", argv[1], 4)) {
                dojoin(argv);
                foo = 1;
        }

        if(!strncmp("QUIT", argv[1], 4)) {
                doquit(argv, argc, replies);
                foo = 1;
        }

        if(!strncmp("PRIVMSG", argv[1], 7)) {
                doprivmsg(argv, argc, sock);
                foo = 1;
        }

        if(!strncmp("451", argv[1], 3)) {
                shipit(sock, ircbuf);
                foo = 1;
        }

        argv[0]--;
        /* basically, ignore everything for now (display it) */
        if(!foo) {
                for(i = 0; i < argc; i++) {
                        ptr = argv[i];
                        printf("%s ", ptr);
                }
                putchar('\n');
        }
}

void processircbuffer(int sock) {
        int i;
        int cc;
        int lod;
        int argc;
        char *ptr;
        char *argv[20];
        /* a:b:c */
        /* a = command, b=frominfo c=data but sometimes b is the data!
        I guess I'll do an args count like ircII does...

         */

        lod = strlen((char *)&iodata[0]);
        if(lod < 1) return;
        argv[0] = (char *)&iodata[0];
        ptr = argv[0];
        cc = argc = 0;
        while((ptr[0] == 0x0a) || (ptr[0] == 0x0d)) {
                argv[0]++;
                ptr++;
        }
        for(i = 0; i < lod; i++) {
                if((iodata[i] == ' ') && (cc < 2)) {
                        iodata[i] = 0x00;
                        argc++;
                        argv[argc] = (char *)&iodata[i + 1];
                }
                if(iodata[i] == ':') {
                        if(cc < 2) cc++;
                }
        }
        argc++;

        if(ptr[0] == ':') {
                /* data */
                argv[0]++;

                dodata(argv, argc, ptr, sock, replied);
                return;
        } else {
                /* server command */
                /* automated PING */
                if(!strncmp("PING", ptr, 4)) {
                        sprintf(replied, "\nPONG %s\n", argv[1]);
                        shipit(sock, replied);
                        return;
                }
                ptr = argv[1];

                /* basically, ignore everything else (just display it) */
                for(i = 0; i < argc; i++) {
                        ptr = argv[i];
                        printf("%s ", ptr);
                }
                putchar('\n');
        }
        return;
}

int processkb(int sock) {
        int stat;
        int kbptr;
        int i;
        char *keys;
        char chatdata[1025];

        stat = 0;

        kbptr = strlen((char *)&keyboard[0]);
        keys = (char *)&keyboard[0];
        if(kbptr < 1) return (0);
        if((kbptr < 2) && (keys[0] == '/')) return (0);
        if(keys[0] == '/') {
                keys++;
                for(i = 0; i < kbptr; i++) {
                        if(keys[i] == ' ') {
                                i = kbptr;
                        } else {
                                keys[i] = toupper(keys[i]);
                        }
                }

                /* vanilla _RAW_ mode */
                if(!strncmp("MODE", keys, 4)) {
                        keys += 4;
                        sprintf(chatdata, "\nMODE %s\n", keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if(!strncmp("CHANMODE", keys, 8)) {
                        keys += 8;
                        sprintf(chatdata, "\nMODE %s %s\n", channel, keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if(!strncmp("SELFMODE", keys, 8)) {
                        keys += 8;
                        sprintf(chatdata, "\nMODE %s %s\n", nick, keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if(!strncmp("USERMODE", keys, 8)) {
                        keys += 8;
                        sprintf(chatdata, "\nMODE %s %s\n", channel, keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }



                if(!strncmp("NICK", keys, 4)) {
                        keys += 4;
                        sprintf(chatdata, "\nNICK %s\n", keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if(!strncmp("ME ", keys, 3)) {
                        keys += 3;
                        if(inchannel) {
                                printf("ACTION %s\n", keyboard);
                                sprintf(chatdata, "\nPRIVMSG %s :\001ACTION %s\001\n", channel, keys);
                                stat = shipit(sock, chatdata);
                                return (stat);
                        }
                        printf("Not in a channel\n");
                        return (0);
                }

                if(!strncmp("QUOTE ", keys, 6)) {
                        keys += 6;
                        sprintf(chatdata, "\n%s\n", keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if(!strncmp("QUIT", keys, 4)) {
                        keys += 4;
                        if(!strncmp(" ", keys, 1)) {
                                keys++;
                                sprintf(chatdata, "\nQUIT :%s\n", keys);
                        } else {
                                sprintf(chatdata, "\nQUIT :.)(.)%s Version %s, Stack Version %s, Goodbye... (|\n", PROGRAM, VERSION, STACKVERSION);
                        }
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                if((!strncmp("PART", keys, 4)) || (!strncmp("LEAVE", keys, 5))) {
                        if(inchannel) {
                                sprintf(chatdata, "\nPART %s\n", channel);
                                stat = shipit(sock, chatdata);
                                return (stat);
                        }
                        printf("Not in a channel\n");
                        return (0);
                }

                if(!strncmp("JOIN ", keys, 5)) {
                        if(inchannel) {
                                sprintf(chatdata, "\nPART %s\n", channel);
                                stat = shipit(sock, chatdata);
                                if(stat < 0) return (stat);
                        }
                        sscanf(keys, "%s %s", channel, channel);
                        sprintf(chatdata, "\nJOIN %s\n", channel);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }

                return (0);
        } else {
                if(inchannel) {
                        printf(">>--> %s\n", keyboard);
                        sprintf(chatdata, "\nPRIVMSG %s :%s\n", channel, keys);
                        stat = shipit(sock, chatdata);
                        return (stat);
                }
                printf("Not in a channel\n");
                return (0);
        }
}

void usage(char *argv[]) {
        AJKNETusage(PROGRAM, VERSION, EXECUTABLE, argv[0], USE);
}

void ircmain(int mainargc, char *mainargv[]) {
        struct u_nameip result;
        struct sockaddr_in to;
        int sock;
        int stat;
        int dotcount;
        int kbptr;
        int port;
        int lpointer;
        int i;
        char netaddress[80];
        unsigned char destination[4];
        sock = -1;
        stat = -1;
        dotcount = 0;
        kbptr = 0;
        port = 6667;

        if(mainargc < 3 || mainargc > 5) {
                usage(mainargv);
                goto bail;
        }
        if(mainargc > 3) {
                sprintf(netaddress, "%s\n", mainargv[3]);
                port = atoi(netaddress) & 0xffff;
                if(port < 1) {
                        usage(mainargv);
                        goto bail;
                }
        }

        if(strlen(mainargv[2]) > 40) {
                usage(mainargv);
                goto bail;
        }
        /* snag out the net address.... */
        i = resolve(mainargv[1], &result);
        //result = findhost(mainargv[1], 1);
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
                //FREE(result);
                to.sin_port = port;
                memcpy(&to.sin_addr, destination, 4);
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if(sock>-1) {
                        stat = connect(sock, (struct sockaddr *)&to, sizeof (to));
                } else goto bail;

        }
        sprintf(server, "%s", mainargv[1]);
        lpointer = 0;
        sprintf(nick, "%s", mainargv[2]);
        for(i = 0; i < (int)strlen(nick); i++) {
                nick[i] = tolower(nick[i]);
        }
        sprintf(ircbuf, "\nNICK %s\nUSER %s example.com %s :%s\n", nick, USER, server, USER);

        if(mainargc > 4) {

                sprintf(pass, "%s", mainargv[4]);
                for(i = 0; i < (int)strlen(pass); i++) {
                        pass[i] = tolower(pass[i]);
                }
                sprintf(ircbuf, "\nNICK %s\nPASS %s\nUSER %s example.com %s :%s\n", nick, pass, USER, server, USER);
        }
        if(!stat) {
                printf("\nConnected\n");
                shipit(sock, ircbuf);
        } else {
                printf("\nService Not available\n");
                socket_close(sock);
                sock = -1;
                goto bail;
        }


        memset(keyboard, 0, 1023);

        for(; sock > -1;) {
                if(sock > -1) {
                        /* input from net till we get a CR */
                        stat = socket_read(sock, &iodata[lpointer], 1023 - lpointer);
                        if((stat < 0) && (!lpointer)) {
                                socket_close(sock);
                                sock = -1;
                                goto bail;
                        }
                        if(lpointer > 1022) {
                                lpointer = 0;
                                stat = 0;
                        }
                        if(stat > 0) lpointer += stat;
                        if(lpointer > 0) {
                                for(i = 0; i < lpointer; i++) {
                                        if((iodata[i] == 0x0d) || (iodata[i] == 0x0a)) {
                                                killline();
                                                iodata[i] = 0x00; /* terminate our string. */
                                                processircbuffer(sock);
                                                i++;
                                                lpointer = lpointer - i;
                                                memmove(iodata, iodata + i, lpointer);
                                                i = 0;
                                                restoreline(kbptr);
                                        }
                                }
                        }
                }
                if(fixkbhit()) {
                        while(fixkbhit()) { /* loop to fast empty keyboard buffer on slow machines */
                                stat = fixgetch(); /* get key */
                                if(sock > -1) {
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
                                                        stat = processkb(sock);
                                                        memset(keyboard, 0, 1023);
                                                        kbptr = 0;
                                                        if(stat < 0) {
                                                                socket_close(sock);
                                                                sock = -1;
                                                                goto bail;
                                                        }
                                                }
                                        }
                                }
                        }
                        restoreline(kbptr);
                }
        }
bail:
        return;
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
        ircbuf = (char *)safemalloc(1025);
        replied = (char *)safemalloc(512);
        server = (char *)safemalloc(1025);
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
                ircmain(mainargc, mainargv);
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
        uint8_t t1 = xmem::SetupTask(IP_task, 1024*3);
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

