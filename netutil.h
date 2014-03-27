/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */


/*
current network utilities.

DNS lookups

More to come dependent upon need.

 */


#ifndef NETUTIL_H_SEEN
#define NETUTIL_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#include "in.h"
#ifdef	__cplusplus
extern "C" {
#endif

extern unsigned char mydns[4];

struct u_nameip {
        char * hostname;
        char * hostip;
};

extern struct u_nameip * findhost K__P F__P(const char *, unsigned char)P__F;
extern struct u_nameip * gethostandip K__P F__P(const char *)P__F;
#ifdef	__cplusplus
}
#endif

#endif /* NETUTIL_H_SEEN */
