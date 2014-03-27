/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
        ELKS (other UN*X too?) serial tty driver

 ***Driver	03	elksser
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

#if USEELKSTTYS /* Include this code? */
#include "elksser.h"
#include "tcpvars.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

char HARDDRV[] = "Linux ELKS Serial Driver 0.0.1";
int serfd = 0;

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

        c = 0;
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
                FIXME!

                I'm not sure if elks uses the same IOCTL that linuxser.c uses,
                so a 1 will atleast allow the code to work... --AJK
         */

        return(checkfd(serfd));
}

int check_out(VOIDFIX) {
        /* fsync(serfd); */
        return(0);
}

/* return free space... this is needed for UDP. */
int check_up(VOIDFIX) {
        /* This driver needs to know how to do this yet... */
        return(4096);
}

void outtodev(c)
unsigned int c;
{
        unsigned char a;

        a = c & 0xff;
        write(serfd, &c, 1);
}

unsigned char infromdev(VOIDFIX) {
        unsigned char c;

        read(serfd, &c, 1);
        return(c);
}

void initializedev(VOIDFIX) {
        struct termios tio;

        memset(&tio, 0, sizeof(struct termios));
        tio.c_cc[VMIN] = 1;
        tio.c_iflag = IGNPAR;
        tio.c_cflag = MY_BAUD_RATE | CS8 | CRTSCTS | CLOCAL | CREAD;
        serfd = open(SERIALDEVICE, O_NOCTTY | O_RDWR);
        if(!serfd) {
                perror("open");
                exit(1);
        }
        tcsetattr(serfd, TCSANOW, &tio);
}
#else

void elksser(VOIDFIX) {
        /* keep compiler happy... */
}

#endif /* USEELKSTTYS */


/* tell link parser who we are and where we belong

1103 libznet.lib elksser.obj xxLIBRARYxx

 */

