/*
  Copyright (c) 1990-2004 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
   ttyio.h
 */

#ifndef __ttyio_h   /* don't include more than once */
#define __ttyio_h

#ifndef __crypt_h
#  include "crypt.h"  /* ensure that encryption header file has been seen */
#endif

#if (CRYPT || (defined(UNZIP) && !defined(FUNZIP)))
/*
 * Non-echo keyboard/console input support is needed and enabled.
 */

#ifndef __G         /* UnZip only, for now (DLL stuff) */
#  define __G
#  define __G__
#  define __GDEF
#  define __GPRO    void
#  define __GPRO__
#endif

#ifndef ZCONST      /* UnZip only (until have configure script like Zip) */
#  define ZCONST const
#endif

#if (defined(MSDOS) || defined(OS2) || defined(WIN32))
#    define DOS_OS2_W32
#endif

#if (defined(__ATHEOS__) || defined(__BEOS__) || defined(UNIX))
#endif

#if (defined(VM_CMS) || defined(MVS))
#    define CMS_MVS
#endif


/* Function prototypes */

/* The following systems supply a `non-echo' character input function "getch()"
 * (or an alias) and do not need the echoff() / echon() function pair.
 */

/* For VM/CMS and MVS, we do not (yet) have any support to switch terminal
 * input echo on and off. The following "fake" definitions allow inclusion
 * of crypt support and UnZip's "pause prompting" features, but without
 * any echo suppression.
 */

/* The THEOS C runtime library supplies the function conmask() to toggle
 * terminal input echo on (conmask("e")) and off (conmask("n")).  But,
 * since THEOS C RTL also contains a working non-echo getch() function,
 * the echo toggles are not needed.
 */

/* VMS has a single echo() function in ttyio.c to toggle terminal
 * input echo on and off.
 */
#ifdef VMS
#  define echoff(f)  echo(0)
#  define echon()    echo(1)
#  define getch()    tt_getch()
#  define FGETCH(f)  tt_getch()
   int echo OF((int));
   int tt_getch OF((void));
#endif

/* For all other systems, ttyio.c supplies the two functions Echoff() and
 * Echon() for suppressing and (re)enabling console input echo.
 */
#ifndef echoff
#  define echoff(f)  Echoff(__G__ f)
#  define echon()    Echon(__G)
   void Echoff OF((__GPRO__ int f));
   void Echon OF((__GPRO));
#endif

/* this stuff is used by MORE and also now by the ctrl-S code; fileio.c only */
#if (defined(UNZIP) && !defined(FUNZIP))
#  ifdef HAVE_WORKING_GETCH
#    define FGETCH(f)  getch()
#  endif
#  ifndef FGETCH
     /* default for all systems where no getch()-like function is available */
     int zgetch OF((__GPRO__ int f));
#    define FGETCH(f)  zgetch(__G__ f)
#  endif
#endif /* UNZIP && !FUNZIP */

#if (CRYPT && !defined(WINDLL))
   char *getp OF((__GPRO__ ZCONST char *m, char *p, int n));
#endif

#else /* !(CRYPT || (UNZIP && !FUNZIP)) */

/*
 * No need for non-echo keyboard/console input; provide dummy definitions.
 */
#define echoff(f)
#define echon()

#endif /* ?(CRYPT || (UNZIP && !FUNZIP)) */

#endif /* !__ttyio_h */
