/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/* chcksum.c  Calculates 1's complement checksum */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chcksum.h"
#include "tcpvars.h"
#include "ioconfig.h"



#ifdef HI_TECH_C
/* use the asm version. */
#include <sys.h>

unsigned int checksum(buffer, len, xbuffer, xlen)
unsigned char *buffer;
unsigned int len;
unsigned char *xbuffer;
unsigned int xlen;
{
        return(_IPcomp_(_IPsum_(buffer, len), _IPsum_(xbuffer, xlen)));
}
#else /* ! HI_TECH_C */

unsigned int checksum(buffer, len, xbuffer, xlen)
unsigned char *buffer;
unsigned int len;
unsigned char *xbuffer;
unsigned int xlen;
{
        /* manual math here because of buggy compiler */
        register unsigned int zipper;
        register unsigned long sum;
        register unsigned char *bufptr;
        sum = 0LU;
        if(xlen) {
                bufptr = xbuffer;
                for(zipper = (xlen / 2); zipper > 0; zipper--) {
                        sum += ((unsigned short)(bufptr[0]) << 8) + (bufptr[1]);
                        bufptr += 2;
                }
        }

        bufptr = buffer;
        /* sum it */
        for(zipper = (len / 2); zipper > 0; zipper--) {
                sum += ((unsigned short)(bufptr[0]) << 8) + (bufptr[1]);
                bufptr += 2;
        }

        if((len & 1)) {
                sum += ((unsigned short)(bufptr[0] & 0xff) << 8);
        }


        while(sum > 0xffff)
                sum = (sum & 0xffff) + (sum >> 16);
        sum = ~sum;

        return((unsigned int) (sum & 0xffff));
}

#endif /* HI_TECH_C */

/* tell link parser who we are and where we belong

0900 libznet.lib chcksum.obj xxLIBRARYxx

 */

