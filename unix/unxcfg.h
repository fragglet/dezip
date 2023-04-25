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
#undef LARGE_FILE_SUPPORT
#endif

/* NO_ZIP64_SUPPORT takes preceedence over ZIP64_SUPPORT */
#if defined(NO_ZIP64_SUPPORT) && defined(ZIP64_SUPPORT)
#undef ZIP64_SUPPORT
#endif

#ifdef LARGE_FILE_SUPPORT
/* 64-bit Large File Support */

/* The following Large File Summit (LFS) defines turn on large file support
   on Linux (probably 2.4 or later kernel) and many other unixen */

/* These have to be before any include that sets types so the large file
   versions of the types are set in the includes */

#define _LARGEFILE_SOURCE /* some OSes need this for fseeko */
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64 /* select default interface as 64 bit */
#define _LARGE_FILES         /* some OSes need this for 64-bit off_t */
#define __USE_LARGEFILE64
#endif /* LARGE_FILE_SUPPORT */

#include <sys/types.h> /* off_t, time_t, dev_t, ... */
#include <sys/stat.h>
#include <unistd.h>

typedef struct stat z_stat;
#define Z_STAT_DEFINED

#include <fcntl.h> /* O_BINARY for open() w/o CR/LF translation */

#ifndef NO_PARAM_H
#include <sys/param.h> /* conflict with <sys/types.h>, some systems? */
#endif                 /* !NO_PARAM_H */

#include <time.h>

#include <unistd.h> /* this includes utime.h on SGIs */
#include <utime.h>
#define GOT_UTIMBUF

#if (defined(_MBCS) && defined(NO_MBCS))
/* disable MBCS support when requested */
#undef _MBCS
#endif

#ifndef DATE_FORMAT
#define DATE_FORMAT DF_MDY /* GRR:  customize with locale.h somehow? */
#endif
#define lenEOL 1

#endif /* !__unxcfg_h */
