/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
/*
        Add your hardware access methods here.
        Don't forget to wrap your code with a proper ifdef!

        TODO: automatically generate this file.
 */
#ifndef IOCONFIG_H_SEEN
#define IOCONFIG_H_SEEN

#ifndef CONFIG_H_SEEN
#include "config.h"
#endif

#if NO_OS
#include "ardser.h"
#include "PICxxCDC.h"
#include "T3xEEM.h"
#else
#include "z80dart.h"
#include "cpm3aux.h"
#include "linuxser.h"
#include "elksser.h"
#include "dosuart.h"
#include "z80sio.h"
#include "silver.h"
#include "c128acia.h"
#endif
#if USESIMPLESLIP
#include "slip.h"
#endif
#if USEARCNET
#include "arcnet.h"
#endif
#endif /* IOCONFIG_H_SEEN */
