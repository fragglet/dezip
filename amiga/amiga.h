/*
  Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* amiga.h
 *
 * Globular definitions that affect all of AmigaDom.
 *
 * Originally included in unzip.h, extracted for simplicity and eeze of
 * maintenance by John Bush.
 *
 * THIS FILE IS #INCLUDE'd by unzip.h
 *
 */

#ifndef __amiga_amiga_h
#define __amiga_amiga_h

#include "amiga/z-stat.h"     /* substitute for <stat.h> and <direct.h> */
#include "amiga/z-time.h"                    /* substitute for <time.h> */
#include <limits.h>
#ifndef NO_FCNTL_H
#  include <fcntl.h>
#else
   int mkdir(const char *_name);
#endif

#ifdef AZTEC_C                       /* Manx Aztec C, 5.0 or newer only */
#  include <clib/dos_protos.h>
#  include <pragmas/dos_lib.h>           /* do inline dos.library calls */
#  define O_BINARY 0
#  define direct dirent

#  define DECLARE_TIMEZONE
#  define ASM_INFLATECODES
#  define ASM_CRC

/* Note that defining REENTRANT will not eliminate all global/static */
/* variables.  The functions we use from c.lib, including stdio, are */
/* not reentrant.  Neither are the stuff in amiga/stat.c or the time */
/* functions in amiga/filedate.c, because they just augment c.lib.   */
/* If you want a fully reentrant and reexecutable "pure" UnZip with  */
/* Aztec C, assemble and link in the startup module purify.a by Paul */
/* Kienitz.  REENTRANT should be used just to reduce memory waste.   */
#endif /* AZTEC_C */


#ifdef __SASC
/* includes */
#  include <sys/types.h>
#  include <sys/dir.h>
#  include <dos.h>
#  include <exec/memory.h>
#  include <exec/execbase.h>
#  if (defined(_M68020) && (!defined(__USE_SYSBASE)))
                            /* on 68020 or higher processors it is faster   */
#    define __USE_SYSBASE   /* to use the pragma libcall instead of syscall */
#  endif                    /* to access functions of the exec.library      */
#  include <proto/exec.h>   /* see SAS/C manual:part 2,chapter 2,pages 6-7  */
#  include <proto/dos.h>
#  include <proto/locale.h>

#  ifdef DEBUG
#    include <sprof.h>      /* profiler header file */
#  endif
#  if ( (!defined(O_BINARY)) && defined(O_RAW))
#    define O_BINARY O_RAW
#  endif
#  if (defined(_SHORTINT) && !defined(USE_FWRITE))
#    define USE_FWRITE      /* define if write() returns 16-bit int */
#  endif
#  if (!defined(REENTRANT) && !defined(FUNZIP))
#    define REENTRANT      /* define if unzip is going to be pure */
#  endif
#  if defined(REENTRANT) && defined(DYNALLOC_CRCTAB)
#    undef DYNALLOC_CRCTAB
#  endif
#  ifdef MWDEBUG
#    include <stdio.h>      /* both stdio.h and stdlib.h must be included */
#    include <stdlib.h>     /* before memwatch.h                          */
#    include "memwatch.h"
#    undef getenv
#  endif /* MWDEBUG */
#endif /* SASC */


#define MALLOC_WORK
#define USE_EF_UT_TIME
#if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#  define TIMESTAMP
#endif

/* check that TZ environment variable is defined before using UTC times */
#if (!defined(NO_IZ_CHECK_TZ) && !defined(IZ_CHECK_TZ))
#  define IZ_CHECK_TZ
#endif

#define AMIGA_FILENOTELEN 80
#ifndef DATE_FORMAT
#  define DATE_FORMAT     DF_MDY
#endif
#define lenEOL            1
#define PutNativeEOL      *q++ = native(LF);
#define PIPE_ERROR        0

#ifdef GLOBAL         /* crypt.c usage conflicts with AmigaDOS headers */
#  undef GLOBAL
#endif

/* Funkshine Prough Toe Taipes */

int Agetch (void);              /* getch() like function, in amiga/filedate.c */
LONG FileDate (char *, time_t[]);
int windowheight (BPTR fh);
void _abort(void);              /* ctrl-C trap */

#define SCREENLINES windowheight(Output())

#ifdef USE_TIME_LIB
   extern int real_timezone_is_set;
#  define VALID_TIMEZONE(tempvar) (tzset(), real_timezone_is_set)
#else
#  define VALID_TIMEZONE(tempvar) ((tempvar = getenv("TZ")) && tempvar[0])
#endif

/* Static variables that we have to add to Uz_Globs: */
#define SYSTEM_SPECIFIC_GLOBALS \
    int filenote_slot;\
    char *(filenotes[DIR_BLKSIZ]);\
    int created_dir, renamed_fullpath, rootlen;\
    char *rootpath, *buildpath, *build_end;\
    DIR *wild_dir;\
    ZCONST char *wildname;\
    char *dirname, matchname[FILNAMSIZ];\
    int dirnamelen, notfirstcall;

/* filenotes[] and filenote_slot are for the -N option that restores      */
/*    comments of Zip archive entries as AmigaDOS filenotes.  The others  */
/*    are used by functions in amiga/amiga.c only.                        */
/* created_dir and renamed_fullpath are used by mapname() and checkdir(). */
/* rootlen, rootpath, buildpath, and build_end are used by checkdir().    */
/* wild_dir, dirname, wildname, matchname[], dirnamelen and notfirstcall  */
/*    are used by do_wild().                                              */
#endif /* __amiga_amiga_h */
