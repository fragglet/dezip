/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/

#ifndef __globals_h
#define __globals_h

#include "bzlib.h"

struct globals {

    /* command options of general use */
    UzpOpts UzO; /* command options of general use */

    /* command options specific to the high level command line interface */

    /* internal flags and general globals */
    int noargs;          /* did true command line have *any* arguments? */
    unsigned filespecs;  /* number of real file specifications to be matched */
    unsigned xfilespecs; /* number of excluded filespecs to be matched */
    int process_all_files;
    int overwrite_mode; /* 0 - query, 1 - always, 2 - never */
    int create_dirs;    /* used by main(), mapname(), checkdir() */
    int extract_flag;
    int newzip; /* reset in extract.c; used in crypt.c */
    off_t real_ecrec_offset;
    off_t expect_ecrec_offset;
    off_t csize;      /* used by decompr. (NEXTBYTE): must be signed */
    off_t used_csize; /* used by extract_or_test_member(), explode() */

    char **pfnames;
    char **pxnames;
    char sig[4];
    char answerbuf[10];
    min_info info[DIR_BLKSIZ];
    min_info *pInfo;
    union work area; /* see unzpriv.h for definition of work */

    const uint32_t *crc_32_tab;
    uint32_t crc32val; /* CRC shift reg. (was static in funzip) */

    uint8_t *inbuf; /* input buffer (any size is OK) */
    uint8_t *inptr; /* pointer into input buffer */
    int incnt;

    uint32_t bitbuf;
    int bits_left; /* unreduce and unshrink only */
    int zipeof;
    char *wildzipfn;
    char *zipfn; /* GRR:  WINDLL:  must nuke any malloc'd zipfn... */
    int zipfd;   /* zipfile file handle */
    off_t ziplen;
    off_t cur_zipfile_bufstart; /* extract_or_test, readbuf, ReadByte */
    off_t extra_bytes;          /* used in unzip.c, misc.c */
    uint8_t *extra_field;       /* Unix, VMS, Mac, OS/2, Acorn, ... */
    uint8_t *hold;

    local_file_hdr lrec; /* used in unzip.c, extract.c */
    cdir_file_hdr crec;  /* used in unzip.c, extract.c, misc.c */
    ecdir_rec ecrec;     /* used in unzip.c, extract.c */
    struct stat statbuf; /* used by main, mapname, check_for_newer */

    int zip64; /* true if Zip64 info in extra field */

    int mem_mode;
    uint8_t *outbufptr;     /* extract.c static */
    uint32_t outsize;       /* extract.c static */
    int reported_backslash; /* extract.c static */
    int disk_full;
    int newfile;
    void **cover; /* used in extract.c for bomb detection */

    int didCRlast;     /* fileio static */
    uint32_t numlines; /* fileio static: number of lines printed */
    int sol;           /* fileio static: at start of line */
    int no_ecrec;      /* process static */
    int symlnk;
    slinkentry *slink_head; /* pointer to head of symlinks list */
    slinkentry *slink_last; /* pointer to last entry in symlinks list */

    FILE *outfile;
    uint8_t *outbuf;

    uint8_t *outbuf2; /*  process_zipfiles() (never changes); */
    uint8_t *outptr;
    uint32_t outcnt;          /* number of chars stored in outbuf */
    char filename[FILNAMSIZ]; /* also used by NT for temporary SFX path */
    char *filename_full;      /* the full path so Unicode checks work */
    size_t fnfull_bufsize;    /* size of allocated filename buffer */
    int unicode_escape_all;
    int unicode_mismatch;
    int native_is_utf8; /* bool, TRUE => native charset == UTF-8 */

    int unipath_version;       /* version of Unicode field */
    uint32_t unipath_checksum; /* Unicode field checksum */
    char *unipath_filename;    /* UTF-8 path */

    char *key;        /* crypt static: decryption password or NULL */
    int nopwd;        /* crypt static */
    uint32_t keys[3]; /* crypt static: keys defining pseudo-random sequence */

    int echofd; /* ttyio static: file descriptor whose echo is off */

    unsigned hufts; /* track memory usage */

    struct huft *fixed_tl;           /* inflate static */
    struct huft *fixed_td;           /* inflate static */
    unsigned fixed_bl, fixed_bd;     /* inflate static */
    struct huft *fixed_tl64;         /* inflate static */
    struct huft *fixed_td64;         /* inflate static */
    unsigned fixed_bl64, fixed_bd64; /* inflate static */
    struct huft *fixed_tl32;         /* inflate static */
    struct huft *fixed_td32;         /* inflate static */
    unsigned fixed_bl32, fixed_bd32; /* inflate static */
    const uint16_t *cplens;          /* inflate static */
    const uint8_t *cplext;           /* inflate static */
    const uint8_t *cpdext;           /* inflate static */
    unsigned wp; /* inflate static: current position in slide */
    uint32_t bb; /* inflate static: bit buffer */
    unsigned bk; /* inflate static: bits count in bit buffer */

    /* cylindric buffer space for formatting off_t values (fileio static) */
    char fofft_buf[OFF_T_NUM][OFF_T_LEN];
    int fofft_index;

    MsgFn *message;
    PasswdFn *decr_passwd;

    int incnt_leftover; /* so improved NEXTBYTE does not waste input */
    uint8_t *inptr_leftover;

    int created_dir, renamed_fullpath;
    char *rootpath, *buildpath, *end;
    const char *wildname;
    char *dirname, matchname[FILNAMSIZ];
    int rootlen, have_dirname, dirnamelen, notfirstcall;
    void *wild_dir;
};

/***************************************************************************/

#define CRC_32_TAB G.crc_32_tab

/* pseudo constant sigs; they are initialized at runtime so unzip executable
 * won't look like a zipfile */
extern char local_hdr_sig[4];
extern char central_hdr_sig[4];
extern char end_central_sig[4];
extern char end_central32_sig[4];
extern char end_central64_sig[4];
extern char end_centloc64_sig[4];

extern struct globals G;

#endif /* __globals_h */
