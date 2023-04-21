/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    Unix specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __unxcfg_h
#define __unxcfg_h


/* LARGE FILE SUPPORT - 10/6/04 EG */
/* This needs to be set before the includes so they set the right sizes */

#if (defined(NO_LARGE_FILE_SUPPORT) && defined(LARGE_FILE_SUPPORT))
#  undef LARGE_FILE_SUPPORT
#endif

/* Automatically set ZIP64_SUPPORT if LFS */
#ifdef LARGE_FILE_SUPPORT
#endif

/* NO_ZIP64_SUPPORT takes preceedence over ZIP64_SUPPORT */
#if defined(NO_ZIP64_SUPPORT) && defined(ZIP64_SUPPORT)
#  undef ZIP64_SUPPORT
#endif

#ifdef LARGE_FILE_SUPPORT
  /* 64-bit Large File Support */

  /* The following Large File Summit (LFS) defines turn on large file support
     on Linux (probably 2.4 or later kernel) and many other unixen */

  /* These have to be before any include that sets types so the large file
     versions of the types are set in the includes */

# define _LARGEFILE_SOURCE      /* some OSes need this for fseeko */
# define _LARGEFILE64_SOURCE
# define _FILE_OFFSET_BITS 64   /* select default interface as 64 bit */
# define _LARGE_FILES           /* some OSes need this for 64-bit off_t */
# define __USE_LARGEFILE64
#endif /* LARGE_FILE_SUPPORT */


#include <sys/types.h>          /* off_t, time_t, dev_t, ... */
#include <sys/stat.h>
#include <unistd.h>

  typedef off_t zoff_t;
#define ZOFF_T_DEFINED
typedef struct stat z_stat;
#define Z_STAT_DEFINED

#  include <fcntl.h>            /* O_BINARY for open() w/o CR/LF translation */

#ifndef NO_PARAM_H
#  include <sys/param.h>    /* conflict with <sys/types.h>, some systems? */
#endif /* !NO_PARAM_H */

#  include <time.h>
   struct tm *gmtime(), *localtime();

#  include <unistd.h>           /* this includes utime.h on SGIs */
#  if (defined(BSD4_4) || defined(linux) || defined(__GLIBC__))
#    include <utime.h>
#    define GOT_UTIMBUF
#  endif

#if (defined(_MBCS) && defined(NO_MBCS))
   /* disable MBCS support when requested */
#  undef _MBCS
#endif

#if (!defined(NO_SETLOCALE) && !defined(_MBCS))
#endif

#if (!defined(HAVE_STRNICMP) & !defined(NO_STRNICMP))
#  define NO_STRNICMP
#endif
#ifndef DATE_FORMAT
#  define DATE_FORMAT DF_MDY    /* GRR:  customize with locale.h somehow? */
#endif
#define lenEOL          1
#  define PutNativeEOL  *q++ = native(LF);
#define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
#define SCREENWIDTH     80
#define SCREENLWRAP     1
#define USE_EF_UT_TIME
#  define SET_SYMLINK_ATTRIBS
#  define SET_DIR_ATTRIB
#  define RESTORE_UIDGID

/* Static variables that we have to add to Uz_Globs: */
#define SYSTEM_SPECIFIC_GLOBALS \
    int created_dir, renamed_fullpath;\
    char *rootpath, *buildpath, *end;\
    ZCONST char *wildname;\
    char *dirname, matchname[FILNAMSIZ];\
    int rootlen, have_dirname, dirnamelen, notfirstcall;\
    zvoid *wild_dir;

/* created_dir, and renamed_fullpath are used by both mapname() and    */
/*    checkdir().                                                      */
/* rootlen, rootpath, buildpath and end are used by checkdir().        */
/* wild_dir, dirname, wildname, matchname[], dirnamelen, have_dirname, */
/*    and notfirstcall are used by do_wild().                          */

#endif /* !__unxcfg_h */
