/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

#ifndef AJKSTACK_H
#define	AJKSTACK_H
#include <ctype.h>
#include <stddef.h>
#include "compat.h"
/* various fixups, os dependent */
#include "net.h"
#include "FScapi.h"
#include "in.h"
#include "netinet.h"
#include "netutil.h"
#include "socket.h"
#include "tcpvars.h"

#ifdef	__cplusplus
extern "C" {
#endif
extern volatile uint8_t network_booted;

// C++ callbacks for the stack
extern void SLIPDEVbegin(long s);
extern int SLIPDEVavailable(void);
extern unsigned char SLIPDEVread(void);
extern size_t SLIPDEVwrite(unsigned char c);
extern void TCPIPBegin(void);

// C++ callbacks for multitasking.
extern int socket(int domain, int type, int protocol);
extern int accept_ready(int sockfd);
extern int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
extern int listen(int sockfd, int backlog);
extern int socket_read(int fd, void *buf, size_t count);
extern int recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern int sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern int socket_write(int fd, const void *buf, size_t count);
extern int socket_close(int sockfd);
extern int socket_select(int sockfd, int rw);
extern int resolve(const void *name, struct u_nameip *buf);
extern int bindcount(int sockfd);

#ifdef	__cplusplus
}
// C++ only...
extern void IP_task(void);

#endif

#endif	/* AJKSTACK_H */
