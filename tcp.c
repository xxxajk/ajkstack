/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*

TCP RFC 0793
Pre-optimized for the compiler via
keeping 0x00 stores together and other tricks to help the C optimizer
I know this makes it harder to follow, but it's worth it! HONEST!

Unfortunately writes are committed instantly due to small TPA constraints.
This may change in the future, providing I get to write a C compiler
that produces less inline asm. Calls may be a bit time expensive,
but ram is the major issue on smaller systems.

There is a special case where the stack gets out-of-sync with the incoming stream
due to low speeds. I'm not quite certain how to fix that yet.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "in.h"
#include "netinet.h"
#include "socket.h"
#include "ioconfig.h"
#include "chcksum.h"
#include "timers.h"
#include "tcpx.h"
#include "compat.h"
#include "icmp.h"

#if IP_ENABLED
#if TCP_ENABLED
#define DEBUGI 0
#define DEBUGD 0
#define DEBUGM 0
#define DEBUGQUE 0
#define OK2WAIT 0

void nuke_tcp_socket(fd)
int fd;
{
        register unsigned char *sktp;
        register unsigned char *foo;
        int par;
        int i;

        sktp = SOKAT(fd);
        par = sktp[SPARET(0, 0)] & 0xff;
        i = sktp[SPARPO(0, 0)] & 0xff;
        foo = SOKAT(par);

#if DEBUGD
        printf("Socket %i, previous %i, parent %i\n", fd, i, par);
#endif
        sktp[DESPRT(0, 0)] = sktp[DESPRT(0, 1)] = 0x00;
        /* clean out the port value */
        if((!sktp[SPARET(0, 0)]) || ((sktp[SPARET(0, 0)]) && (foo[SOCKST(0, 0)] == (unsigned char) STATE_CLOSED))) {
                sktp[SOCKST(0, 0)] = (unsigned char) STATE_CLOSED;
                FREE(rdatapointers[fd]);
                FREE(wdatapointers[fd]);
        } else {
                /* Reclaim this buffer (copy from parent) */
                memcpy(sktp, foo, SOKDAT);
                sktp[SPARET(0, 0)] = par & 0xff;
                sktp[SPARPO(0, 0)] = i & 0xff;
                sktp[SOCKST(0, 0)] = (unsigned char) STATE_LISTEN;
                sktp = rdatapointers[par]; /* free field position */
                sktp[i] = fd & 0xff; /* reclaimed */
        }
        wbufcount[fd] = rbufcount[fd] = rbuftail[fd] = rbufhead[fd] = sockexpecting[fd] = 0;
#if DEBUGD
        printf("Resource %i freed\n", fd);
#endif
}

void processtcpdata(bufnum, sock, iplen, tcplen, datalen)
unsigned int bufnum;
int sock;
int iplen;
int tcplen;
int datalen;
{
        register unsigned char *sktp;
        unsigned char sockstate;
        unsigned char command;
        unsigned char *buffer;
        int send;
        unsigned long oack;
        unsigned long iseq;
        long diff;
        unsigned char *datapointer;
        unsigned char *ours;
#if DEBUGD
        printf("\nprocesstcpdata\n");
#endif
        sktp = SOKAT(sock);
        buffer = bufptrs[bufnum];
        sockstate = sktp[SOCKST(0, 0)];
        command = buffer[iplen + 13];
        oack = ret32word(&sktp[ACKNUM(0, 0)]);
        iseq = ret32word(NUMSEQ(buffer, iplen));
        /* needs to handle 0xffffffff roll over !!!!!! */
        diff = (unsigned long) (oack - iseq);
        send = 0;
#if DEBUGD
        printf("DL %4.4x DIFF %8.8lx oack %8.8lx iseq %8.8lx\n", datalen, diff, oack, iseq);
#endif
        if((unsigned long) diff <= (unsigned long) (MAXTAILR - rbufcount[sock])) {
                /* we have enough buffer space to swallow this data */

                if(command & SYN) diff--;
                if(diff == 0L) { /* match only *EXACT* packets */

                        datapointer = buffer + iplen + tcplen - datalen;
                        if(datalen > 0) {
                                sav32word(&sktp[ACKNUM(0, 0)], (unsigned long) (oack + (unsigned long) (datalen)));
                                bufferdatamove(datapointer, sock, datalen);
                                send = 1;
                        }

                        if(sockstate == (unsigned char) STATE_FINWAIT1) {

                                if((unsigned char) (command & (FIN | ACK)) == (unsigned char) FIN) {
                                        /* we only got a fin packet, so we need to change state so we can resync */
                                        send = 1;
                                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_CLOSING;
                                        sktp[BUFSTA(0, 0)] = (unsigned char) (ACK | FIN);
                                        sockexpecting[sock] = 1; /* XXX */
                                        timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* XXX */
#if DEBUGD
                                        printf("STATE_CLOSING(fixup)\n");
#endif
                                } else {
                                        freebuffer(bufnum);
                                        sockexpecting[sock] = 1; /* expecting another packet, allow a timeout. keep */
                                        timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* keep */
                                        sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
                                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_FINWAIT2;
#if DEBUGD
                                        printf("STATE_FINWAIT2\n");
#endif
                                        return;
                                }
                        }
                        if(command & FIN) {

                                ours = &sktp[ACKNUM(0, 0)];
                                inc32word(ours);
#if DEBUGD
                                printf("consumed fin.\n");
#endif
                                if(sockstate == (unsigned char) STATE_FINWAIT2) {
#if DEBUGD
                                        printf("STATE_TIME_WAIT\n");
#endif
                                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_TIME_WAIT;
                                        sockexpecting[sock] = 1; /* this is just to time the slob out keep */
                                        sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
                                        send = 1;
                                }
                                /* BSD stacks ignore this..... */
                                if(sockstate == (unsigned char) STATE_ESTABLISHED) {
                                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_CLOSING;
                                        send = 1;
                                        sockexpecting[sock] = 1; /* XXX */
                                        timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* XXX */
                                }
                        }
                        /* free the inbound buffers */
                        freebuffer(bufnum);
                        /* bug, ack's an ack w/out data, needs to check datalen and state */
                        if(send) sendsocket(sock); /* ACK back */
                } else {
                        /*
                         If window didn't match...
                         Send our current ack regardless?
                         Hope we get a retransmit of what we want?
                         Free the inbound buffers regardless.
                         Timeout on remote seems to work fine. (sort of)
                         Need to track current ack is OK, and if not, send that reply.
                         Lost packets are a problem with some applications, if the wait is to long.
                         The optimal fix is a proper driver that doesn't lose packets.
                         Another way is to hold current packets, but we can't guarantee there is enough ram to do that.
                         */
                        freebuffer(bufnum);
                        sendsocket(sock); /* testing... 666 */
                }
        } else {
                /* else we save the packet for another time, should we send winsize=0? no read buffer left :-(
                however this doesn't mean we can't send our data out... we'll see what happens with it this way first :O) */
#if DEBUGQUE
                printf("out of buffer? Buffer free %8.8lx, inbound %8.8lx\n", (long) (MAXTAILR - rbufcount[sock]), diff);
#endif
                freebuffer(bufnum);
                sendsocket(sock);
        }
}

void rst_special(buffer, iplen)
unsigned char *buffer;
int iplen;
{
        register unsigned char *sktp;
        sktp = SOKAT(0);
        snag32(&buffer[12], &sktp[IPDEST(0, 0)]);
        snag16(&buffer[iplen], &sktp[DESPRT(0, 0)]);
        snag16(&buffer[iplen + 2], &sktp[SRCPRT(0, 0)]);
        snag32(&buffer[iplen + 4], &sktp[ACKNUM(0, 0)]);
        snag32(&buffer[iplen + 8], &sktp[SEQNUM(0, 0)]);
        tcp_rst(0); /* use special buffer */
}

void listenproc(bufnum, sock, iplen)
unsigned int bufnum;
int sock;
int iplen;
{
        register unsigned char *sktp;
        unsigned char command;
        unsigned char *buffer;
        unsigned char *ours;

#if DEBUGD
        printf("\nlistenproc\n");
#endif
        buffer = bufptrs[bufnum];
        command = buffer[iplen + 13];
        if(command & SYN) {
                sktp = SOKAT(sock);
                sktp[SOCKST(0, 0)] = (unsigned char) STATE_SYN_RECEIVED;
                snag16(PRTSRC(buffer, iplen), DESPRT(sock, sockets));
                ours = &sktp[ACKNUM(0, 0)];
                snag32(NUMSEQ(buffer, iplen), ours);
                inc32word(ours);
                ours = &sktp[SEQNUM(0, 0)];
                sav32word(ours, isnclock);
                snag32(DESTIP(buffer), &sktp[IPDEST(0, 0)]);
                tcp_synandack(sock);
                timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* keep */
                sockexpecting[sock] = 1; /* keep */
                errortx[sock] = 0;
#if DEBUGD
                printf("%i YES!\n", sock);
#endif
        } else {
#if DEBUGD
                printf("%i NO!\n", sock);
#endif
                rst_special(buffer, iplen);
        }
        /* free the inbound buffers */
        freebuffer(bufnum);
}

void syn_sentproc(bufnum, sock, iplen)
unsigned int bufnum;
int sock;
int iplen;
{
        register unsigned char *sktp;
        unsigned char command;
        unsigned char *buffer;
        unsigned long iack;
        unsigned long iseq;
        unsigned long oseq;

#if DEBUGD
        printf("\nsyn_sentproc\n");
#endif
        buffer = bufptrs[bufnum];
        command = buffer[iplen + 13];
        sktp = SOKAT(sock);
        iack = ret32word(NUMACK(buffer, iplen));
        iseq = ret32word(NUMSEQ(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        if(command & SYN) {
                inc32word(&sktp[ACKNUM(0, 0)]);
                sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
                if((command & ACK) && (iack == (oseq + 1))) {
                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_ESTABLISHED;
                        inc32word(&sktp[SEQNUM(0, 0)]);
                        sav32word(&sktp[ACKNUM(0, 0)], (iseq + 1));
                        sockexpecting[sock] = 0;
                } else {
                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_SYN_RECEIVED;
                }
                errortx[sock] = 0;
        } else {
                /* RST other end to avoid a loop */
                rst_special(buffer, iplen);
        }
        /* free the inbound buffer as we are done with it now */
        freebuffer(bufnum);
        /* task the other side */
        sendsocket(sock);
}

void syn_receivedproc(bufnum, sock, iplen)
unsigned int bufnum;
int sock;
int iplen;
{
        register unsigned char *sktp;
        unsigned char command;
        unsigned char *buffer;
        unsigned long iack;
        unsigned long oseq;

#if DEBUGD
        printf("\nsyn_receivedproc\n");
#endif
        buffer = bufptrs[bufnum];
        command = buffer[iplen + 13];
        sktp = SOKAT(sock);
        iack = ret32word(NUMACK(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        if(command & SYN) {
                sktp[BUFSTA(0, 0)] = (unsigned char) (SYN | ACK);
                sendsocket(sock);
                errortx[sock] = 0;
                timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* XXX */
                sockexpecting[sock] = 1; /* XXX */
        }
        if((command & ACK) && (iack == (oseq + 1))) {
                sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
                sendsocket(sock);
                inc32word(&sktp[SEQNUM(0, 0)]);
                sockexpecting[sock] = errortx[sock] = 0;
                sktp[SOCKST(0, 0)] = (unsigned char) STATE_ESTABLISHED;
        }
        /* free the inbound buffers */
        freebuffer(bufnum);
}

void establishedproc(bufnum, sock, iplen, tcplen, datalen)
unsigned int bufnum;
int sock;
int iplen;
int tcplen;
int datalen;
{
        register unsigned char *sktp;
        unsigned char sockstate;
        unsigned char command;
        unsigned char *buffer;
        unsigned char *front;
        unsigned char *back;
        unsigned long iack;
        unsigned long oack;
        unsigned long iseq;
        unsigned long oseq;
        unsigned long expected;
        int count;
        unsigned long diff;


#if DEBUGD
        printf("\nestablishedproc\n");
#endif
        buffer = bufptrs[bufnum];
        sktp = SOKAT(sock);
        sockstate = sktp[SOCKST(0, 0)];
        command = buffer[iplen + 13];
        iack = ret32word(NUMACK(buffer, iplen));
        oack = ret32word(&sktp[ACKNUM(0, 0)]);
        iseq = ret32word(NUMSEQ(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        if(!((unsigned char) (command & ACK))) {

#if DEBUGD
                printf("tcp.c: command bug %2.2x\n", command);
#endif
                freebuffer(bufnum);
                return;
        }
        /* process ack value in packet */
        expected = sockdlen[sock];
        /* this ugly conglomeration of shit handles 0xffffffff rollover/rollbacks */

#if DEBUGD
        printf("oseq %8.8lX iack %8.8lX\n", oseq, iack);
#endif

        diff = (unsigned long) (((unsigned long) (oseq + expected)) - iack);

        if((unsigned long) diff <= (unsigned long) expected) {
                count = expected - diff;
#if DEBUGD
                printf("expected - diff = count (%8.8lX - %8.8lX = %4.4X)\n", expected, diff, count);
#endif
                sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
                errortx[sock] = 0;
                front = wdatapointers[sock];
                back = wdatapointers[sock] + count;
                wbufcount[sock] -= count;
                sockdlen[sock] -= count;
                sav32word(&sktp[SEQNUM(0, 0)], (unsigned long) (oseq + (unsigned long) (count)));
                sockexpecting[sock] = ((!sockdlen[sock])) ? 0 : 1;

#if DEBUGM
                printf("%4.4x OCTETS SENT, BUFFERS: %4.4x %4.4x\n", count, wbufcount[sock], sockdlen[sock]);
                printf("MEMMOVE -> TO %4.4x FROM %4.4x FOR %4.4x OCTECTS TCP.C, line %i\n", back, front, wbufcount[sock], __LINE__);
#endif
                /* horrible on cpu, could use a ring buffer here */
                memmove(front, back, (wbufcount[sock]));
        }
        /*
        we should hit this only if:
        1: we have data
        2: we recieved data
        3: any fin flags set
         */
        if(((unsigned char) (command & ~ACK)) || (datalen)) {
                processtcpdata(bufnum, sock, iplen, tcplen, datalen);
                /* the inbound buffers are already free */
        } else {
                freebuffer(bufnum);
        }
}

void finwait1proc(bufnum, sock, iplen, tcplen, datalen)
unsigned int bufnum;
int sock;
int iplen;
int tcplen;
int datalen;
{
        register unsigned char *sktp;
        unsigned char sockstate;
        unsigned char command;
        unsigned char *buffer;

#if DEBUGD
        printf("\nfinwait1proc\n");
#endif
        buffer = bufptrs[bufnum];
        sktp = SOKAT(sock);
        sockstate = sktp[SOCKST(0, 0)];
        command = buffer[iplen + 13];
#if DEBUGD
        printf("finwait1proc: command %2.2x\n", command);
#endif
        if(!((unsigned char) (command & FIN))) {
                /* toss it, this is an error */
                freebuffer(bufnum);
                return;
        }

        processtcpdata(bufnum, sock, iplen, tcplen, datalen);
}

void finwait2proc(bufnum, sock, iplen, tcplen, datalen)
unsigned int bufnum;
int sock;
int iplen;
int tcplen;
int datalen;
{
        register unsigned char *sktp;
        unsigned char sockstate;
        unsigned char command;
        unsigned char *buffer;
        unsigned long iack;
        unsigned long oseq;

#if DEBUGD
        printf("\nfinwait2proc\n");
#endif
        buffer = bufptrs[bufnum];
        sktp = SOKAT(sock);
        sockstate = sktp[SOCKST(0, 0)];
        command = buffer[iplen + 13];
        iack = ret32word(NUMACK(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        errortx[sock] = 0;
        sktp[BUFSTA(0, 0)] = (unsigned char) (FIN | ACK);
        if((command & FIN) && ((unsigned long) (iack - oseq) == 0L)) {
                sockexpecting[sock] = 1; /* this is just to time the slob out keep */
                sktp[SOCKST(0, 0)] = (unsigned char) STATE_TIME_WAIT;
                sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
        }
        processtcpdata(bufnum, sock, iplen, tcplen, datalen); /* answer it */
        /* the inbound buffers are already free */
}

void closingproc(bufnum, sock, iplen)
unsigned int bufnum;
int sock;
int iplen;
{
        register unsigned char *sktp;
        unsigned char *buffer;
        unsigned long iack;
        unsigned long oseq;


#if DEBUGD
        printf("\nclosingproc\n");
#endif
        buffer = bufptrs[bufnum];
        sktp = SOKAT(sock);
        iack = ret32word(NUMACK(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        if(iack == (oseq + 1)) {
                freebuffer(bufnum);
                if(rbufcount[sock]) {
                        errortx[sock] = 0;
                        sktp[BUFSTA(0, 0)] = (unsigned char) (ACK);
                } else {
                        sktp[SOCKST(0, 0)] = (unsigned char) STATE_TIME_WAIT;
                        errortx[sock] = 0;
                        sktp[BUFSTA(0, 0)] = (unsigned char) (ACK | FIN);
                        sendsocket(sock);
                }
        } else {
#if DEBUGD
                printf("\nOWCH@@!\007\n");
                printf("Socket %i %lu:%lu\n", sock, iack, oseq);
#endif
                /* free the inbound buffers */
                freebuffer(bufnum);
                sktp[BUFSTA(0, 0)] = (unsigned char) (ACK | FIN);
                sendsocket(sock);
                sockexpecting[sock] = 1; /* XXX */
                timertx[sock] = CHK_TIME1 + (loopcali * RETRYTIME); /* XXX */
                errortx[sock] = 9;
        }
}

void last_ackproc(bufnum, sock, iplen)
unsigned int bufnum;
int sock;
int iplen;
{
        register unsigned char *sktp;
        unsigned char *buffer;
        unsigned long iack;
        unsigned long oseq;


#if DEBUGD
        printf("\nlast_ackproc\n");
#endif
        buffer = bufptrs[bufnum];
        sktp = SOKAT(sock);
        iack = ret32word(NUMACK(buffer, iplen));
        oseq = ret32word(&sktp[SEQNUM(0, 0)]);
        if(iack == (oseq + 1)) {
                sockexpecting[sock] = sockdlen[sock] = errortx[sock] = 0;
                nuke_tcp_socket(sock);
        } else {
                sktp[BUFSTA(0, 0)] = (unsigned char) (ACK | FIN);
                sendsocket(sock);
                errortx[sock] = 0;
        }
        /* free the inbound buffers */
        freebuffer(bufnum);
}

void time_waitproc(bufnum, sock)
unsigned int bufnum;
int sock;
{
#if OK2WAIT
        register unsigned char *sktp;
#if DEBUGD
        printf("\ntime_waitproc\n");
#endif
        sktp = SOKAT(sock);
        sktp[BUFSTA(0, 0)] = (unsigned char) ACK;
        errortx[sock] = 0;
        sendsocket(sock);
        sockexpecting[sock] = 1; /* this is just to time the slob out keep */
#else
        /* slam the door shut */
        nuke_tcp_socket(sock);
#endif
}

void proctcp(bufnum, sock, iplen, tcplen, datalen)
unsigned int bufnum;
int sock;
int iplen;
int tcplen;
int datalen;
{
        register unsigned char *sktp;
        register unsigned char sockstate;

        sktp = SOKAT(sock);
        sockstate = sktp[SOCKST(0, 0)];
#if DEBUGD
        printf("\nproctcp sock %i state %2.2x ->", sock, sockstate);
#endif

        switch(sockstate) {
                case (unsigned char) STATE_LISTEN:
                        listenproc(bufnum, sock, iplen);
                        break;

                case (unsigned char) STATE_SYN_SENT:
                        syn_sentproc(bufnum, sock, iplen);
                        break;

                case (unsigned char) STATE_SYN_RECEIVED:
                        syn_receivedproc(bufnum, sock, iplen);
                        break;

                case (unsigned char) STATE_ESTABLISHED:
                        establishedproc(bufnum, sock, iplen, tcplen, datalen);
                        break;

                case (unsigned char) STATE_CLOSING:
                        closingproc(bufnum, sock, iplen);
                        break;

                case (unsigned char) STATE_FINWAIT1:
                        finwait1proc(bufnum, sock, iplen, tcplen, datalen);
                        break;

                case (unsigned char) STATE_FINWAIT2:
                        finwait2proc(bufnum, sock, iplen, tcplen, datalen);
                        break;

                case (unsigned char) STATE_LAST_ACK:
                        last_ackproc(bufnum, sock, iplen);
                        break;

                case (unsigned char) STATE_TIME_WAIT:
                        time_waitproc(bufnum, sock);
                        /* fall thru */
                default:
                        freebuffer(bufnum);
                        break;
        }
#if DEBUGD
        printf("proctcp sock %i state %2.2x\n", sock, sockstate);
#endif
        return;
}

/* rx TCP RFC 0793 */
void tcp(bufnum, iplen, tcplen)
unsigned int bufnum;
unsigned int iplen;
unsigned int tcplen;
{
        register unsigned char *sktp;
        register unsigned char *buffer;
        unsigned int datalen;
        unsigned int sum;
        unsigned int tsum;
        unsigned char a;
        unsigned char b;
        unsigned char fakebuf[13];
        int sock;

        sock = -1;

        buffer = bufptrs[bufnum];
        datalen = tcplen - ((buffer[iplen + 12] & 0xf0) / 4);

        fakebuf[ 0] = buffer[12];
        fakebuf[ 1] = buffer[13];
        fakebuf[ 2] = buffer[14];
        fakebuf[ 3] = buffer[15];

        fakebuf[ 4] = buffer[16];
        fakebuf[ 5] = buffer[17];
        fakebuf[ 6] = buffer[18];
        fakebuf[ 7] = buffer[19];

        fakebuf[ 8] = 0x00;
        fakebuf[ 9] = 0x06;
        fakebuf[10] = (tcplen / 256) & 0xff;
        fakebuf[11] = tcplen & 0xff;

#if DEBUGD

        printf("\nTCP ");

#endif
        a = buffer[iplen + 16];
        b = buffer[iplen + 17];
        buffer[iplen + 16] = buffer[iplen + 17] = 0x00;
        tsum = checksum(&buffer[iplen], tcplen, fakebuf, 12);
        sum = (a * 256) + b;
        /* packet flag type */
        a = buffer[iplen + 13];
        if(sum == tsum) {
                /* sum is good, check the ports */
                sock = matchsocket(&buffer[iplen]);
                if(sock < 0) {

#if DEBUGI
                        printf("No match on socket ");
#endif
                        if(!((unsigned char) (a & (RST)))) {

                                if((unsigned char) (a & (SYN)) == (unsigned char) SYN) {
                                        /* type 3, code 3 port unreachable */
                                        err_reply(bufnum, 3, 3, 0);
                                } else {
                                        /* send a RST reply or, if this is a FIN, finack it */
                                        sock = 0;
                                        sktp = SOKAT(sock);
                                        snag32(&buffer[12], &sktp[IPDEST(0, 0)]);
                                        snag16(&buffer[iplen], &sktp[DESPRT(0, 0)]);
                                        snag16(&buffer[iplen + 2], &sktp[SRCPRT(0, 0)]);
                                        snag32(&buffer[iplen + 4], &sktp[ACKNUM(0, 0)]);
                                        snag32(&buffer[iplen + 8], &sktp[SEQNUM(0, 0)]);
#if 0
                                        if((unsigned char) (a & FIN) == (unsigned char) FIN) {
                                                if((unsigned char) (a & (FIN | ACK)) == (unsigned char) (FIN | ACK)) {
                                                        tcp_fin(sock);
                                                } else {
                                                        tcp_finack(sock);
                                                }
                                        } else {
                                                tcp_rst(sock); /* use special buffer */
                                        }
#else
                                        tcp_rst(sock); /* use special buffer */
#endif
                                        freebuffer(bufnum);
                                }
                        }
                } else {
                        sktp = SOKAT(sock);
#if DEBUGI
                        printf("socket %i ", sock);
                        fflush(stdout);
#endif
                        /* socket state */
                        b = sktp[SOCKST(0, 0)];
                        if((unsigned char) (a & 0xc0)) {
#if DEBUGI
                                printf("Malformed flags ");
#endif
                                freebuffer(bufnum);

                        } else {
                                if((unsigned char) (a & RST) == (unsigned char) RST) {
                                        /* is it an *active* socket? */
                                        if(sktp[SOCKST(0, 0)] > (unsigned char) STATE_NOTINIT) {
                                                /* do a gentle reset of the socket */
                                                sktp[SOCKST(0, 0)] = (unsigned char) STATE_PEER_RESET;
                                                sockdlen[sock] = wbufcount[sock] = rbufcount[sock] = rbuftail[sock] = rbufhead[sock] = 0;
#if DEBUGI
                                                printf("Gentle reset by peer ");
#endif
                                                tcp_rst(sock); /* use special buffer */
                                        } /* else we ignore the reset */
                                        freebuffer(bufnum);
                                } else {
#if DEBUGI
                                        printf("Packet OK flags are %2.2x ", a);
#endif
                                        /* passes most header checks, process packet */
                                        proctcp(bufnum, sock, iplen, tcplen, datalen);
                                }
                        }
                }
        } else {
#if DEBUGI
                printf("Bad checksum\n");
#endif
                freebuffer(bufnum); /* bad checksum */
        }/* endif sum == tsum */

#if DEBUGI || DEBUGD
        putchar('\n');
#endif
        return;
}



#else /* TCP_ENABLED */

/* no tcp available, so.... */
void tcp(bufnum, loh, lop, lod)
unsigned int bufnum
unsigned int loh;
unsigned int lop;
unsigned int lod;
{
        /* type 3, code 2 protocol unreachable */
        err_reply(bufnum, 3, 2, 0);
}
#endif /* TCP_ENABLED */

#else /* IP_ENABLED */
#endif /* IP_ENABLED */

/* tell link parser who we are and where we belong

0700 libznet.lib tcp.obj xxLIBRARYxx

 */

