/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
#ifndef UNPROTO_H_SEEN

#define UNPROTO_H_SEEN


/* fix your problimnatic compiler here! */
#ifdef __BCC__
#define STUPID_COMPILER 1
#endif

#ifndef STUPID_COMPILER
#define STUPID_COMPILER 0
#endif

#if STUPID_COMPILER
/*
        fixes for strict K&R type compilers
        that barf on ANSI prototypes.
        (bcc is one of these)
        this is taken directly from the features.h
        file of bcc on my system, plus additional
        fixes that allows it to work alot better.
 */

#define F__P 	\
(
#define P__F 	)

#ifdef __STDC__

#define K__P(x) x
#define __const const

/* Almost ansi */
#if __STDC__ != 1
#define const
#define volatile
#endif

#else /* K&R */

#define K__P(x) ()
#define __const
#define const
#define volatile

#endif

#define VOIDFIX

#else
/*
        No need to fixup any prototype for ANSI.
 */
#define VOIDFIX void
#define K__P
#define F__P
#define P__F
#endif /* STUPID_COMPILER */


#endif /* UNPROTO_H_SEEN */
