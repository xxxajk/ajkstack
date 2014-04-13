/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpvars.h"
#include "compat.h"


#ifdef SAFEMEMORY

/* just incase the code wedges on an unchecked malloc... */
void * safemalloc(nw)
size_t nw;
{
        register void * nm;

        printf("SAFE MALLOC of %i bytes\n", nw);
        nm = malloc(nw);
        if(!nm) {
                printf("Out of memory\n");
                exit(1);
        }
        printf("Success!\n_M_ %lx < \n", (unsigned long) nm);
        fflush(stdout);
        return(nm);
}

/* just incase the code wedges on an unchecked realloc... */
void * REALLOC(om, nw)
void * om;
size_t nw;
{
        register void * nm;

        printf("SAFE REALLOC of %i bytes\n_O_ %lx < \n", nw, (unsigned long) om);
        fflush(stdout);
        if(!om) {
                printf("Realloc of NULL attempted!\n");
                exit(1);
        }
        nm = realloc(om, nw);
        if(!nm) {
                printf("Out of memory\n");
                exit(1);
        }
        printf("Success!\n_R_ %lx < \n", (unsigned long) nm);
        fflush(stdout);
        return(nm);
}

void FREE(nm)
void *nm;
{
        printf("Free\n_F_ %lx > \n", (unsigned long) nm);
        free(nm);

}
#endif

/* very simple terse usage */
void AJKNETusage(Fprgogname, version, Fexecute, programname, syntax)
char *Fprgogname;
char *version;
char *Fexecute;
char *programname;
char *syntax;
{

#if NO_OS
        printf("\n%s, Version %s\nStack Version %s\n\n", Fprgogname, version, STACKVERSION);
        printf("Syntax:\n%s %s\n\n", Fprgogname, syntax);
#else
#ifdef CPM
        printf("\n%s, Version %s\nStack Version %s\n\n", Fprgogname, version, STACKVERSION);
        printf("Syntax:\n%s %s\n\n", Fprgogname, syntax);
#else
        printf("\n%s, Version %s\nStack Version %s\n\n", programname, version, STACKVERSION);
        printf("Syntax:\n%s %s\n\n", programname, syntax);
#endif
        exit(1);
#endif
}

void snag32(from, to)
unsigned char *from;
unsigned char *to;
{
        /* copy block of 4, incase bcopy dosen't exist... */
        to[0] = from[0];
        to[1] = from[1];
        to[2] = from[2];
        to[3] = from[3];
}

void snag16(from, to)
unsigned char *from;
unsigned char *to;
{
        /* copy block of 2, incase bcopy dosen't exist... */
        to[0] = from[0];
        to[1] = from[1];
}

/* BIG ENDIAN */
void inc32word(workspace)
unsigned char *workspace;
{
        workspace[3]++;
        if(!workspace[3]) {
                workspace[2]++;
                if(!workspace[2]) {
                        workspace[1]++;
                        if(!workspace[1]) {
                                workspace[0]++;
                        }
                }
        }
}

/* BIG ENDIAN */
void inc16word(workspace)
unsigned char *workspace;
{
        workspace[1]++;
        if(!workspace[1]) workspace[0]++;
}

/* 32bit endian flip */
unsigned long ret32word(workspace)
unsigned char *workspace;
{
        /* ugly compiler bug work arround */
        unsigned long foo;
        unsigned char *bleh;

        foo = 0L;
        bleh = (unsigned char *) &foo;
        bleh[3] = workspace[0];
        bleh[2] = workspace[1];
        bleh[1] = workspace[2];
        bleh[0] = workspace[3];
        return(foo);
}

/* flip endianness and store  */
void sav32word(workspace, val)
unsigned char *workspace;
unsigned long val;
{
        unsigned long foo;
        register unsigned char *bleh;

        foo = val;
        bleh = (unsigned char *) &foo;

        workspace[0] = bleh[3];
        workspace[1] = bleh[2];
        workspace[2] = bleh[1];
        workspace[3] = bleh[0];
}

/* tell link parser who we are and where we belong

1200 libznet.lib compat.obj xxLIBRARYxx

 */

