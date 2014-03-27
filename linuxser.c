/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
        Linux (other UN*X too?) serial tty driver

 ***Driver	02	linuxser
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

#if USELINUXTTYS /* Include this code? */
#include "tcpvars.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "linuxser.h"
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <stropts.h>
#include "net.h"

#define TERMIO termios

char HARDDRV[] = "Linux Serial Driver 0.0.3";
int serfd = 0;

struct TERMIO tty_old, tty_new;

int checkfd(fd)
int fd;
{
        struct timeval tv;
        fd_set fds;

        tv.tv_sec = tv.tv_usec = 0;

        if(fd < 0)
                return(0);
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        select(fd + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(fd, &fds))
                return(1);
        return(0);
}

int fixgetch(VOIDFIX) {
        unsigned int c;

        read(0, &c, 1);
        return(c);
}

int fixkbhit(VOIDFIX) {
        return(checkfd(0));
}

void zapbuffers(VOIDFIX) {
        /* don't do anything */
}

int checkdata(VOIDFIX) {
        /*
                AJK --
                Technically for optimum performance we need to know
                the serial queue DEPTH. A 1 is better than a zero, however...
                return(checkfd(serfd));
                can be rewritten as...
         */

        int count;

        count = 0;

        ioctl(serfd, FIONREAD, &count);
        return(count);
        /* Now we can work faster...  -- AJK */
}

int check_out(VOIDFIX) {
        /* fsync(serfd); */
        int count;

        count = 0;

        ioctl(serfd, TIOCOUTQ, &count);
        return(count);
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* linux uses a 4k output buffer. */
        return(4096 - check_out());
}

void outtodev(c)
unsigned int c;
{
        unsigned char a;

        a = c & 0xff;
        write(serfd, &a, 1);
}

unsigned char infromdev(VOIDFIX) {
        unsigned char c;

        read(serfd, &c, 1);
        return(c);
}

void inittty(VOIDFIX) {
        tcgetattr(0, &tty_old);
        tcgetattr(0, &tty_new);
        tty_new.c_iflag |= IGNBRK;
        tty_new.c_lflag = 0;
        tty_new.c_cflag &= ~(PARENB | CSIZE); /* Disable parity */
        tty_new.c_cflag |= (CREAD | CS8); /* Set character size = 8 */
        tcsetattr(0, TCSANOW, &tty_new);
}

/* this chunk of code restores the tty via atexit() */
void ttyrestore(VOIDFIX) {
        tcsetattr(0, TCSANOW, &tty_old);
        endnetwork();
}

void initializedev(VOIDFIX) {
        struct termios tio;
        int baud;
        int modembits;

        modembits = MY_BAUD_RATE;

        switch(modembits) {
                case 1200: baud = B1200;
                        break;
                case 4800: baud = B4800;
                        break;
                case 9600: baud = B9600;
                        break;
                case 19200: baud = B19200;
                        break;
                case 38400: baud = B38400;
                        break;

        }

        memset(&tio, 0, sizeof(struct termios));
        tio.c_cc[VMIN] = 1;
        tio.c_iflag = IGNPAR;
        tio.c_cflag = HUPCL | IGNBRK | IGNPAR | CS8 | CRTSCTS | CLOCAL | CREAD | baud;
        serfd = open(SERIALDEVICE, O_NOCTTY | O_RDWR);
        if(!serfd) {
                perror("open");
                exit(1);
        }
        tcsetattr(serfd, TCSANOW, &tio);
        ioctl(serfd, TIOCMBIS, &modembits);
        inittty();
        atexit(ttyrestore);
}

#else

void linuxser(VOIDFIX) {
        /* keep compiler happy... */
}

#endif /* USELINUXSER */


/* tell link parser who we are and where we belong

1102 libznet.lib linuxser.obj xxLIBRARYxx

 */


