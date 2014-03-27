/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* compat.h various patches/work arounds for OS's */
#ifndef COMPAT_H_SEEN
#define COMPAT_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif
#ifdef	__cplusplus
extern "C" {
#endif

#ifdef SAFEMEMORY
/*
Crash prevention, takes a little bit more ram
 */
extern void *safemalloc(size_t);
#define MALLOC safemalloc
extern void *REALLOC(void *, size_t);
extern void FREE(void *);
#else

/* ram is at a premium... */
#define safemalloc malloc
#define MALLOC malloc
#define REALLOC realloc
#define FREE free
#endif

/*
Compatability support for missing OS features, and hardware.
 */
#ifndef EAGAIN
#define EAGAIN 11 /* Try again */
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN /* Operation would block */
#endif
#ifndef ENOSYS
#define ENOSYS 88 /* Function not implemented */
#endif
extern void AJKNETusage K__P F__P(char *, char *, char *, char *, char *)P__F;
extern int fixgetch K__P F__P(void)P__F;
extern int fixkbhit K__P F__P(void)P__F;

/*
Numeric compatability.
 */
extern void inc32word K__P F__P(unsigned char *)P__F;
extern void inc16word K__P F__P(unsigned char *)P__F;
extern void sav32word K__P F__P(unsigned char *, unsigned long)P__F;
extern void snag32 K__P F__P(unsigned char *, unsigned char *)P__F;
extern void snag16 K__P F__P(unsigned char *, unsigned char *)P__F;
extern unsigned long ret32word K__P F__P(unsigned char *)P__F;

#define CMPGT 0x01
#define CMPLT 0x02
#define CMPEQU 0x03
#define CMPNEQ 0x04
#define CMPLEQ 0x05
#define CMPGEQ 0x06

#ifdef	__cplusplus
}
#endif

#endif /* COMPAT_H_SEEN */
