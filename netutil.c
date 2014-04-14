/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

/*
current network utilities.

DNS lookups

More to come dependent upon need.

IMPORTANT!
NETWORK MUST BE ALREADY UP AND INITIALIZED!
"MAKEMEBLEED" is needed for all systems.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"
#include "netutil.h"
#include "in.h"
#include "netinet.h"
#include "socket.h"
#include "timers.h"
#include "compat.h"

#define WORKSPACE_SIZE 512
#define NAME_SIZE 16

/**
 *
 * @param p pre-allocated buffer area to store converted name
 * @param domain_name string to convert
 * @return pointer to byte after the conversion
 */
static unsigned char *install_domain_name(p, domain_name)
unsigned char *p;
char *domain_name;
{
        *p++ = '.';
        strcpy((char *) p, domain_name);
        p--;

        while(*p != '\0') {
                if(*p == '.') {
                        unsigned char *end = p + 1;
                        while(*end != '.' && *end != '\0') end++;
                        *p = end - p - 1;
                }
                p++;
        }
        return p + 1;
}

/**
 * Find first matching record.
 * This routine will NOT do a lookup of a hex address, or any other weird formats!
 * Caller must free RAM.
 * To prevent poisoning/attacks/hijacks from the TO-DO limitations below,
 * use your own DNS server on your LAN. Many ISPs will provide an address to a web
 * page that spews something from a search, filled with ads, so it is usually
 * preferred to avoid ISP for DNS anyway.
 * Using 8.8.8.8 (Google) is a good choice in a pinch.
 *
 * TO-DO: The ID field might not be random. Use CRC of the query string?
 * TO-DO: The ID field is overwritten, and can not be verified. (above will fix it)
 * TO-DO: Needs to verify the packet is from the server IP we queried.
 *
 * @param hostinfo domain to query
 * @param qtype Type of query, 0x01 (A), 0x0C (PTR) or 0x0F (MX)
 * @return first matching answer, NULL on error
 */
struct u_nameip *findhost(hostinfo, qtype)
const char *hostinfo;
unsigned char qtype;
{
        int socket_handle;
        unsigned char *p; // Points into the workspace array.
        struct sockaddr_in srv_addy; /* bind */
        struct sockaddr_in to; /* sendto */
        struct sockaddr_in from; /* recvfrom */
        int j;
        int k;
        int len;
        long now;
        unsigned char mov;
        unsigned char *ptr;
        unsigned char *nextbp;
        int noreply = 0;
        struct u_nameip *result = NULL;
        unsigned char *workspace = NULL; // Used to hold datagrams.
        unsigned char *ipp;
        char *hostp;
        MAKEMEBLEED();

        if(!hostinfo) goto no_workspace;
        if(strlen(hostinfo) > 255 || strlen(hostinfo) == 0) goto no_workspace;
        // Allocate everything. This is going to eat .76KB. Caller must free result and pointers in it.
        // TO-DO: Optimize by reusing workspace, thus reducing use to .5KB
        workspace = MALLOC(WORKSPACE_SIZE);
        if(!workspace) {
                goto no_workspace;
        }
        result = MALLOC(sizeof(struct u_nameip));
        if(!result) {
                goto success;
        }

        ipp = MALLOC(4);
        if(!(ipp)) {
                goto oom1;
        }
        result->hostip = ipp;
        hostp = malloc(256);
        if(!(hostp)) {
                goto oom2;
        }
        result->hostname = hostp;
        // parse the query type
        srv_addy.sin_family = AF_INET;
        srv_addy.sin_port = 8192; /* This should be free a port... */
        p = (unsigned char *) (&srv_addy.sin_addr);
        *p = 0;
        p++;
        *p = 0;
        p++;
        *p = 0;
        p++;
        *p = 0;

        to.sin_family = AF_INET;
        to.sin_port = 53;
        p = (unsigned char *) (&to.sin_addr);
        *p = mydns[0];
        p++;
        *p = mydns[1];
        p++;
        *p = mydns[2];
        p++;
        *p = mydns[3];
        socket_handle = AJK_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(socket_handle > -1) {
                /* The first two bytes are random number. Whatever is in ram will work fine. */
                memset(workspace + 2, 0, WORKSPACE_SIZE - 2);
                p = workspace + 2;
                /* forward lookup */
                *p++ = 0x01;
                p += 2;
                *p++ = 0x01;
                p += 6;
                p = install_domain_name(p, hostinfo);
                p++;
                *p++ = qtype;
                p++;
                *p++ = 0x01;

                ptr = p;
                len = p - workspace;
                j = AJK_bind(socket_handle, (struct sockaddr_in *) &srv_addy, sizeof(srv_addy));
                if(j>-1) {
                        noreply = 1;
                        while(noreply) {
                                k = AJK_sendto(socket_handle, workspace, len, 0, (struct sockaddr *) &to, sizeof(to));
                                if(k < 0) {
                                        k = 0;
                                        break;
                                }
                                if(!k) {
                                        now = CHK_TIME1 + (loopcali * RETRYTIME);
                                        /* packet was sent, we need to wait a bit for a reply */
                                        do {
                                                k = AJK_sockdepth(socket_handle, 0);
                                                if(now < CHK_TIME1) break;
                                        } while(k < 1);
                                        if(k) noreply = 29; /* exit out */
                                }
                                noreply++;
                                if(noreply == 30) noreply = 0; /* exit out */
                        }
                        if(k) {
                                k = AJK_recvfrom(socket_handle, workspace, WORKSPACE_SIZE, 0, (struct sockaddr *) &from, sizeof(from));
                                AJK_sockclose(socket_handle);

                                p = workspace;
                                p += 3;
                                if(*p & 0x0f) {
                                        goto error_cleanup; // error
                                }
                                p += 3;
                                int answers = (*p * 256) + *(p + 1);
                                if(!answers) {
                                        goto error_cleanup; // error
                                }
                                p = ptr; // p now points at the first answer rr
                                while(answers) {
                                        answers--;
                                        if((*p & 0xC0) == 0xC0) {
                                                // Compression is being used.
                                                p += 10;
                                        } else {
                                                while(*p && (*p < 64)) p++;
                                                p += 10;
                                        }
                                        // p now points at the data length
                                        nextbp = p + 2 + ((*p) * 256) + *(p + 1);
                                        p += 2;
                                        if(qtype == *(p - 9)) {
                                                // we're interested in this record
                                                if(qtype == 0x01) {
                                                        // give IP, in network byte order
                                                        *ipp++ = *p++;
                                                        *ipp++ = *p++;
                                                        *ipp++ = *p++;
                                                        *ipp++ = *p++;
                                                        goto success;
                                                }
                                                if(qtype == 0x0c || qtype == 0x0f) {
                                                        // give name
                                                        ptr = (unsigned char *)hostp;
                                                        if(qtype == 0x0f) {
                                                                // skip preference
                                                                p += 2;
                                                        }
                                                        while(*p != 0x00) {
                                                                if(*p > 0xbf) {
                                                                        p = workspace + ((*p & 0x3f)*256) + *(p + 1);
                                                                } else {
                                                                        *ptr = *p;
                                                                        ptr++;
                                                                        p++;
                                                                }
                                                        }
                                                        *ptr = 0x00;
                                                        p = (unsigned char *)hostp;

                                                        while(*p) {
                                                                mov = 1 + *p;
                                                                *p = '.';
                                                                p += mov;
                                                        }
                                                        p = (unsigned char *)hostp;
                                                        while(*p) {
                                                                *p = *(p + 1);
                                                                p++;
                                                        }
                                                        goto success;
                                                }

                                        }
                                        p = nextbp;
                                }
                        } else {
                                AJK_sockclose(socket_handle);
                        }
                } else {
                        AJK_sockclose(socket_handle);
                }
        }

error_cleanup:
        FREE(result->hostname);
oom2:
        FREE(result->hostip);
oom1:
        FREE(result);
        result = NULL;
success:
        FREE(workspace);
no_workspace:
        return(result);

}

/**
 * Forward and reverse DNS lookup.
 * Caller must free RAM.
 *
 * @param hostinfo either decimal dotted quad, or a hostname
 * @return host info, NULL on error
 */
struct u_nameip *gethostandip(hostinfo)
const char *hostinfo;
{
        char *ptr = (char *)hostinfo;
        int dots = 0;
        int letters = 0;
        int a = -1;
        int b = -1;
        int c = -1;
        int d = -1;
        struct u_nameip *result = NULL;
        unsigned char qtype = 0x01; // forward lookup (A record)
        // if hostinfo is fully numeric, and three dots do PTR lookup
        if(ptr) {
                if(strlen(ptr) > 255 || strlen(ptr) == 0) return NULL;
                while(*ptr) {
                        if(*ptr == '.') {
                                dots++;
                        } else if(*ptr < '0' || *ptr > '9') {
                                letters++;
                        }
                        ptr++;
                }
                ptr = (char *)hostinfo;
                if(letters == 0 && dots == 3) {
                        // check that all 4 numbers are <255
                        sscanf(hostinfo, "%d.%d.%d.%d", &a, &b, &c, &d);
                        if(a < 256 && b < 256 && c < 256 && d < 256) {
                                // form a reverse lookup query, and change lookup type to ptr.
                                qtype = 0x0c; // reverse lookup (PTR record)
                                ptr = NULL;
                                ptr = MALLOC(29);
                                if(ptr) {
                                        sprintf(ptr, "%d.%d.%d.%d.in-addr.arpa", d, c, b, a);
                                }
                        }
                }
        }
        if(ptr) {
                // do the lookup.
                result = findhost(ptr, qtype);
                if(qtype == 0x0c) {
                        FREE(ptr);
                }
                if(result) {
                        if(qtype == 0x0c) {
                                ptr=(char *)result->hostip;
                                // fill in what we know
                                *ptr++ = a;
                                *ptr++ = b;
                                *ptr++ = c;
                                *ptr++ = d;
                        } else {
                                // Fill in what we know.
                                // NOTE: Not canonical unless you entered it as such.
                                // To get the canonical name, lookup the IP that is returned.
                                ptr=result->hostname;
                                sprintf(ptr, "%s", hostinfo);
                        }
                }
        }
        return result;
}
/* tell link parser who we are and where we belong

0000 libutil.lib netutil.obj xxLIBRARYxx

 */
