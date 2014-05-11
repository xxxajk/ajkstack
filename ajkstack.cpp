/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ajkstack.h"
#include "tcpvars.h"
#include "ioconfig.h"

#if USEARDUINO
#include <Arduino.h>
#include "ardser.h"
#include <xmem.h>
#include <xmemUSB.h>
#include <HardwareSerial.h>

#define OP_LISTEN       0x01
#define OP_SOCKET       0x02
#define OP_READY        0x03
#define OP_ACCEPT       0x04
#define OP_BIND         0x05
#define OP_CONNECT      0x06
#define OP_READ         0x07
#define OP_WRITE        0x08
#define OP_RECV         0x09
#define OP_SEND         0x0A
#define OP_CLOSE        0x0B
#define OP_SELECT       0x0C
#define OP_DNS_LOOKUP   0x0D
#define OP_BINDCOUNT    0x0E

static memory_stream to_ipstack_task;
static memory_stream from_ipstack_task;

typedef struct {
        uint8_t op;
        int fd;
        int intarg;
} __attribute__((packed)) op_fd_int;

typedef struct {
        uint8_t op;
        int domain;
        int type;
        int protocol;
} __attribute__((packed)) op_socket;

typedef struct {
        uint8_t op;
        int fd;
} __attribute__((packed)) op_fd_only;

typedef struct {
        int rv;
        struct sockaddr addr;
} __attribute__((packed)) accept_reply;

typedef struct {
        uint8_t op;
        int fd;
        struct sockaddr addr;
} __attribute__((packed)) op_socket_bc;

typedef struct {
        uint8_t op;
        int fd;
        size_t count;
        uint8_t buf_beginning;
} __attribute__((packed)) op_socket_write;

typedef struct {
        uint8_t op;
        int fd;
        size_t count;
} __attribute__((packed)) op_socket_read;

typedef struct {
        int rv;
        uint8_t buf_beginning;
} __attribute__((packed)) op_socket_read_reply;

typedef struct {
        uint8_t op;
        int sockfd;
        size_t len;
} __attribute__((packed)) op_recv;

typedef struct {
        int rv;
        struct sockaddr src_addr;
        uint8_t buf;
} __attribute__((packed)) op_recv_reply;

typedef struct {
        uint8_t op;
        int sockfd;
        size_t len;
        struct sockaddr dest_addr;
        uint8_t buf;
} __attribute__((packed)) op_send;

typedef struct {
        uint8_t op;
        uint8_t buf;
} __attribute__((packed)) op_dns_lookup;

typedef struct {
        int rv;
        uint8_t ip[4];
        uint8_t name;
} __attribute__((packed)) op_dns_lookup_reply;

extern "C" {
        volatile uint8_t network_booted = 0;

        void SLIPDEVbegin(long s) {
                SLIPDEV.begin(s);
        }

        // WARNING: _NOT_ thread safe!

        int fixgetch(void) {
                return getchar();
        }

        // WARNING: _NOT_ thread safe!

        int fixkbhit(void) {
                if(KONSOLE.available()) return 1;
                return 0;
        }

        int SLIPDEVavailable(void) {
                if(SLIPDEV.available()) {
                        return 1;
                }
                return 0;
        }

        unsigned char SLIPDEVread(void) {
                return (SLIPDEV.read() & 0xff);
        }

        size_t SLIPDEVwrite(unsigned char c) {
                return SLIPDEV.write(c);
        }

        void TCPIPBegin(void) {
                initnetwork(0, NULL);
                network_booted = 1;
        }

        ///////////////////////////////////////////////////////////////////////////////
        //       Multitasking specific                                               //
        ///////////////////////////////////////////////////////////////////////////////

        int listen(int sockfd, int backlog) {
                op_fd_int *message = (op_fd_int *)xmem::safe_malloc(sizeof (op_fd_int));
                int *reply;
                int rv;
                message->op = OP_LISTEN;
                message->fd = sockfd;
                message->intarg = backlog;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_int), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int socket(int domain, int type, int protocol) {
                op_socket *message = (op_socket *)xmem::safe_malloc(sizeof (op_socket));
                int *reply;
                int rv;
                message->op = OP_SOCKET;
                message->domain = domain;
                message->type = type;
                message->protocol = protocol;
                xmem::memory_send((uint8_t*)message, sizeof (op_socket), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int socket_close(int sockfd) {
                op_fd_only *message = (op_fd_only *)xmem::safe_malloc(sizeof (op_fd_only));
                int *reply;
                int rv;
                message->op = OP_CLOSE;
                message->fd = sockfd;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_only), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int accept_ready(int sockfd) {
                op_fd_only *message = (op_fd_only *)xmem::safe_malloc(sizeof (op_fd_only));
                int *reply;
                int rv;
                message->op = OP_READY;
                message->fd = sockfd;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_only), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int bindcount(int sockfd) {
                op_fd_only *message = (op_fd_only *)xmem::safe_malloc(sizeof (op_fd_only));
                int *reply;
                int rv;
                message->op = OP_BINDCOUNT;
                message->fd = sockfd;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_only), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
                // addrlen is not used
                op_fd_only *message = (op_fd_only *)xmem::safe_malloc(sizeof (op_fd_only));
                accept_reply *reply;
                int rv;
                message->op = OP_ACCEPT;
                message->fd = sockfd;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_only), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = reply->rv;
                memcpy(addr, &reply->addr, sizeof (struct sockaddr));
                free(reply);
                return rv;
        }

        int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
                // addrlen is not used
                op_socket_bc *message = (op_socket_bc *)xmem::safe_malloc(sizeof (op_socket_bc));
                int *reply;
                int rv;
                message->op = OP_BIND;
                message->fd = sockfd;
                memcpy(&message->addr, addr, sizeof (struct sockaddr));
                xmem::memory_send((uint8_t*)message, sizeof (op_socket_bc), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
                // addrlen is not used
                op_socket_bc *message = (op_socket_bc *)xmem::safe_malloc(sizeof (op_socket_bc));
                int *reply;
                int rv;
                message->op = OP_CONNECT;
                message->fd = sockfd;
                memcpy(&message->addr, addr, sizeof (struct sockaddr));
                xmem::memory_send((uint8_t*)message, sizeof (op_socket_bc), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int socket_read(int fd, void *buf, size_t count) {
                op_socket_read_reply *reply;
                int rv;
                op_socket_read *message = (op_socket_read *)xmem::safe_malloc(sizeof (op_socket_read));
                message->op = OP_READ;
                message->fd = fd;
                message->count = count;
                xmem::memory_send((uint8_t*)message, sizeof (op_socket_read), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = reply->rv;
                if(rv > 0) {
                        memcpy(buf, &reply->buf_beginning, rv);
                }
                free(reply);
                return rv;

        }

        int socket_write(int fd, const void *buf, size_t count) {
                int *reply;
                int rv;
                op_socket_write *message = (op_socket_write *)xmem::safe_malloc(sizeof (op_socket_write) + count);
                message->op = OP_WRITE;
                message->fd = fd;
                message->count = count;
                memcpy(&message->buf_beginning, buf, count);
                xmem::memory_send((uint8_t*)message, sizeof (op_socket_write) + count, &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
                int rv;
                op_recv_reply *reply;
                op_recv *message = (op_recv *)(op_recv *)xmem::safe_malloc(sizeof (op_recv));
                message->op = OP_RECV;
                message->sockfd = sockfd;
                message->len = len;
                xmem::memory_send((uint8_t*)message, sizeof (op_recv), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = reply->rv;
                *src_addr = reply->src_addr;
                if(rv > 0) {
                        memcpy(buf, &reply->buf, rv);
                }
                free(reply);
                return rv;
        }

        int sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
                int *reply;
                int rv;
                op_send *message = (op_send *)xmem::safe_malloc(sizeof (op_send) + len);
                message->op = OP_SEND;
                message->sockfd = sockfd;
                message->len = len;
                memcpy(&message->buf, buf, len);
                xmem::memory_send((uint8_t*)message, sizeof (op_send) + len, &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int socket_select(int sockfd, int rw) {
                op_fd_int *message = (op_fd_int *)xmem::safe_malloc(sizeof (op_fd_int));
                int *reply;
                int rv;
                message->op = OP_SELECT;
                message->fd = sockfd;
                message->intarg = rw;
                xmem::memory_send((uint8_t*)message, sizeof (op_fd_int), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = *reply;
                free(reply);
                return rv;
        }

        int resolve(const void *name, struct u_nameip *buf) {
                int rv;
                op_dns_lookup_reply *reply;
                op_dns_lookup *message = (op_dns_lookup *)xmem::safe_malloc(sizeof (op_dns_lookup) + strlen((char *)name));
                message->op = OP_DNS_LOOKUP;
                strcpy((char *)&(message->buf), (char *)name);
                xmem::memory_send((uint8_t*)message, sizeof (op_dns_lookup) + strlen((char *)name), &to_ipstack_task);
                free(message);
                xmem::memory_recv((uint8_t**)(&reply), &from_ipstack_task);
                rv = reply->rv;
                if(!rv) {
                        buf->hostip = (unsigned char *)xmem::safe_malloc(4);
                        buf->hostname = (char *)xmem::safe_malloc(1 + strlen((char *)reply->name));
                        memcpy(buf->hostip, &(reply->ip), 4);
                        strcpy((char *)buf->hostname, (char *)&(reply->name));
                }
                free(reply);
                return rv;
        }
}

void IP_task(void) {
        uint8_t *message;
        uint8_t *reply;
        struct u_nameip *nip;
        int rv;
        uint16_t rlen;
        int len;
        xmem::memory_init(&to_ipstack_task);
        xmem::memory_init(&from_ipstack_task);
        TCPIPBegin();
        for(;;) {
                MAKEMEBLEED();
                if(xmem::memory_ready(&to_ipstack_task)) {
                        message = NULL;
                        reply = NULL;
                        nip = NULL;
                        rv = -1;
                        rlen = sizeof (int);
                        len = xmem::memory_recv(&message, &to_ipstack_task);
                        if(len) {
                                uint8_t *ptr = message;
                                uint8_t d = *ptr;
                                ptr++;
                                switch(d) {
                                        case OP_LISTEN:
                                                rv = AJK_listen(((op_fd_int *)message)->fd, ((op_fd_int *)message)->intarg);
                                                break;
                                        case OP_SOCKET:
                                                rv = AJK_socket(((op_socket *)message)->domain, ((op_socket *)message)->type, ((op_socket *)message)->protocol);
                                                break;
                                        case OP_ACCEPT:
                                                rlen = sizeof (accept_reply);
                                                reply = (uint8_t *)xmem::safe_malloc(rlen);
                                                rv = AJK_accept(((op_fd_only *)message)->fd, (sockaddr_in *)(&((accept_reply *)reply)->addr), 0);
                                                break;
                                        case OP_BIND:
                                                rv = AJK_bind(((op_socket_bc *)message)->fd, (sockaddr_in *)(&((op_socket_bc *)message)->addr), 0);
                                                break;
                                        case OP_CONNECT:
                                                rv = AJK_connect(((op_socket_bc *)message)->fd, (sockaddr_in *)(&((op_socket_bc *)message)->addr), 0);
                                                break;
                                        case OP_READ:
                                                rlen = sizeof (op_socket_read_reply) + ((op_socket_read *)message)->count;
                                                reply = (uint8_t *)xmem::safe_malloc(rlen);
                                                rv = AJK_sockread(((op_socket_read *)message)->fd, &(((op_socket_read_reply *)reply)->buf_beginning), ((op_socket_read *)message)->count);
                                                break;
                                        case OP_RECV:
                                                rlen = sizeof (op_recv_reply)+((op_recv *)message)->len;
                                                reply = (uint8_t *)xmem::safe_malloc(rlen);
                                                rv = AJK_recvfrom(((op_recv *)message)->sockfd, &(((op_recv_reply *)reply)->buf), ((op_recv *)message)->len, 0, &((op_recv_reply *)reply)->src_addr, 0);
                                                break;
                                        case OP_WRITE:
                                                rv = AJK_sockwrite(((op_socket_write *)message)->fd, &(((op_socket_write *)message)->buf_beginning), ((op_socket_write *)message)->count);
                                                break;
                                        case OP_SEND:
                                                rv = AJK_sendto(((op_send *)message)->sockfd, &(((op_send *)message)->buf), ((op_send *)message)->len, 0, &(((op_send *)message)->dest_addr), 0);
                                                break;
                                        case OP_READY:
                                                rv = AJK_acceptready(((op_fd_only *)message)->fd);
                                                break;
                                        case OP_BINDCOUNT:
                                                rv = AJK_bindcount(((op_fd_only *)message)->fd);
                                                break;
                                        case OP_CLOSE:
                                                rv = AJK_sockclose(((op_fd_only *)message)->fd);
                                                break;
                                        case OP_SELECT:
                                                rv = AJK_sockdepth(((op_fd_int *)message)->fd, ((op_fd_int *)message)->intarg);
                                                break;
                                        case OP_DNS_LOOKUP:
                                                nip = gethostandip((const char *)&((op_dns_lookup*)message)->buf);
                                                if(nip) {
                                                        rv = 0;
                                                        rlen = sizeof (op_dns_lookup_reply) + strlen(nip->hostname);
                                                        reply = (uint8_t *)xmem::safe_malloc(rlen);
                                                        memcpy(&((op_dns_lookup_reply *)reply)->ip, (nip->hostip), 4);
                                                        strcpy((char *)&((op_dns_lookup_reply *)reply)->name, (char *)(nip->hostname));
                                                        free(nip->hostip);
                                                        free(nip->hostname);
                                                        free(nip);
                                                }
                                                break;
                                        default:
                                                break;
                                }
                                free(message);
                                if(!reply) {
                                        reply = (uint8_t *)xmem::safe_malloc(sizeof (int));
                                }
                                *(int *)reply = rv;
                                MAKEMEBLEED();
                                xmem::memory_send(reply, rlen, &from_ipstack_task);
                                MAKEMEBLEED();
                                free(reply);
                        }
                } else xmem::Yield();
        }
}

#endif /* USEARDUINO */
