/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

#ifndef NETINET_H_SEEN
#define NETINET_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

/* ip "struct" */
#define DESTIP(x) (x + 12)

/* tcp "struct" */
#define PRTSRC(x,y) (x + y + 0)
#define PRTDES(x,y) (x + y + 2)
#define NUMSEQ(x,y) (x + y + 4)
#define NUMACK(x,y) (x + y + 8)

#include "net.h"

/* unsynchronized states */
#define STATE_CLOSED 		0x00
/* these three misc socket states are placeholders for socket functions */
#define STATE_BIND 		0x01
#define STATE_BOUND 		0x02
#define STATE_LISTEN 		0x03
#define STATE_NOTINIT 		0x06
/* special "state" for UDP */
#define STATE_UDP 		0x0f
/* states from this point onwords use timer */
#define STATE_SYN_SENT 		0x10
#define STATE_SYN_RECEIVED 	0x20
/* synchronized states */
#define STATE_ESTABLISHED 	0x40
#define STATE_CLOSEWAIT 	0x80
#define STATE_CLOSING 		0x81
#define STATE_FINWAIT1 		0x82
#define STATE_FINWAIT2 		0x83
#define STATE_LAST_ACK 		0x84
#define STATE_PEER_RESET 	0xfe
#define STATE_TIME_WAIT 	0xff

#define URG 0x20
#define ACK 0x10
#define PSH 0x08
#define RST 0x04
#define SYN 0x02
#define FIN 0x01

#endif /* NETINET_H_SEEN */
