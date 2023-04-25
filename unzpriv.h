/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unzpriv.h

  This header file contains private (internal) macros, typedefs, prototypes
  and global-variable declarations used by all of the UnZip source files.
  In a prior life it was part of the main unzip.h header, but now it is only
  included by that header if UNZIP_INTERNAL is defined.

  ---------------------------------------------------------------------------*/

#ifndef __unzpriv_h /* prevent multiple inclusions */
#define __unzpriv_h

#define DYNAMIC_CRC_TABLE

#include "unix/unxcfg.h"

#ifndef Z_STAT_DEFINED
typedef struct stat z_stat;
#define Z_STAT_DEFINED
#endif

#include <stdio.h>

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <signal.h>

#include <stddef.h>
#include <stdlib.h>
typedef size_t extent;

#define UNZIP_VERSION     46
#define VMS_UNZIP_VERSION 42 /* if OS-needed-to-extract is VMS:  can do */

/* clean up with a few defaults */
#ifndef DIR_END
#define DIR_END '/' /* last char before program name or filename */
#endif
#ifndef DATE_FORMAT
#define DATE_FORMAT DF_YMD /* defaults to invariant ISO-style */
#endif
#ifndef DATE_SEPCHAR
#define DATE_SEPCHAR '-'
#endif
#ifndef CLOSE_INFILE
#define CLOSE_INFILE() close(G.zipfd)
#endif

/* defaults that we hope will take care of most machines in the future */

#define MSG_STDERR(f) (f & 1)        /* bit 0:  0 = stdout, 1 = stderr */
#define MSG_INFO(f)   ((f & 6) == 0) /* bits 1 and 2:  0 = info */
#define MSG_WARN(f)   ((f & 6) == 2) /* bits 1 and 2:  1 = warning */
#define MSG_ERROR(f)  ((f & 6) == 4) /* bits 1 and 2:  2 = error */
#define MSG_FATAL(f)  ((f & 6) == 6) /* bits 1 and 2:  (3 = fatal error) */
#define MSG_ZFN(f)    (f & 0x0008)   /* bit 3:  1 = print zipfile name */
#define MSG_FN(f)     (f & 0x0010)   /* bit 4:  1 = print filename */
#define MSG_LNEWLN(f) (f & 0x0020)   /* bit 5:  1 = leading newline if !SOL */
#define MSG_TNEWLN(f) (f & 0x0040)   /* bit 6:  1 = trailing newline if !SOL */
#define MSG_MNEWLN(f) (f & 0x0080)   /* bit 7:  1 = trailing NL for prompts */
/* the following are subject to change */
#define MSG_NO_WGUI(f) (f & 0x0100) /* bit 8:  1 = skip if Windows GUI */
#define MSG_NO_AGUI(f) (f & 0x0200) /* bit 9:  1 = skip if Acorn GUI */
#define MSG_NO_DLL2(f) (f & 0x0400) /* bit 10:  1 = skip if OS/2 DLL */
#define MSG_NO_NDLL(f) (f & 0x0800) /* bit 11:  1 = skip if WIN32 DLL */
#define MSG_NO_WDLL(f) (f & 0x1000) /* bit 12:  1 = skip if Windows DLL */

#define DIR_BLKSIZ 16384 /* use more memory, to reduce long-range seeks */

#ifndef WSIZE
#define WSIZE 65536L /* window size--must be a power of two, and */
#endif

#ifndef INBUFSIZ
#define INBUFSIZ 8192 /* larger buffers for real OSes */
#endif

/* Logic for case of small memory, length of EOL > 1:  if OUTBUFSIZ == 2048,
 * OUTBUFSIZ>>1 == 1024 and OUTBUFSIZ>>7 == 16; therefore rawbuf is 1008 bytes
 * and transbuf 1040 bytes.  Have room for 32 extra EOL chars; 1008/32 == 31.5
 * chars/line, smaller than estimated 35-70 characters per line for C source
 * and normal text.  Hence difference is sufficient for most "average" files.
 * (Argument scales for larger OUTBUFSIZ.)
 */
#define OUTBUFSIZ   (lenEOL * WSIZE) /* more efficient text conversion */
#define TRANSBUFSIZ (lenEOL * OUTBUFSIZ)
typedef int shrint; /* for efficiency/speed, we hope... */
#define RAWBUFSIZ OUTBUFSIZ

/* The LZW patent is expired worldwide since 2004-Jul-07, so USE_UNSHRINK
 * is now enabled by default.  See unshrink.c.
 */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN /* in <sys/param.h> on some systems */
#else
#ifdef _MAX_PATH
#define PATH_MAX _MAX_PATH
#else
#if FILENAME_MAX > 255
#define PATH_MAX FILENAME_MAX /* used like PATH_MAX on some systems */
#else
#define PATH_MAX 1024
#endif
#endif /* ?_MAX_PATH */
#endif /* ?MAXPATHLEN */
#endif /* !PATH_MAX */

/*
 * buffer size required to hold the longest legal local filepath
 * (including the trailing '\0')
 */
#define FILNAMSIZ PATH_MAX

/* 2007-09-18 SMS.
 * Include <locale.h> here if it will be needed later for Unicode.
 * Otherwise, setlocale may be defined here, and then defined again
 * (differently) when <locale.h> is read later.
 */
#include <wchar.h>
#ifndef _MBCS /* no need to include <locale.h> twice, see below */
#include <locale.h>
#endif

/* DBCS support for Info-ZIP  (mainly for japanese (-: )
 * by Yoshioka Tsuneo (QWF00133@nifty.ne.jp,tsuneo-y@is.aist-nara.ac.jp)
 */
#ifdef _MBCS
#include <locale.h>
/* Multi Byte Character Set */
#define ___MBS_TMP_DEF char *___tmp_ptr;
#define ___TMP_PTR     ___tmp_ptr
#ifndef CLEN
#define NEED_UZMBCLEN
#define CLEN(ptr) (int) uzmbclen((const unsigned char *) (ptr))
#endif
#ifndef PREINCSTR
#define PREINCSTR(ptr) (ptr += CLEN(ptr))
#endif
#define POSTINCSTR(ptr) \
    (___TMP_PTR = (char *) (ptr), PREINCSTR(ptr), ___TMP_PTR)
char *plastchar(const char *ptr, extent len);
#define lastchar(ptr, len) ((int) (unsigned) *plastchar(ptr, len))
#ifndef MBSCHR
#define NEED_UZMBSCHR
#define MBSCHR(str, c) (char *) uzmbschr((const unsigned char *) (str), c)
#endif
#ifndef MBSRCHR
#define NEED_UZMBSRCHR
#define MBSRCHR(str, c) (char *) uzmbsrchr((const unsigned char *) (str), c)
#endif
#else /* !_MBCS */
#define ___MBS_TMP_DEF
#define ___TMP_PTR
#define CLEN(ptr)           1
#define PREINCSTR(ptr)      (++(ptr))
#define POSTINCSTR(ptr)     ((ptr)++)
#define plastchar(ptr, len) (&ptr[(len) -1])
#define lastchar(ptr, len)  (ptr[(len) -1])
#define MBSCHR(str, c)      strchr(str, c)
#define MBSRCHR(str, c)     strrchr(str, c)
#endif /* ?_MBCS */
#define INCSTR(ptr) PREINCSTR(ptr)

#if (defined(MALLOC_WORK) && !defined(MY_ZCALLOC))
/* Any system without a special calloc function */
#ifndef zcalloc
#define zcalloc(items, size) \
    (void *) calloc((unsigned) (items), (unsigned) (size))
#endif
#ifndef zcfree
#define zcfree free
#endif
#endif /* MALLOC_WORK && !MY_ZCALLOC */

#define memzero(dest, len) memset(dest, 0, len)

#ifndef TRUE
#define TRUE 1 /* sort of obvious */
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define SYMLINKS

#ifndef IS_VOLID
#define IS_VOLID(m) ((m) &0x08)
#endif

/***********************************/
/*  LARGE_FILE_SUPPORT             */
/***********************************/
/* This whole section lifted from Zip 3b tailor.h

 * LARGE_FILE_SUPPORT and ZIP64_SUPPORT are automatically
 * set in OS dependent headers (for some ports) based on the port and compiler.
 *
 * E. Gordon 9/21/2003
 * Updated 1/28/2004
 * Lifted and placed here 6/7/2004 - Myles Bennett
 */

#ifndef SSTAT
#define SSTAT stat
#endif

/* Default format_off_t() format selection. */

#ifdef LARGE_FILE_SUPPORT
#define OFF_T_FMT     "ll"
#define OFF_T_HEX_WID "16"
#else /* def LARGE_FILE_SUPPORT */
#define OFF_T_FMT     "l"
#define OFF_T_HEX_WID "8"
#endif /* def LARGE_FILE_SUPPORT */

#define OFF_T_HEX_DOT_WID ("." OFF_T_HEX_WID)

#define OFF_T_NUM 4  /* Number of chambers. */
#define OFF_T_LEN 24 /* Number of characters/chamber. */

#ifndef S_TIME_T_MAX /* max value of signed (>= 32-bit) time_t */
#define S_TIME_T_MAX ((time_t) (ulg) 0x7fffffffL)
#endif
#ifndef U_TIME_T_MAX /* max value of unsigned (>= 32-bit) time_t */
#define U_TIME_T_MAX ((time_t) (ulg) 0xffffffffL)
#endif
#ifdef DOSTIME_MINIMUM /* min DOSTIME value (1980-01-01) */
#undef DOSTIME_MINIMUM
#endif
#define DOSTIME_MINIMUM ((ulg) 0x00210000L)
#ifdef DOSTIME_2038_01_18 /* approximate DOSTIME equivalent of */
#undef DOSTIME_2038_01_18 /*  the signed-32-bit time_t limit */
#endif
#define DOSTIME_2038_01_18 ((ulg) 0x74320000L)

#define ZSUFX     ".zip"
#define ALT_ZSUFX ".ZIP" /* Unix-only so far (only case-sensitive fs) */

#define CENTRAL_HDR_SIG "\001\002" /* the infamous "PK" signature bytes, */
#define LOCAL_HDR_SIG   "\003\004" /*  w/o "PK" (so unzip executable not */
#define END_CENTRAL_SIG "\005\006" /*  mistaken for zipfile itself) */
#define EXTD_LOCAL_SIG  "\007\010" /* [ASCII "\113" == EBCDIC "\080" ??] */

/** internal-only return codes **/
#define IZ_DIR 76 /* potential zipfile is a directory */
/* special return codes for mapname() */
#define MPN_OK          0         /* mapname successful */
#define MPN_INF_TRUNC   (1 << 8)  /* caution - filename truncated */
#define MPN_INF_SKIP    (2 << 8)  /* info  - skipped because nothing to do */
#define MPN_ERR_SKIP    (3 << 8)  /* error - entry skipped */
#define MPN_ERR_TOOLONG (4 << 8)  /* error - path too long */
#define MPN_NOMEM       (10 << 8) /* error - out of memory, file skipped */
#define MPN_CREATED_DIR                                                     \
    (16 << 8)                   /* directory created: set time & permission \
                                 */
#define MPN_VOL_LABEL (17 << 8) /* volume label, but can't set on hard disk */
#define MPN_INVALID   (99 << 8) /* internal logic error, should never reach */
/* mask for internal mapname&checkdir return codes */
#define MPN_MASK 0x7F00
/* error code for extracting/testing extra field blocks */
#define IZ_EF_TRUNC 79 /* local extra field truncated (PKZIP'd) */

/* choice of activities for do_string() */
#define SKIP        0 /* skip header block */
#define DISPLAY     1 /* display archive comment (ASCII) */
#define DISPL_8     5 /* display file comment (ext. ASCII) */
#define DS_FN       2 /* read filename (ext. ASCII, chead) */
#define DS_FN_C     2 /* read filename from central header */
#define DS_FN_L     6 /* read filename from local header */
#define EXTRA_FIELD 3 /* copy extra field into buffer */
#define DS_EF       3

#define DOES_NOT_EXIST   -1 /* return values for check_for_newer() */
#define EXISTS_AND_OLDER 0
#define EXISTS_AND_NEWER 1

#define OVERWRT_QUERY  0 /* status values for G.overwrite_mode */
#define OVERWRT_ALWAYS 1
#define OVERWRT_NEVER  2

#define IS_OVERWRT_ALL  (G.overwrite_mode == OVERWRT_ALWAYS)
#define IS_OVERWRT_NONE (G.overwrite_mode == OVERWRT_NEVER)

#define ROOT        0 /* checkdir() extract-to path:  called once */
#define INIT        1 /* allocate buildpath:  called once per member */
#define APPEND_DIR  2 /* append a dir comp.:  many times per member */
#define APPEND_NAME 3 /* append actual filename:  once per member */
#define GETPATH     4 /* retrieve the complete path and free it */
#define END         5 /* free root path prior to exiting program */

/* version_made_by codes (central dir):  make sure these */
/*  are not defined on their respective systems!! */
#define FS_FAT_   0 /* filesystem used by MS-DOS, OS/2, Win32 */
#define AMIGA_    1
#define VMS_      2
#define UNIX_     3
#define VM_CMS_   4
#define ATARI_    5 /* what if it's a minix filesystem? [cjh] */
#define FS_HPFS_  6 /* filesystem used by OS/2 (and NT 3.x) */
#define MAC_      7 /* HFS filesystem used by MacOS */
#define Z_SYSTEM_ 8
#define CPM_      9
#define TOPS20_   10
#define FS_NTFS_  11 /* filesystem used by Windows NT */
#define QDOS_     12
#define ACORN_    13 /* Archimedes Acorn RISC OS */
#define FS_VFAT_  14 /* filesystem used by Windows 95, NT */
#define MVS_      15
#define BEOS_     16 /* hybrid POSIX/database filesystem */
#define TANDEM_   17 /* Tandem NSK */
#define THEOS_    18 /* THEOS */
#define MAC_OSX_  19 /* Mac OS/X (Darwin) */
#define ATHEOS_   30 /* AtheOS */
#define NUM_HOSTS 31 /* index of last system + 1 */

#define STORED      0 /* compression methods */
#define SHRUNK      1
#define REDUCED1    2
#define REDUCED2    3
#define REDUCED3    4
#define REDUCED4    5
#define IMPLODED    6
#define TOKENIZED   7
#define DEFLATED    8
#define ENHDEFLATED 9
#define DCLIMPLODED 10
#define BZIPPED     12
#define LZMAED      14
#define IBMTERSED   18
#define IBMLZ77ED   19
#define WAVPACKED   97
#define PPMDED      98
#define NUM_METHODS 17 /* number of known method IDs */
/* don't forget to update list.c (list_files()) and extract.c
 * appropriately if NUM_METHODS changes */

/* (the PK-class error codes are public and have been moved into unzip.h) */

#define DF_MDY 0 /* date format 10/26/91 (USA only) */
#define DF_DMY 1 /* date format 26/10/91 (most of the world) */
#define DF_YMD 2 /* date format 91/10/26 (a few countries) */

/*---------------------------------------------------------------------------
    Extra-field block ID values and offset info.
  ---------------------------------------------------------------------------*/
/* extra-field ID values, all little-endian: */
#define EF_PKSZ64   0x0001 /* PKWARE's 64-bit filesize extensions */
#define EF_AV       0x0007 /* PKWARE's authenticity verification */
#define EF_EFS      0x0008 /* PKWARE's extended language encoding */
#define EF_OS2      0x0009 /* OS/2 extended attributes */
#define EF_PKW32    0x000a /* PKWARE's Win95/98/WinNT filetimes */
#define EF_PKVMS    0x000c /* PKWARE's VMS */
#define EF_PKUNIX   0x000d /* PKWARE's Unix */
#define EF_PKFORK   0x000e /* PKWARE's future stream/fork descriptors */
#define EF_PKPATCH  0x000f /* PKWARE's patch descriptor */
#define EF_PKPKCS7  0x0014 /* PKWARE's PKCS#7 store for X.509 Certs */
#define EF_PKFX509  0x0015 /* PKWARE's file X.509 Cert&Signature ID */
#define EF_PKCX509  0x0016 /* PKWARE's central dir X.509 Cert ID */
#define EF_PKENCRHD 0x0017 /* PKWARE's Strong Encryption header */
#define EF_PKRMCTL  0x0018 /* PKWARE's Record Management Controls*/
#define EF_PKLSTCS7 0x0019 /* PKWARE's PKCS#7 Encr. Recipient Cert List */
#define EF_PKIBM    0x0065 /* PKWARE's IBM S/390 & AS/400 attributes */
#define EF_PKIBM2   0x0066 /* PKWARE's IBM S/390 & AS/400 compr. attribs */
#define EF_IZVMS    0x4d49 /* Info-ZIP's VMS ("IM") */
#define EF_IZUNIX   0x5855 /* Info-ZIP's first Unix[1] ("UX") */
#define EF_IZUNIX2  0x7855 /* Info-ZIP's second Unix[2] ("Ux") */
#define EF_IZUNIX3  0x7875 /* Info-ZIP's newest Unix[3] ("ux") */
#define EF_TIME     0x5455 /* universal timestamp ("UT") */
#define EF_UNIPATH  0x7075 /* Info-ZIP Unicode Path ("up") */
#define EF_UNICOMNT 0x6375 /* Info-ZIP Unicode Comment ("uc") */
#define EF_MAC3     0x334d /* Info-ZIP's new Macintosh (= "M3") */
#define EF_JLMAC    0x07c8 /* Johnny Lee's old Macintosh (= 1992) */
#define EF_ZIPIT    0x2605 /* Thomas Brown's Macintosh (ZipIt) */
#define EF_ZIPIT2   0x2705 /* T. Brown's Mac (ZipIt) v 1.3.8 and newer ? */
#define EF_SMARTZIP 0x4d63 /* Mac SmartZip by Marco Bambini */
#define EF_VMCMS    0x4704 /* Info-ZIP's VM/CMS ("\004G") */
#define EF_MVS      0x470f /* Info-ZIP's MVS ("\017G") */
#define EF_ACL      0x4c41 /* (OS/2) access control list ("AL") */
#define EF_NTSD     0x4453 /* NT security descriptor ("SD") */
#define EF_ATHEOS   0x7441 /* AtheOS ("At") */
#define EF_BEOS     0x6542 /* BeOS ("Be") */
#define EF_QDOS     0xfb4a /* SMS/QDOS ("J\373") */
#define EF_AOSVS    0x5356 /* AOS/VS ("VS") */
#define EF_SPARK    0x4341 /* David Pilling's Acorn/SparkFS ("AC") */
#define EF_TANDEM   0x4154 /* Tandem NSK ("TA") */
#define EF_THEOS    0x6854 /* Jean-Michel Dubois' Theos "Th" */
#define EF_THEOSO   0x4854 /* old Theos port */
#define EF_MD5      0x4b46 /* Fred Kantor's MD5 ("FK") */
#define EF_ASIUNIX  0x756e /* ASi's Unix ("nu") */

#define EB_HEADSIZE    4 /* length of extra field block header */
#define EB_ID          0 /* offset of block ID in header */
#define EB_LEN         2 /* offset of data length field in header */
#define EB_UCSIZE_P    0 /* offset of ucsize field in compr. data */
#define EB_CMPRHEADLEN 6 /* lenght of compression header */

#define EB_UX_MINLEN   8  /* minimal "UX" field contains atime, mtime */
#define EB_UX_FULLSIZE 12 /* full "UX" field (atime, mtime, uid, gid) */
#define EB_UX_ATIME    0  /* offset of atime in "UX" extra field data */
#define EB_UX_MTIME    4  /* offset of mtime in "UX" extra field data */
#define EB_UX_UID      8  /* byte offset of UID in "UX" field data */
#define EB_UX_GID      10 /* byte offset of GID in "UX" field data */

#define EB_UX2_MINLEN 4        /* minimal "Ux" field contains UID/GID */
#define EB_UX2_UID    0        /* byte offset of UID in "Ux" field data */
#define EB_UX2_GID    2        /* byte offset of GID in "Ux" field data */
#define EB_UX2_VALID  (1 << 8) /* UID/GID present */

#define EB_UX3_MINLEN 7 /* minimal "ux" field size (2-byte UID/GID) */

#define EB_UT_MINLEN   1        /* minimal UT field contains Flags byte */
#define EB_UT_FLAGS    0        /* byte offset of Flags field */
#define EB_UT_TIME1    1        /* byte offset of 1st time value */
#define EB_UT_FL_MTIME (1 << 0) /* mtime present */
#define EB_UT_FL_ATIME (1 << 1) /* atime present */
#define EB_UT_FL_CTIME (1 << 2) /* ctime present */

#define EB_FLGS_OFFS                                                        \
    4                         /* offset of flags area in generic compressed \
                                 extra field blocks (BEOS, MAC, and others) */
#define EB_OS2_HLEN      4    /* size of OS2/ACL compressed data header */
#define EB_BEOS_HLEN     5    /* length of BeOS&AtheOS e.f attribute header */
#define EB_BE_FL_UNCMPR  0x01 /* "BeOS&AtheOS attribs uncompr." bit flag */
#define EB_MAC3_HLEN     14   /* length of Mac3 attribute block header */
#define EB_SMARTZIP_HLEN 64   /* fixed length of the SmartZip extra field */
#define EB_M3_FL_DATFRK  0x01 /* "this entry is data fork" flag */
#define EB_M3_FL_UNCMPR  0x04 /* "Mac3 attributes uncompressed" bit flag */
#define EB_M3_FL_TIME64  0x08 /* "Mac3 time fields are 64 bit wide" flag */
#define EB_M3_FL_NOUTC   0x10 /* "Mac3 timezone offset fields missing" flag */

#define EB_NTSD_C_LEN   4   /* length of central NT security data */
#define EB_NTSD_L_LEN   5   /* length of minimal local NT security data */
#define EB_NTSD_VERSION 4   /* offset of NTSD version byte */
#define EB_NTSD_MAX_VER (0) /* maximum version # we know how to handle */

#define EB_ASI_CRC32 0 /* offset of ASI Unix field's crc32 checksum */
#define EB_ASI_MODE  4 /* offset of ASI Unix permission mode field */

#define EB_IZVMS_HLEN   12 /* length of IZVMS attribute block header */
#define EB_IZVMS_FLGS   4  /* offset of compression type flag */
#define EB_IZVMS_UCSIZ  6  /* offset of ucsize field in IZVMS header */
#define EB_IZVMS_BCMASK 07 /* 3 bits for compression type */
#define EB_IZVMS_BCSTOR 0  /*  Stored */
#define EB_IZVMS_BC00   1  /*  0byte -> 0bit compression */
#define EB_IZVMS_BCDEFL 2  /*  Deflated */

/*---------------------------------------------------------------------------
    True sizes of the various headers (excluding their 4-byte signatures),
    as defined by PKWARE--so it is not likely that these will ever change.
    But if they do, make sure both these defines AND the typedefs below get
    updated accordingly.

    12/27/2006
    The Zip64 End Of Central Directory record is variable size and now
    comes in two flavors, version 1 and the new version 2 that supports
    central directory encryption.  We only use the old fields at the
    top of the Zip64 EOCDR, and this block is a fixed size still, but
    need to be aware of the stuff following.
  ---------------------------------------------------------------------------*/
#define LREC_SIZE    26 /* lengths of local file headers, central */
#define CREC_SIZE    42 /*  directory headers, end-of-central-dir */
#define ECREC_SIZE   18 /*  record, zip64 end-of-cent-dir locator */
#define ECLOC64_SIZE 16 /*  and zip64 end-of-central-dir record,  */
#define ECREC64_SIZE 52 /*  respectively                          */

#define MAX_BITS 13              /* used in unshrink() */
#define HSIZE    (1 << MAX_BITS) /* size of global work area */

#define LF    10 /* '\n' on ASCII machines; must be 10 due to EBCDIC */
#define CR    13 /* '\r' on ASCII machines; must be 13 due to EBCDIC */
#define CTRLZ 26 /* DOS & OS/2 EOF marker (used in fileio.c, vms.c) */

#ifndef ENV_UNZIP
#define ENV_UNZIP "UNZIP" /* the standard names */
#endif
#define ENV_UNZIP2 "UNZIPOPT" /* alternate names, for zip compat. */

#define QCOND (!uO.qflag) /*  comments with -vq or -vqq */

#define QCOND2 (!uO.qflag)

/**************/
/*  Typedefs  */
/**************/

#ifndef Z_UINT8_DEFINED
#ifdef __GNUC__
typedef unsigned long long z_uint8;
#else
typedef unsigned __int64 z_uint8;
#endif
#define Z_UINT8_DEFINED
#endif
#ifndef Z_UINT4_DEFINED
#if (defined(UINT_MAX) && (UINT_MAX == 0xffffffffUL))
typedef unsigned int z_uint4;
#define Z_UINT4_DEFINED
#else
#if (defined(ULONG_MAX) && (ULONG_MAX == 0xffffffffUL))
typedef unsigned long z_uint4;
#define Z_UINT4_DEFINED
#else
#if (defined(USHRT_MAX) && (USHRT_MAX == 0xffffffffUL))
typedef unsigned short z_uint4;
#define Z_UINT4_DEFINED
#endif
#endif
#endif
#endif /* !Z_UINT4_DEFINED */
#ifndef Z_UINT4_DEFINED
typedef ulg z_uint4;
#define Z_UINT4_DEFINED
#endif

/* The following three user-defined unsigned integer types are used for
   holding zipfile entities (required widths without / with Zip64 support):
   a) sizes and offset of zipfile entries
      (4 bytes / 8 bytes)
   b) enumeration and counts of zipfile entries
      (2 bytes / 8 bytes)
      Remark: internally, we use 4 bytes for archive member counting in the
              No-Zip64 case, because UnZip supports more than 64k entries for
              classic Zip archives without Zip64 extensions.
   c) enumeration and counts of zipfile volumes of multivolume archives
      (2 bytes / 4 bytes)
 */
typedef z_uint8 zusz_t; /* zipentry sizes & offsets */
typedef z_uint8 zucn_t; /* archive entry counts */
typedef z_uint4 zuvl_t; /* multivolume numbers */
#define MASK_ZUCN64 (~(zucn_t) 0)
/* In case we ever get to support an environment where z_uint8 may be WIDER
   than 64 bit wide, we will have to apply a construct similar to
     #define MASK_ZUCN64        (~(zucn_t)0 & (zucn_t)0xffffffffffffffffULL)
   for the 64-bit mask.
 */
#define MASK_ZUCN16 ((zucn_t) 0xFFFF)

typedef struct utimbuf ztimbuf;

typedef struct iztimes {
    time_t atime; /* new access time */
    time_t mtime; /* new modification time */
    time_t ctime; /* used for creation time; NOT same as st_ctime */
} iztimes;

typedef struct direntry {  /* head of system-specific struct holding */
    struct direntry *next; /*  defered directory attributes info */
    char *fn;              /* filename of directory */
    char buf[1];           /* start of system-specific internal data */
} direntry;

typedef struct slinkentry {  /* info for deferred symlink creation */
    struct slinkentry *next; /* pointer to next entry in chain */
    extent targetlen;        /* length of target filespec */
    extent attriblen;        /* length of system-specific attrib data */
    char *target;            /* pointer to target filespec */
    char *fname;             /* pointer to name of link */
    char buf[1];             /* data/name/link buffer */
} slinkentry;

typedef struct min_info {
    off_t offset;
    zusz_t compr_size;   /* compressed size (needed if extended header) */
    zusz_t uncompr_size; /* uncompressed size (needed if extended header) */
    ulg crc;             /* crc (needed if extended header) */
    zuvl_t diskstart;    /* no of volume where this entry starts */
    uch hostver;
    uch hostnum;
    unsigned file_attr;     /* local flavor, as used by creat(), chmod()... */
    unsigned encrypted : 1; /* file encrypted: decrypt before uncompressing */
    unsigned ExtLocHdr : 1; /* use time instead of CRC for decrypt check */
    unsigned textfile  : 1; /* file is text (according to zip) */
    unsigned textmode  : 1; /* file is to be extracted as text */
    unsigned lcflag    : 1; /* convert filename to lowercase */
    unsigned vollabel  : 1; /* "file" is an MS-DOS volume (disk) label */
    unsigned symlink   : 1; /* file is a symbolic link */
    unsigned HasUxAtt  : 1; /* crec ext_file_attr has Unix style mode bits */
    unsigned GPFIsUTF8 : 1; /* crec gen_purpose_flag UTF-8 bit 11 is set */
    char *cfilname;         /* central header version of filename */
} min_info;

/*---------------------------------------------------------------------------
    Zipfile work area declarations.
  ---------------------------------------------------------------------------*/

#ifdef MALLOC_WORK
union work {
    struct {            /* unshrink(): */
        shrint *Parent; /* pointer to (8192 * sizeof(shrint)) */
        uch *value;     /* pointer to 8KB char buffer */
        uch *Stack;     /* pointer to another 8KB char buffer */
    } shrink;
    uch *Slide; /* explode(), inflate(), unreduce() */
};
#else  /* !MALLOC_WORK */
union work {
    struct {                  /* unshrink(): */
        shrint Parent[HSIZE]; /* (8192 * sizeof(shrint)) == 16KB minimum */
        uch value[HSIZE];     /* 8KB */
        uch Stack[HSIZE];     /* 8KB */
    } shrink;                 /* total = 32KB minimum; 80KB on Cray/Alpha */
    uch Slide[WSIZE];         /* explode(), inflate(), unreduce() */
};
#endif /* ?MALLOC_WORK */

#define slide G.area.Slide

#define redirSlide G.area.Slide

/*---------------------------------------------------------------------------
    Zipfile layout declarations.  If these headers ever change, make sure the
    xxREC_SIZE defines (above) change with them!
  ---------------------------------------------------------------------------*/

typedef uch local_byte_hdr[LREC_SIZE];
#define L_VERSION_NEEDED_TO_EXTRACT_0 0
#define L_VERSION_NEEDED_TO_EXTRACT_1 1
#define L_GENERAL_PURPOSE_BIT_FLAG    2
#define L_COMPRESSION_METHOD          4
#define L_LAST_MOD_DOS_DATETIME       6
#define L_CRC32                       10
#define L_COMPRESSED_SIZE             14
#define L_UNCOMPRESSED_SIZE           18
#define L_FILENAME_LENGTH             22
#define L_EXTRA_FIELD_LENGTH          24

typedef uch cdir_byte_hdr[CREC_SIZE];
#define C_VERSION_MADE_BY_0            0
#define C_VERSION_MADE_BY_1            1
#define C_VERSION_NEEDED_TO_EXTRACT_0  2
#define C_VERSION_NEEDED_TO_EXTRACT_1  3
#define C_GENERAL_PURPOSE_BIT_FLAG     4
#define C_COMPRESSION_METHOD           6
#define C_LAST_MOD_DOS_DATETIME        8
#define C_CRC32                        12
#define C_COMPRESSED_SIZE              16
#define C_UNCOMPRESSED_SIZE            20
#define C_FILENAME_LENGTH              24
#define C_EXTRA_FIELD_LENGTH           26
#define C_FILE_COMMENT_LENGTH          28
#define C_DISK_NUMBER_START            30
#define C_INTERNAL_FILE_ATTRIBUTES     32
#define C_EXTERNAL_FILE_ATTRIBUTES     34
#define C_RELATIVE_OFFSET_LOCAL_HEADER 38

typedef uch ec_byte_rec[ECREC_SIZE + 4];
/*     define SIGNATURE                         0   space-holder only */
#define NUMBER_THIS_DISK               4
#define NUM_DISK_WITH_START_CEN_DIR    6
#define NUM_ENTRIES_CEN_DIR_THS_DISK   8
#define TOTAL_ENTRIES_CENTRAL_DIR      10
#define SIZE_CENTRAL_DIRECTORY         12
#define OFFSET_START_CENTRAL_DIRECTORY 16
#define ZIPFILE_COMMENT_LENGTH         20

typedef uch ec_byte_loc64[ECLOC64_SIZE + 4];
#define NUM_DISK_START_EOCDR64 4
#define OFFSET_START_EOCDR64   8
#define NUM_THIS_DISK_LOC64    16

typedef uch ec_byte_rec64[ECREC64_SIZE + 4];
#define ECREC64_LENGTH                 4
#define EC_VERSION_MADE_BY_0           12
#define EC_VERSION_NEEDED_0            14
#define NUMBER_THIS_DSK_REC64          16
#define NUM_DISK_START_CEN_DIR64       20
#define NUM_ENTRIES_CEN_DIR_THS_DISK64 24
#define TOTAL_ENTRIES_CENTRAL_DIR64    32
#define SIZE_CENTRAL_DIRECTORY64       40
#define OFFSET_START_CENTRAL_DIRECT64  48

/* The following structs are used to hold all header data of a zip entry.
   Traditionally, the structs' layouts followed the data layout of the
   corresponding zipfile header structures.  However, the zipfile header
   layouts were designed in the old ages of 16-bit CPUs, they are subject
   to structure padding and/or alignment issues on newer systems with a
   "natural word width" of more than 2 bytes.
   Please note that the structure members are now reordered by size
   (top-down), to prevent internal padding and optimize memory usage!
 */
typedef struct local_file_header { /* LOCAL */
    zusz_t csize;
    zusz_t ucsize;
    ulg last_mod_dos_datetime;
    ulg crc32;
    uch version_needed_to_extract[2];
    ush general_purpose_bit_flag;
    ush compression_method;
    ush filename_length;
    ush extra_field_length;
} local_file_hdr;

typedef struct central_directory_file_header { /* CENTRAL */
    zusz_t csize;
    zusz_t ucsize;
    zusz_t relative_offset_local_header;
    ulg last_mod_dos_datetime;
    ulg crc32;
    ulg external_file_attributes;
    zuvl_t disk_number_start;
    ush internal_file_attributes;
    uch version_made_by[2];
    uch version_needed_to_extract[2];
    ush general_purpose_bit_flag;
    ush compression_method;
    ush filename_length;
    ush extra_field_length;
    ush file_comment_length;
} cdir_file_hdr;

typedef struct end_central_dir_record { /* END CENTRAL */
    zusz_t size_central_directory;
    zusz_t offset_start_central_directory;
    zucn_t num_entries_centrl_dir_ths_disk;
    zucn_t total_entries_central_dir;
    zuvl_t number_this_disk;
    zuvl_t num_disk_start_cdir;
    int have_ecr64;       /* valid Zip64 ecdir-record exists */
    int is_zip64_archive; /* Zip64 ecdir-record is mandatory */
    ush zipfile_comment_length;
    zusz_t ec_start, ec_end;     /* offsets of start and end of the
                                    end of central directory record,
                                    including if present the Zip64
                                    end of central directory locator,
                                    which immediately precedes the
                                    end of central directory record */
    zusz_t ec64_start, ec64_end; /* if have_ecr64 is true, then these
                                    are the offsets of the start and
                                    end of the Zip64 end of central
                                    directory record */
} ecdir_rec;

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..16.  e == 31 is EOB (end of block), e == 32
   means that v is a literal, 32 < e < 64 means that v is a pointer to
   the next table, which codes (e & 31)  bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */

struct huft {
    uch e; /* number of extra bits or operation */
    uch b; /* number of bits in this code or subcode */
    union {
        ush n;          /* literal, length base, or distance base */
        struct huft *t; /* pointer to next level of table */
    } v;
};

typedef struct _APIDocStruct {
    char *compare;
    char *function;
    char *syntax;
    char *purpose;
} APIDocStruct;

/*************/
/*  Globals  */
/*************/

#include "globals.h"

/*************************/
/*  Function Prototypes  */
/*************************/

/*---------------------------------------------------------------------------
    Functions in unzip.c (initialization routines):
  ---------------------------------------------------------------------------*/

int main(int argc, char **argv);
int unzip(int argc, char **argv);
int uz_opts(int *pargc, char ***pargv);
int usage(int error);

/*---------------------------------------------------------------------------
    Functions in process.c (main driver routines):
  ---------------------------------------------------------------------------*/

int process_zipfiles(void);
void free_G_buffers(void);
/* static int    do_seekable(int lastchance); */
/* static int    find_ecrec(long searchlen); */
/* static int    process_central_comment(void); */
int process_cdir_file_hdr(void);
int process_local_file_hdr(void);
int getZip64Data(const uch *ef_buf, unsigned ef_len);
int getUnicodeData(const uch *ef_buf, unsigned ef_len);
unsigned ef_scan_for_izux(const uch *ef_buf, unsigned ef_len, int ef_is_c,
                          ulg dos_mdatetime, iztimes *z_utim, ulg *z_uidgid);

/*---------------------------------------------------------------------------
    Functions in list.c (generic zipfile-listing routines):
  ---------------------------------------------------------------------------*/

int list_files(void);
int get_time_stamp(time_t *last_modtime, ulg *nmember);
int ratio(zusz_t uc, zusz_t c);
void fnprint(void);

/*---------------------------------------------------------------------------
    Functions in fileio.c:
  ---------------------------------------------------------------------------*/

int open_input_file(void);
int open_outfile(void); /* also vms.c */
void undefer_input(void);
void defer_leftover_input(void);
unsigned readbuf(char *buf, register unsigned len);
int readbyte(void);
int fillinbuf(void);
int seek_zipf(off_t abs_offset);
int flush(uch *buf, ulg size, int unshrink);
/* static int  disk_error(void); */
void handler(int signal);
time_t dos_to_unix_time(ulg dos_datetime);
int check_for_newer(char *filename); /* os2,vmcms,vms */
int do_string(unsigned int length, int option);
ush makeword(const uch *b);
ulg makelong(const uch *sig);
zusz_t makeint64(const uch *sig);
char *format_off_t(off_t val, const char *pre, const char *post);
#if (!defined(STR_TO_ISO) || defined(NEED_STR2ISO))
char *str2iso(char *dst, const char *src);
#endif
#if (!defined(STR_TO_OEM) || defined(NEED_STR2OEM))
char *str2oem(char *dst, const char *src);
#endif
#ifdef NEED_UZMBCLEN
extent uzmbclen(const unsigned char *ptr);
#endif
#ifdef NEED_UZMBSCHR
unsigned char *uzmbschr(const unsigned char *str, unsigned int c);
#endif
#ifdef NEED_UZMBSRCHR
unsigned char *uzmbsrchr(const unsigned char *str, unsigned int c);
#endif

/*---------------------------------------------------------------------------
    Functions in extract.c:
  ---------------------------------------------------------------------------*/

int extract_or_test_files(void);
unsigned find_compr_idx(unsigned compr_methodnum);
int memextract(uch *tgt, ulg tgtsize, const uch *src, ulg srcsize);
int memflush(const uch *rawbuf, ulg size);
char *fnfilter(const char *raw, uch *space, extent size);

/*---------------------------------------------------------------------------
    Decompression functions:
  ---------------------------------------------------------------------------*/

int explode(void);             /* explode.c */
int huft_free(struct huft *t); /* inflate.c */
int huft_build(const unsigned *b, unsigned n, unsigned s, const ush *d,
               const uch *e, struct huft **t, unsigned *m);
int inflate(int is_defl64);            /* inflate.c */
int inflate_free(void);                /* inflate.c */
int unshrink(void);                    /* unshrink.c */
int UZbunzip2(void);                   /* extract.c */
void bz_internal_error(int bzerrcode); /* ubz2err.c */

/*---------------------------------------------------------------------------
    Miscellaneous/shared functions:
  ---------------------------------------------------------------------------*/

int envargs(int *Pargc, char ***Pargv, const char *envstr, const char *envstr2);
/* envargs.c */
void mksargs(int *argcp, char ***argvp); /* envargs.c */

int match(const char *s, const char *p, int ic); /* match.c */
int iswild(const char *p);                       /* match.c */

int dateformat(void);                              /* local */
char dateseparator(void);                          /* local */
int mapattr(void);                                 /* local */
int mapname(int renamed);                          /* local */
int checkdir(char *pathcomp, int flag);            /* local */
char *do_wild(const char *wildzipfn);              /* local */
char *GetLoadPath(void);                           /* local */
void close_outfile(void);                          /* local */
int set_symlnk_attribs(slinkentry *slnk_entry);    /* local */
int defer_dir_attribs(direntry **pd);              /* local */
int set_direc_attribs(direntry *d);                /* local */
int stamp_file(const char *fname, time_t modtime); /* local */

/************/
/*  Macros  */
/************/

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef DEBUG
#define Trace(x) fprintf x
#else
#define Trace(x)
#endif

#ifdef DEBUG_TIME
#define TTrace(x) fprintf x
#else
#define TTrace(x)
#endif

#ifdef NO_DEBUG_IN_MACROS
#define MTrace(x)
#else
#define MTrace(x) Trace(x)
#endif

#define ToLower(x) ((char) (isupper((int) x) ? tolower((int) x) : x))

/* The return value of the Info() "macro function" is never checked in
 * UnZip. Otherwise, to get the same behaviour as for (*G.message)(), the
 * Info() definition for "FUNZIP" would have to be corrected:
 * #define Info(buf,flag,sprf_arg) \
 *      (fputs((char *)(sprintf sprf_arg, (buf)), \
 *             (flag)&1? stderr : stdout) < 0)
 */
#ifndef Info /* may already have been defined for redirection */
#define Info(buf, flag, sprf_arg) \
    (*G.message)((uch *) (buf), (ulg) sprintf sprf_arg, (flag))
#endif /* !Info */

/*  The following macro wrappers around the fnfilter function are used many
 *  times to prepare archive entry names or name components for displaying
 *  listings and (warning/error) messages. They use sections in the upper half
 *  of 'slide' as buffer, since their output is normally fed through the
 *  Info() macro with 'slide' (the start of this area) as message buffer.
 */
#define FnFilter1(fname) \
    fnfilter((fname), slide + (extent) (WSIZE >> 1), (extent) (WSIZE >> 2))
#define FnFilter2(fname)                                              \
    fnfilter((fname), slide + (extent) ((WSIZE >> 1) + (WSIZE >> 2)), \
             (extent) (WSIZE >> 2))

#define MESSAGE(str, len, flag) (*G.message)((str), (len), (flag))

#define CRCVAL_INITIAL 0L

/* This macro defines the Zip "made by" hosts that are considered
   to support storing symbolic link entries. */
#define SYMLINK_HOST(hn)                                                    \
    ((hn) == UNIX_ || (hn) == ATARI_ || (hn) == ATHEOS_ || (hn) == BEOS_ || \
     (hn) == VMS_)

#define SKIP_(length)                                         \
    if (length && ((error = do_string(length, SKIP)) != 0)) { \
        error_in_archive = error;                             \
        if (error > 1)                                        \
            return error;                                     \
    }

#define FLUSH(w)                                    \
    ((G.mem_mode) ? memflush(redirSlide, (ulg) (w)) \
                  : flush(redirSlide, (ulg) (w), 0))
#define NEXTBYTE (G.incnt-- > 0 ? (int) (*G.inptr++) : readbyte())

#define READBITS(nbits, zdest)                                        \
    {                                                                 \
        if (nbits > G.bits_left) {                                    \
            int temp;                                                 \
            G.zipeof = 1;                                             \
            while (G.bits_left <= 8 * (int) (sizeof(G.bitbuf) - 1) && \
                   (temp = NEXTBYTE) != EOF) {                        \
                G.bitbuf |= (ulg) temp << G.bits_left;                \
                G.bits_left += 8;                                     \
                G.zipeof = 0;                                         \
            }                                                         \
        }                                                             \
        zdest = (shrint) ((unsigned) G.bitbuf & mask_bits[nbits]);    \
        G.bitbuf >>= nbits;                                           \
        G.bits_left -= nbits;                                         \
    }

/*
 * macro READBITS(nbits,zdest)    * only used by unreduce and unshrink *
 *  {
 *      if (nbits > G.bits_left) {  * fill G.bitbuf, 8*sizeof(ulg) bits *
 *          int temp;
 *
 *          G.zipeof = 1;
 *          while (G.bits_left <= 8*(int)(sizeof(G.bitbuf)-1) &&
 *                 (temp = NEXTBYTE) != EOF) {
 *              G.bitbuf |= (ulg)temp << G.bits_left;
 *              G.bits_left += 8;
 *              G.zipeof = 0;
 *          }
 *      }
 *      zdest = (shrint)((unsigned)G.bitbuf & mask_bits[nbits]);
 *      G.bitbuf >>= nbits;
 *      G.bits_left -= nbits;
 *  }
 *
 */

/* GRR:  should use StringLower for STRLOWER macro if possible */

/*
 *  Copy the zero-terminated string in str1 into str2, converting any
 *  uppercase letters to lowercase as we go.  str2 gets zero-terminated
 *  as well, of course.  str1 and str2 may be the same character array.
 */
#ifdef _MBCS
#define STRLOWER(str1, str2)                                                 \
    {                                                                        \
        char *p, *q, c;                                                      \
        unsigned i;                                                          \
        p = (char *) (str1);                                                 \
        q = (char *) (str2);                                                 \
        while ((c = *p) != '\0') {                                           \
            if ((i = CLEN(p)) > 1) {                                         \
                while (i--)                                                  \
                    *q++ = *p++;                                             \
            } else {                                                         \
                *q++ = (char) (isupper((int) (c)) ? tolower((int) (c)) : c); \
                p++;                                                         \
            }                                                                \
        }                                                                    \
        *q = '\0';                                                           \
    }
#else
#define STRLOWER(str1, str2)                                                \
    {                                                                       \
        char *p, *q;                                                        \
        p = (char *) (str1) -1;                                             \
        q = (char *) (str2);                                                \
        while (*++p)                                                        \
            *q++ = (char) (isupper((int) (*p)) ? tolower((int) (*p)) : *p); \
        *q = '\0';                                                          \
    }
#endif
/*
 *  NOTES:  This macro makes no assumptions about the characteristics of
 *    the tolower() function or macro (beyond its existence), nor does it
 *    make assumptions about the structure of the character set (i.e., it
 *    should work on EBCDIC machines, too).  The fact that either or both
 *    of isupper() and tolower() may be macros has been taken into account;
 *    watch out for "side effects" (in the C sense) when modifying this
 *    macro.
 */

#define A_TO_N(str1)

/* default setup for internal codepage: assume ISO 8859-1 compatibility!! */
#if (!defined(NATIVE) && !defined(CRTL_CP_IS_ISO) && !defined(CRTL_CP_IS_OEM))
#define CRTL_CP_IS_ISO
#endif

/*  Translate "extended ASCII" chars (OEM coding for DOS and OS/2; else
 *  ISO-8859-1 [ISO Latin 1, Win Ansi,...]) into the internal "native"
 *  code page.  As with A_TO_N(), conversion is done in place.
 */
#ifndef _ISO_INTERN
#ifdef CRTL_CP_IS_OEM
#ifndef IZ_ISO2OEM_ARRAY
#define IZ_ISO2OEM_ARRAY
#endif
#define _ISO_INTERN(str1)                               \
    if (iso2oem) {                                      \
        register uch *p;                                \
        for (p = (uch *) (str1); *p; p++)               \
            *p = (*p & 0x80) ? iso2oem[*p & 0x7f] : *p; \
    }
#else
#define _ISO_INTERN(str1) A_TO_N(str1)
#endif
#endif

#ifndef _OEM_INTERN
#ifdef CRTL_CP_IS_OEM
#define _OEM_INTERN(str1) A_TO_N(str1)
#else
#ifndef IZ_OEM2ISO_ARRAY
#define IZ_OEM2ISO_ARRAY
#endif
#define _OEM_INTERN(str1)                               \
    if (oem2iso) {                                      \
        register uch *p;                                \
        for (p = (uch *) (str1); *p; p++)               \
            *p = (*p & 0x80) ? oem2iso[*p & 0x7f] : *p; \
    }
#endif
#endif

#ifndef STR_TO_ISO
#ifdef CRTL_CP_IS_ISO
#define STR_TO_ISO strcpy
#else
#define STR_TO_ISO str2iso
#define NEED_STR2ISO
#endif
#endif

#ifndef STR_TO_OEM
#ifdef CRTL_CP_IS_OEM
#define STR_TO_OEM strcpy
#else
#define STR_TO_OEM str2oem
#define NEED_STR2OEM
#endif
#endif

#if (!defined(INTERN_TO_ISO) && !defined(ASCII2ISO))
#ifdef CRTL_CP_IS_OEM
/* know: "ASCII" is "OEM" */
#define ASCII2ISO(c) ((((c) &0x80) && oem2iso) ? oem2iso[(c) &0x7f] : (c))
#if (defined(NEED_STR2ISO) && !defined(CRYP_USES_OEM2ISO))
#define CRYP_USES_OEM2ISO
#endif
#else
/* assume: "ASCII" is "ISO-ANSI" */
#define ASCII2ISO(c) (c)
#endif
#endif

#if (!defined(INTERN_TO_OEM) && !defined(ASCII2OEM))
#ifdef CRTL_CP_IS_OEM
/* know: "ASCII" is "OEM" */
#define ASCII2OEM(c) (c)
#else
/* assume: "ASCII" is "ISO-ANSI" */
#define ASCII2OEM(c) ((((c) &0x80) && iso2oem) ? iso2oem[(c) &0x7f] : (c))
#if (defined(NEED_STR2OEM) && !defined(CRYP_USES_ISO2OEM))
#define CRYP_USES_ISO2OEM
#endif
#endif
#endif

/* codepage conversion setup for testp() in crypt.c */
#ifdef CRTL_CP_IS_ISO
#ifndef STR_TO_CP2
#define STR_TO_CP2 STR_TO_OEM
#endif
#else
#ifdef CRTL_CP_IS_OEM
#ifndef STR_TO_CP2
#define STR_TO_CP2 STR_TO_ISO
#endif
#else /* native internal CP is neither ISO nor OEM */
#ifndef STR_TO_CP1
#define STR_TO_CP1 STR_TO_ISO
#endif
#ifndef STR_TO_CP2
#define STR_TO_CP2 STR_TO_OEM
#endif
#endif
#endif

/* Convert filename (and file comment string) into "internal" charset.
 * This macro assumes that Zip entry filenames are coded in OEM (IBM DOS)
 * codepage when made on
 *  -> DOS (this includes 16-bit Windows 3.1)  (FS_FAT_)
 *  -> OS/2                                    (FS_HPFS_)
 *  -> Win95/WinNT with Nico Mak's WinZip      (FS_NTFS_ && hostver == "5.0")
 * EXCEPTIONS:
 *  PKZIP for Windows 2.5, 2.6, and 4.0 flag their entries as "FS_FAT_", but
 *  the filename stored in the local header is coded in Windows ANSI (CP 1252
 *  resp. ISO 8859-1 on US and western Europe locale settings).
 *  Likewise, PKZIP for UNIX 2.51 flags its entries as "FS_FAT_", but the
 *  filenames stored in BOTH the local and the central header are coded
 *  in the local system's codepage (usually ANSI codings like ISO 8859-1).
 *
 * All other ports are assumed to code zip entry filenames in ISO 8859-1.
 */
#ifndef Ext_ASCII_TO_Native
#define Ext_ASCII_TO_Native(string, hostnum, hostver, isuxatt, islochdr)       \
    if (((hostnum) == FS_FAT_ &&                                               \
         !(((islochdr) || (isuxatt)) &&                                        \
           ((hostver) == 25 || (hostver) == 26 || (hostver) == 40))) ||        \
        (hostnum) == FS_HPFS_ || ((hostnum) == FS_NTFS_ && (hostver) == 50)) { \
        _OEM_INTERN((string));                                                 \
    } else {                                                                   \
        _ISO_INTERN((string));                                                 \
    }
#endif

/**********************/
/*  Global constants  */
/**********************/

extern const unsigned mask_bits[17];
extern const char *fnames[2];

#ifdef IZ_ISO2OEM_ARRAY
extern const uch *iso2oem;
extern const uch iso2oem_850[];
#endif
#ifdef IZ_OEM2ISO_ARRAY
extern const uch *oem2iso;
extern const uch oem2iso_850[];
#endif

extern const char CentSigMsg[];
extern const char EndSigMsg[];
extern const char SeekMsg[];
extern const char ReportMsg[];
extern const char CompiledWith[];

/***********************************/
/*  Global (shared?) RTL variables */
/***********************************/

/* Default character when a zwchar too big for wchar_t */
#define zwchar_to_wchar_t_default_char '_'

/* Default character string when wchar_t does not convert to mb */
#define wide_to_mb_default_string "_"

/* wide character type */
typedef unsigned long zwchar;

/* UTF-8 related conversion functions, currently found in process.c */

/* convert UTF-8 string to multi-byte string */
char *utf8_to_local_string(const char *utf8_string, int escape_all);

/* convert UTF-8 string to wide string */
zwchar *utf8_to_wide_string(const char *utf8_string);

/* convert wide string to multi-byte string */
char *wide_to_local_string(const zwchar *wide_string, int escape_all);

/* convert wide character to escape string */
char *wide_to_escape_string(unsigned long);

#endif /* !__unzpriv_h */
