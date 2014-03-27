/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* tcp/udp socket code */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "in.h"
#include "netinet.h"
#include "socket.h"
#include "ioconfig.h"
#include "tcp.h"
#include "udp.h"
#include "timers.h"
#include "slip.h"
#include "tcpx.h"
#include "udpx.h"
#include "compat.h"
#include "errno.h"

#define DEBUGD 0
#define SOCKM 0
#define DEBUGU 0

#define BLOCKS 0

static int sched = 0;

/* conditional debugging, set from the header file */
#if DUMPSOCKSTATES

unsigned char svidx[] = {
        STATE_CLOSED, /* 0	*/
        STATE_BIND, /* 1	*/
        STATE_BOUND, /* 2	*/
        STATE_LISTEN, /* 3	*/
        STATE_NOTINIT, /* 4	*/
        STATE_SYN_SENT, /* 5	*/
        STATE_SYN_RECEIVED, /* 6	*/
        STATE_UDP, /* 7	*/
        STATE_ESTABLISHED, /* 8	*/
        STATE_CLOSEWAIT, /* 9	compare the compiler lines*/
        STATE_CLOSING, /* 10	*/
        STATE_FINWAIT1, /* 11	*/
        STATE_FINWAIT2, /* 12	*/
        STATE_LAST_ACK, /* 13	*/
        STATE_PEER_RESET, /* 14	*/
        STATE_TIME_WAIT /* 15	*/
};

char *sv[] = {
        "CLOSED",
        "BIND",
        "BOUND",
        "LISTEN",
        "NOTINIT",
        "SYN_SENT",
        "SYN_RECEIVED",
        "UDP",
        "ESTABLISHED",
        "CLOSEWAIT",
        "CLOSING",
        "FINWAIT1",
        "FINWAIT2",
        "LAST_ACK",
        "PEER_RESET",
        "TIME_WAIT",
        "??"
};

char * indst(v)
unsigned char v;
{
        int i;
        int j;

        j = 1;
        i = 0;
        while(j) {
                if(v == svidx[i]) {
                        j = 0;
                } else {
                        i++;
                        if(!i) {
                                i = 16;
                                j = 0;
                        }
                }
        }
        return(sv[i]);
}

void sockstats(VOIDFIX) {
        register unsigned char *sktp;

        int s;

        printf("%c[2J%c[f", 0x1b, 0x1b);
        for(s = 11; s > 0; s--) {compare the compiler lines

                sktp = SOKAT(s);
                printf("Socket %i, state %s\n", s, indst(sktp[SOCKST(0, 0)]));
        }
        printf("\n");
}

void dumpsock(s)
int s;
{
        register unsigned char *sktp;
        register unsigned char *foo;
        int i;
        int j;
        int count;

        schedulestuff();
        if((s < 1) || (s > BUFFERS)) {
                printf("Out of range %i\n", s);
                return;
        }
        sktp = SOKAT(s);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_BOUND) {
                printf("Not a multiple listen parent %i, state %i\n", s, sktp[SOCKST(0, 0)]);
                return;
        }
        sktp = rdatapointers[s];
        count = sktp[0] & 0xff;
        printf("\nParent socket %i Listening sockets: %i\n", s, count);
        i = 0;

        while(i < count) {
                i++;
                j = sktp[i] & 0xff;
                foo = SOKAT(j);

                if(foo[SOCKST(0, 0)] == (unsigned char) STATE_LISTEN) {
                        printf("[%i:L] ", j);
                }
        }
        printf("\nOther states :\n");

        for(i = SOKALLOW; i > 0; i--) {
                foo = SOKAT(i);
                if((foo[SPARET(0,compare the compiler lines 0)] & 0xff) == (s & 0xff)) {
                        if(foo[SOCKST(0, 0)] != (unsigned char) STATE_LISTEN) {
                                printf("[%i:%2.2x:%i.%i.%i.%i] ", i, foo[SOCKST(0, 0)] & 0xff, foo[0], foo[1], foo[2], foo[3]);
                        }
                }
        }
        putchar('\n');

}


#endif

void sendsocket(s)
int s;
{
        register unsigned char *sktp;
        unsigned char *stuff;
        int mss;
        int tmss;
        unsigned long oseq;

#if DEBUGD
        printf("sendsocket %i, %i data bytes, depth = %i ", s, sockdlen[s], CHECKUP);
#endif
        sktp = SOKAT(s);
        /*
                bad, assumes tcp, but we don't care...
                In order to window data, etc, more logic needs to go here
                Needs to track the SEND_NO_CARE state as well?
         */
        if(sktp[STTYPE(0, 0)] == SOCK_STREAM) {
                timertx[s] = CHK_TIME1 + ((loopcali * RETRYTIME) * (errortx[s] + 1));
                tmss = 0;
                stuff = wdatapointers[s];
                oseq = ret32word(&sktp[SEQNUM(0, 0)]); /* note the base sequence number */
                if((sktp[SOCKST(0, 0)] > (unsigned char) STATE_UDP) && (sktp[SOCKST(0, 0)] < (unsigned char) STATE_FINWAIT1)) {
                        /* Unified window area: check depths, send if we can, could use QoS */
#if WINDOWED
                        /* WINDOWED == amount of bytes allowed "in-flight" */
                        mss = wbufcount[s] - sockdlen[s];
                        if((sockdlen[s] < WINDOWED) && (mss)) {
#else
                        if((sockexpecting[s]) && (wbufcount[s])) {
#endif /* WINDOWED */
                                mss = ((sktp[SOKMSS(0, 0)] * 256) + sktp[SOKMSS(0, 1)]);
                                tmss = wbufcount[s] - sockdlen[s];
                                if(tmss > mss) tmss = mss;
                                /* adjust advanced sequence */
                                sav32word(&sktp[SEQNUM(0, 0)], (unsigned long) (oseq + (unsigned long) (sockdlen[s]))); /* artificially advance sequence number */
                                stuff = wdatapointers[s];
                                stuff += sockdlen[s];
                                sockdlen[s] += tmss;
                                sockexpecting[s] = 1;
                        }
                }
                fasttcppack(s, (sktp[BUFSTA(0, 0)] & 0xff), (sktp[SOCKST(0, 0)] & 0xff), tmss, stuff, 0);
                sav32word(&sktp[SEQNUM(0, 0)], oseq); /* restore base sequence number */
        }

#if DEBUGD
        printf("\n");
#endif
}

/*
 Scheduled sockets.
 */
void check_sockets(VOIDFIX) {
        register unsigned char *sktp;
        register unsigned char state;

        int work;
        int send;
        unsigned long now;

        sched--;
        if(sched < 0) {
                sched = BUFFERS - 1;
                inc32word((unsigned char *) &isnclock);
        }
        now = CHK_TIME1;
        /* scheduled combined writes */

        sktp = SOKAT(sched);
        state = sktp[SOCKST(0, 0)];

        if((state == (unsigned char) STATE_CLOSED) && (sktp[SPARET(0, 0)])) {
                /* Collect closed sockets that have parents. */
                nuke_tcp_socket(sched);
#if DEBUGD
                printf("Socket %i collected\n", s);
#endif
        }
        work = 0;
more:
        send = 0;
        if(state > (unsigned char) STATE_UDP) {
#if WINDOWED
                if(((wbufcount[sched] - sockdlen[sched])) && (sockdlen[sched] < WINDOWED)) {
#else
                if((wbufcount[sched]) && (!sockexpecting[sched])) {
#endif /* WINDOWED */
                        send = 1;
                        sktp[BUFSTA(0, 0)] = PSH | ACK;
                }
                /* check the timer... even if the above is true */
                if((now > timertx[sched]) && (sockexpecting[sched])) {
                        errortx[sched]++;
                        if(errortx[sched] > 10) {
                                /* Socket timed out! Ignorantly close it. */
                                nuke_tcp_socket(sched);
                                sockexpecting[sched] = sockdlen[sched] = send = 0;
                        } else {
                                send = 1;
                                sockdlen[sched] = 0; /* start at the beginning again */
                                if((state > (unsigned char) STATE_SYN_RECEIVED && state < (unsigned char) STATE_FINWAIT1)) {
                                        sktp[BUFSTA(0, 0)] = PSH | ACK;
                                }
                        }
                }

                if(send) {
                        sendsocket(sched);
                        work++;
                        if(work < 2) goto more;
                }
        }
}

void schedulestuff(VOIDFIX) {
        check_sockets();
        TRANSPORT
}

int writetcpbytes(fd, bytes, len)
int fd;
unsigned char *bytes;
unsigned int len;
{
        register unsigned char *sktp;
        register unsigned char *to;
        int count;
        int status;

        status = 0;

#if BLOCKS
        schedulestuff();
#endif
        sktp = SOKAT(fd);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_ESTABLISHED) {
                status = -1;
        } else {
                status = len;
                if(status > (MAXTAILW - wbufcount[fd])) {
                        status = MAXTAILW - wbufcount[fd];
                }
                count = wbufcount[fd];
                wbufcount[fd] += status;
                to = wdatapointers[fd] + count;
                /* copy bytes */
                memmove(to, bytes, status);
        }

#ifdef SOCK_IM
        /* immediate sockets */
        if((!sockdlen[fd]) && (wbufcount[fd]) && (!sockexpecting[fd])) {
                /* fire out this packet _NOW_ */
                sktp[BUFSTA(0, 0)] = PSH | ACK;
                sendsocket(fd);
        }
#endif

        return(status);
}

/* open, returns handle */
int sockconnect(fd, remoteport, destip)
int fd;
unsigned int remoteport;
unsigned char *destip;
{
        /*
        fd points to the socket slot here.
        I'm not going to bother storing the fd twice,
        so it will return the fd right back upon success.
        This simplifies a lot of things.
        A higher layer (if implemented, MUX for example)
        should assign it a different one anyhow.
         */
        register unsigned char *sktp;

        sktp = SOKAT(fd);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_NOTINIT) {
                fd = -1;
        } else {
                rdatapointers[fd] = MALLOC(MAXTAILR);
                if(!rdatapointers[fd]) {
#if SOCKM
                        printf("socket.c: %i No RAM\n", __LINE__);
#endif
                        return(-1);
                }
                wdatapointers[fd] = MALLOC(MAXTAILW);
                if(!wdatapointers[fd]) {
                        FREE(rdatapointers[fd]);
#if SOCKM
                        printf("socket.c: %i No RAM\n", __LINE__);
#endif
                        return(-1);
                }
                switch(sktp[STTYPE(0, 0)]) {
                        case SOCK_STREAM: /* tcp */
                                fd = tcp_open(fd, remoteport, destip);
                                /* now we wait until such a time that
                                we either time out, or get a connection.
                                It really needs something accurate for a timer.
                                60Hz would be wonderful.
                                 */
                                if(fd > 0) {
                                        do {
                                                schedulestuff();
                                        } while((sktp[SOCKST(0, 0)] != (unsigned char) STATE_CLOSED) && (sktp[SOCKST(0, 0)] != (unsigned char) STATE_ESTABLISHED));
                                        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_ESTABLISHED) {
                                                fd = -1;
                                        }
                                } else fd = -1;
                                break;

                        default:
                                schedulestuff();
                                fd = -1;
                                break;
                }
        }
        return(fd);
}

/* write */
int AJK_sockwrite(fd, sokbuf, len)
int fd;
void *sokbuf;
unsigned int len;
{

        register unsigned char *sktp;
        register unsigned char *xbuf;
        int j;
        int status;

        status = -1;


        if((fd < 1) || (fd > BUFFERS)) {
                status = -2;
        } else {
                sktp = SOKAT(fd);
                switch(sktp[STTYPE(0, 0)]) {
                        case SOCK_STREAM: /* tcp */
                                if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED) {
                                        j = status = 0;
                                        xbuf = (unsigned char *) sokbuf;
#if BLOCKS
                                        while(len > 0) {
#endif
                                                j = writetcpbytes(fd, xbuf, len);
                                                status += j;
                                                xbuf += j;
                                                len -= j;
#if BLOCKS
                                                if(j < 0) {
                                                        len = 0;
                                                }
                                        }
#endif
                                }
                                break;

                        default:
                                status = -3;
                                break;
                }
        }
        schedulestuff();
        return(status);
}

int incchk(i)
int i;
{
        i++;
        if(i == MAXTAILR) i = 0;
        return(i);
}

int wmov(here, to, count, i)
unsigned char *here;
unsigned char *to;
int count;
int i;
{

        while(count) {
                *to = here[i];
                i = incchk(i);
                to++;
                count--;
        }
        return i;
}

/* read */
int AJK_sockread(fd, sokbuf, len)
int fd;
void *sokbuf;
unsigned int len;
{
        /* sneak in some tcp/ip handling here */
        register unsigned char *sktp;
#if 0
        register unsigned char *bleh;
#endif
        register unsigned char *here;
        int i;
        int old;
        int status;

        status = -1;

        if(len != 1) schedulestuff();

        if((fd < 1) || (fd > BUFFERS)) {
                status = -2;
        } else {
                sktp = SOKAT(fd);
                old = rbufcount[fd];
                switch(sktp[STTYPE(0, 0)]) {
                        case SOCK_STREAM: /* tcp */
                                if(old > 0) { /* drain socket */
                                        /* if there's data here, read it */
                                        /* for now also our sockets are non-blocking */
                                        here = rdatapointers[fd];
                                        i = rbuftail[fd];
                                        status = rbufcount[fd];
                                        if(status > (int) len) status = len;
                                        rbuftail[fd] = wmov(here, sokbuf, status, i);
                                        rbufcount[fd] = rbufcount[fd] - status;
                                } else {
                                        if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED) {
                                                status = 0; /* keep socket alive! *till really closed* */
                                        }
                                }
                                if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED) {
                                        if(!sockdlen[fd]) {
                                                /* extra data push if need be */
                                                if(wbufcount[fd]) {
                                                        sendsocket(fd);
                                                } else {
                                                        /*
                                                                Advertise our new depth, this should push streaming better
                                                                Times to advertise: buffer now empty, buffer crossed 1/2 full...
                                                                but ONLY if we "did something"
                                                         */
                                                        /* if((rbufcount[fd] < (MAXTAILR / 2) && (status > 0)) { */
                                                        if(status > 2 || ((rbufcount[fd] < 10) && (status > 0))) {
                                                                if((!rbufcount[fd]) || ((rbufcount[fd] < (MAXTAILR / 2)) && (old > (MAXTAILR / 2)))) {
                                                                        sktp[BUFSTA(0, 0)] = ACK;
                                                                        errortx[fd] = 0;
                                                                        timertx[fd] = CHK_TIME1 + (loopcali * RETRYTIME);
                                                                        sendsocket(fd);
                                                                }
                                                        }
                                                }
                                        }
                                }
                                break;

                        default:
                                status = -3;
                                break;
                }
        }
        if(len == 1 && status != 1) schedulestuff();
        return(status);
}

/* for fflush() */
void flushsocket(fd)
int fd;
{
        register unsigned char *sktp;

        if((fd < 1) || (fd > BUFFERS)) return;

        sktp = SOKAT(fd);
        while(((sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED) || (sktp[SOCKST(0, 0)] == (unsigned char) STATE_CLOSING)) && ((wbufcount[fd]) || (sockdlen[fd]))) {
#if DEBUGD
                printf("sockdlen = %i, wbufcount = %i, state = %2.2x\n", sockdlen[fd], wbufcount[fd], sktp[SOCKST(0, 0)]);
#endif
                schedulestuff();
                if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_PEER_RESET) break;
        }
}

/* closes socket. */
int sockAclose(fd)
int fd;
{
        register unsigned char *sktp;

        if((fd < 1) || (fd > BUFFERS)) return 0;

        rbufcount[fd] = 0;
        sktp = SOKAT(fd);
        if(sktp[SOCKST(0, 0)] < (unsigned char) STATE_ESTABLISHED) goto nukeit;

        if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED || sktp[SOCKST(0, 0)] == (unsigned char) STATE_CLOSING) {
                /* drain write buffer, if we can */
                flushsocket(fd);
        }

        schedulestuff();
        if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_PEER_RESET) goto nukeit;
        sktp[SOCKST(0, 0)] = (unsigned char) STATE_LAST_ACK; /* shortcircuit */
        sockexpecting[fd] = 1; /* so we can time out keep */
        sktp[BUFSTA(0, 0)] = ACK | FIN;
        errortx[fd] = 0;
        sendsocket(fd);

        schedulestuff();
        /* this doesn't seem to hurt... */
nukeit:
        nuke_tcp_socket(fd);
        return(0);
}

/* closes the socket, but checks for binded sockets and kills those first */
int AJK_sockclose(fd)
int fd;
{
        register unsigned char *sktp;
        register unsigned char *foo;
        int i;
        int socks;
        int kill;

        if((fd < 1) || (fd > BUFFERS)) return 0;
        sktp = SOKAT(fd);
        /*

        If this is a bind() port, we need to kill allocated sockets, then fall thru.
        All sockets are killed without prejudice.

         */
        if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_BOUND) {
                foo = rdatapointers[fd];
                socks = foo[0] & 0xff;
                i = 0;
                while(i < socks) {
                        /* remove parent flag and kill */
                        i++;
                        kill = foo[i] & 0xff;
                        sktp = SOKAT(kill);
                        sktp[SPARET(0, 0)] = 0x00;
                        sockAclose(kill); /* don't care on the return value */
                        sktp[SPARET(0, 0)] = 0x00;
                }
        }
        /* Fall thru and close anything, including our bind socket. */
        kill = sockAclose(fd);
        return(kill);
}

/* find a free socket area */
int findfreeslot(type, protocol)
int type;
int protocol;
{
        register unsigned char* foo;
        int fd;
        int i;

        fd = -1;

        for(i = SOKALLOW - 1; i > 0; i--) {
                if((sockets[SOCKST(i, 0)] == (unsigned char) STATE_CLOSED) && (!rbufcount[i])) {
                        fd = i;
                }
        }
        if(fd > 0) {
                foo = SOKAT(fd);
                /* secure this socket NOW! */
                memset((void *) foo, 0, SOKDAT);
                foo[SOCKST(0, 0)] = STATE_NOTINIT; /* allocated, but NOT initialized! */
                foo[STTYPE(0, 0)] = type & 0xff;
                foo[STPROT(0, 0)] = protocol & 0xff;
        }
        return(fd);
}

/* setup socket */
int AJK_socket(domain, type, protocol)
int domain;
int type;
int protocol;
{
        register int fd;

        fd = -1;

        if(domain == AF_INET) {
                if(type == SOCK_STREAM) {
                        if(protocol == IPPROTO_IP) {
                                fd = findfreeslot(type, protocol); /* TCP/IP */
                        } /* protocol == 0 */
                } /* type == SOCK_STREAM */
#if UDP_ENABLED

                if(type == SOCK_DGRAM) {
                        if(protocol == IPPROTO_UDP) {
                                fd = findfreeslot(type, protocol); /* UDP/IP */
                        }
                } /* type == SOCK_DGRAM */
#endif /* UDP_ENABLED */
        } /* domain == AF_INET */
        return(fd);
}

int AJK_connect(sockfd, serv_addr, addrlen)
int sockfd;
struct sockaddr_in *serv_addr;
int addrlen; /* thrown away */
{
        int retval;

        retval = sockconnect(sockfd, (unsigned short) serv_addr->sin_port, (unsigned char *) &(serv_addr->sin_addr));
        if(retval > 0) retval = 0;
        return(retval);
}

/*
        bind()
        creates a blank fd that we can stat via listen()
        The second parameter isn't even used because we do not route.
 */
int AJK_bind(fd, my_addr, addrlen)
int fd;
struct sockaddr_in *my_addr;
int addrlen; /* thrown away */
{

        register unsigned char *sktp;

        if((fd > 0) || (fd < BUFFERS + 1)) {
                sktp = SOKAT(fd);
                if(sktp[SOCKST(0, 0)] == (unsigned char) STATE_NOTINIT) {
                        /* save the port # */
                        sktp[SRCPRT(0, 1)] = (char) ((unsigned short) my_addr->sin_port & 0xff);
                        sktp[SRCPRT(0, 0)] = (char) (((unsigned short) my_addr->sin_port / 256) & 0xff);
                        /*
                        This is a ficticious socket state,
                        allowing less ram via reuse.
                        This allows some fields to be (ab)used for tracking.
                        Don't forget to close the bind sockfd
                        if your done with it to free resources!
                         */
#if UDP_ENABLED
                        /* set the fake state for UDP, because it requires nothing more. */
                        if(sktp[STTYPE(0, 0)] == SOCK_DGRAM) {
                                if(sktp[STPROT(0, 0)] == IPPROTO_UDP) {
                                        rdatapointers[fd] = MALLOC(MAXTAILR);
                                        if(!rdatapointers[fd]) {
                                                return(-1);
                                        }
                                        sktp[SOCKST(0, 0)] = STATE_UDP;
                                }
                        } else
#endif /* UDP_ENABLED */
                                sktp[SOCKST(0, 0)] = STATE_BIND;

                        return(0);
                }
        }
        return(-1);
}

/*
        listen()
        Backlog is going to be a problem on a small system.
        Each open socket consumes ~3.5k... do the math...
        5 max, * 3.5 = 17k
        So be careful. Running out of RAM isn't fun at all.
        3 is this stack's default, and is
        changeable in the config up to max buffers - 1.
        This is because bind needs an fd too.
        Returns amount able to allocate. (nonstandard?)
 */
int AJK_listen(s, backlog)
int s;
int backlog;
{
        register unsigned char* foo;
        register unsigned char *sktp;
        int i;
        int slots;
        int result;

        schedulestuff();
        if((s < 1) || (s > BUFFERS)) {
                return(-3);
        }
        if(backlog < 1) backlog = 1;
        if(backlog > MAXBACKLOG) backlog = MAXBACKLOG;
        rdatapointers[s] = MALLOC(backlog + 1);
        if(!rdatapointers[s]) {
                return(-2);
        }
        slots = 0;
        foo = SOKAT(s);
        foo[WINDOW(0, 0)] = ((MAXPKTSZ) / 256) & 0xff;
        foo[WINDOW(0, 1)] = (MAXPKTSZ) & 0xff;
        foo[SOKMSS(0, 0)] = /* (DEFAULTMSS / 256) & 0xff; */ foo[WINDOW(0, 0)];
        foo[SOKMSS(0, 1)] = /* (DEFAULTMSS) & 0xff; */ foo[WINDOW(0, 1)];
        for(i = 0; i < backlog; i++) {
                /* grab backlog count sockets, and make them listen, save each one in the pointer queue */
                /* if we can't grab all sockets, truncate. No slots is an error. */
                result = findfreeslot(foo[STTYPE(0, 0)], foo[STPROT(0, 0)]);
                if(result > 0) {
                        sktp = SOKAT(result);
                        /* Allocate resources, if we can. */
                        rdatapointers[result] = MALLOC(MAXTAILR);
                        if(rdatapointers[result]) {
                                wdatapointers[result] = MALLOC(MAXTAILW);
                                if(wdatapointers[result]) {
                                        /* copy the socket, fix the state */
                                        slots++;
                                        sktp[SPARET(0, 0)] = s & 0xff;
                                        sktp[SPARPO(0, 0)] = slots & 0xff;
                                        nuke_tcp_socket(result);
                                } else {
                                        /* Reclaim resources. */
                                        FREE(rdatapointers[result]);
                                        sktp[SOCKST(0, 0)] = STATE_CLOSED;
                                }
                        } else {
                                /* Reclaim resources. */
                                sktp[SOCKST(0, 0)] = STATE_CLOSED;
                        }
                }
        }
        sktp = rdatapointers[s];
        sktp[0] = slots & 0xff;
        if(!slots) {
                FREE(rdatapointers[s]);
                return(-1);
        } else {
                foo[SOCKST(0, 0)] = STATE_BOUND;
        }
        return(slots);
}

/*
        sockstatus()
        check if socket() is at STATE_ESTABLISHED
 */
int sockstatus(s)
int s;
{
        register unsigned char *sktp;

        schedulestuff();
        if((s > 0) || (s < BUFFERS + 1)) {
                sktp = SOKAT(s);
                return(sktp[SOCKST(0, 0)] & 0xff);
        }
        return(-1);
}

/*
        bindcount()
        Returns the count of listening connections on a bound socket.
 */

int AJK_bindcount(s)
int s;
{
        register unsigned char *sktp;
        register unsigned char *foo;
        int i;
        int j;
        int count;
        int retval;

        schedulestuff();
        if((s < 1) || (s > BUFFERS)) {
                return(-1);
        }
        sktp = SOKAT(s);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_BOUND) {
                return(-2);
        }
        sktp = rdatapointers[s];
        count = sktp[0] & 0xff;
        i = retval = 0;
        while(i < count) {
                i++;
                j = sktp[i] & 0xff;
                foo = SOKAT(j);
                if((j) && (foo[SOCKST(0, 0)] == (unsigned char) STATE_LISTEN)) retval++;
        }
        return(retval);
}

/*
        acceptready()
        Returns if ANY socket in the backlog can be accepted.
 */

int AJK_acceptready(s)
int s;
{
        register unsigned char *sktp;
        register unsigned char *foo;
        int i;
        int j;
        int count;

        schedulestuff();
        if((s < 1) || (s > BUFFERS)) {
                return(-1);
        }

        sktp = SOKAT(s);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_BOUND) {
                return(-2);
        }

        sktp = rdatapointers[s];
        count = sktp[0] & 0xff;
        i = 0;
        while(i < count) {
                i++;
                j = sktp[i] & 0xff;
                foo = SOKAT(j);
                if((j) && (foo[SOCKST(0, 0)] == (unsigned char) STATE_ESTABLISHED)) return(1);
        }
        return(0); /* none ready */
}

/*
        accept()
        Unfortunately, this one has to block (however, see the above function).
        Returns the fd to the socket that connected or error.
 */

int AJK_accept(s, addr, addrlen)
int s;
struct sockaddr_in *addr;
int *addrlen; /* thrown away */
{
        register unsigned char *sktp;
        int this;
        int sock;
        int status;
        int count;
        schedulestuff();
        if((s < 1) || (s > BUFFERS)) {
                return(-2);
        }

        sktp = SOKAT(s);
        if(sktp[SOCKST(0, 0)] != (unsigned char) STATE_BOUND) {
                return(-3);
        }

        if(AJK_bindcount(s) < 0) {
                return(-1); /* error */
        }
        /* dig out the fds from the listening sockets */
        sktp = rdatapointers[s];
        count = sktp[0] & 0xff;
        this = 0;
        while(this < count) {
                this++;
                status = sockstatus(sktp[this]);
                if((status == (unsigned char) STATE_ESTABLISHED) && (sktp[this] > 0)) {
                        sock = sktp[this];
                        sktp[this] = 0x00; /* now dead, out of the ring */
                        /* fill in the address/port info for return */
                        sktp = SOKAT(sock);
                        addr->sin_family = sktp[STTYPE(0, 0)];
                        addr->sin_port = sktp[DESPRT(0, 0)] + (sktp[DESPRT(0, 1)] * 256);
                        snag32((unsigned char *) &sktp[IPDEST(0, 0)], (unsigned char *) &addr->sin_addr);
                        return(sock); /* return the listening socket # */

                }
        }
        return(-4);
}

#if UDP_ENABLED

/*

UDP send is unique in the fact that it is delivered instantly.
A check on the TX queue is needed to verify there is room so that
it does not block. UDP is not buffered at all in this implementation.
On some platforms that have no scheduling, there is no TX queue.
This is because slip verifies the buffer as empty.

 */
int AJK_sendto(s, msg, len, flags, to, tolen)
int s;
const void *msg;
size_t len;
unsigned int flags; /* thrown away */
const struct sockaddr *to;
socklen_t tolen; /* thrown away */
{
        register unsigned char *sktp;
        unsigned char *buffer;
        unsigned char *ptr;
        int i;
        int ht;
        int freebuf;
        struct sockaddr_in *zap;

        schedulestuff();

        if((s > 0) && (s <= BUFFERS)) {
                sktp = SOKAT(s);
                /* assumes UDP */
                if(sktp[STTYPE(0, 0)] == SOCK_DGRAM) {
                        /* we do not fragment, so if it can't fit in the message size, error! */
                        if((len + IPHEADLEN + UDPHEADLEN) <= (size_t) (CHECKUP)) {
                                zap = (struct sockaddr_in *) to;
                                freebuf = -1;
                                /* we can xmit w/out blocking... build a UDP packet and ship it */
                                for(i = 1; i < BUFFERS; i++) {
                                        if(!(txrxflags[i] & 0xff)) freebuf = i;
                                }
                                if(freebuf < 0) {
                                        goto wouldblock; /* out of build buffers */
                                }
                                /* malloc the buffer */
                                bufptrs[freebuf] = MALLOC(len + IPHEADLEN + UDPHEADLEN);
                                if(!bufptrs[freebuf]) {
                                        /* we're screwed... */
                                        goto wouldblock; /* out of build buffers */
                                }
                                txrxflags[freebuf] = (ISTX | FILLBUSY); /* lock the buffer */
                                buffer = bufptrs[freebuf];
                                ptr = (unsigned char *) &(zap->sin_addr);
                                /* lay out ip */
                                createip(buffer, ptr, 0x11);
                                ptr = (unsigned char *) &(zap->sin_port);
                                ht = createudp(buffer, ptr, len, (unsigned char *) msg, sktp);
                                /* calculate sums */
                                buffer = bufptrs[freebuf];
                                udpsumming(buffer); /* needs to check the option for summing as well */

                                /* set the size of the entire package */
                                rxtxpointer[freebuf] = ht;
                                /* toss it in the tx queue */
                                txrxflags[freebuf] = (ISTX | READY | SENDNOCARE) & 0xff;
                                return(0);
                        } else {
                                goto wouldblock; /* no nic buffer free, no way to fragment */
                        }
                }
        }
        errno = ENOSYS;
        return(-1);
wouldblock:
        errno = EWOULDBLOCK;
        return(-1);
}

int AJK_recvfrom(s, buf, len, flags, from, fromlen)
int s;
void *buf;
size_t len;
unsigned int flags; /* thrown away */
struct sockaddr *from;
socklen_t fromlen; /* thrown away */
{
        register unsigned char *sktp;
        unsigned char *here;
        int i;
        size_t count;
        size_t pcount;
        struct sockaddr_in *zap;
        unsigned char *ptr;

        count = 0;
        schedulestuff();

        if((s > 0) && (s <= BUFFERS)) {

                sktp = SOKAT(s);
                /* assumes UDP */
                if(sktp[STTYPE(0, 0)] == SOCK_DGRAM) {
                        /* data? */
                        if(!rbufcount[s]) {
                                errno = EWOULDBLOCK;
                                return(-1);
                        }
                        zap = (struct sockaddr_in *) from;
                        /* slop the content where it needs to go */
                        here = rdatapointers[s];
                        i = rbuftail[s]; /* 0 */
                        ptr = (unsigned char *) zap;
                        i = wmov(here, ptr, 8, i);

                        /* how many bytes in the packet */
                        pcount = (int) ((unsigned char) here[i] * 256); /* l 1 */
                        i = incchk(i);
                        pcount = pcount + ((int) (unsigned char) here[i]); /* l 0 */
                        i = incchk(i);


                        count = (size_t) pcount;
                        if(len < count) count = len;
                        /* increment tail as we go */
                        ptr = (unsigned char *) buf;
#if DEBUGU
                        printf("copy %i bytes, wanted %i packet was %i\n", count, len, pcount);
#endif
                        i = wmov(here, ptr, count, i);

                        if(len < pcount) {
                                count = pcount - len;
                                while(count) {
                                        i = incchk(i);
                                        count--;
                                }
                        }
                        rbufcount[s] = rbufcount[s] - (pcount + 9);
                        rbuftail[s] = i;
                        if(len < pcount) pcount = len;
                        return(pcount); /* return count in packet */
                }
        }
        errno = ENOSYS;
        return(-1);
}


#else /* UDP_ENABLED */

int AJK_sendto(s, msg, len, flags, to, tolen)
int s;
const void *msg;
size_t len;
int flags;
const struct sockaddr *to;
socklen_t tolen;
{
        errno = ENOSYS;
        return(-1);

}

int AJK_recvfrom(s, buf, len, flags, from, fromlen)
int s;
void *buf;
size_t len;
int flags;
struct sockaddr from;
socklen_t *fromlen;
{
        errno = ENOSYS;
        return(-1);

}

#endif /* UDP_ENABLED */

/* something along the lines of select() to check read and write buffer depths */
int AJK_sockdepth(s, rw)
int s;
int rw;
{
        register unsigned char i;

        schedulestuff();


        if((s > 0) && (s <= BUFFERS)) {
                /*
                                printf("Socket %i, state %s\n", s, indst(sockets[SOCKST(s,0)]));
                 */
                i = sockets[SOCKST(s, 0)];
                if(i > (unsigned char) STATE_NOTINIT && i < (unsigned char) STATE_FINWAIT1) {
                        /* read is 0, write is 1 */
                        if(!rw) {
                                return(rbufcount[s]);
                        } else if(rw == 1) {
                                return(wbufcount[s]);

                        }
                }
        }
        return(-1); /* error */
}


/* tell link parser who we are and where we belong

0100 libznet.lib socket.obj xxLIBRARYxx

 */

