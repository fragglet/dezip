/*
 * unzip.c was getting TOO big, so I split out a bunch of likely
 * defines and constants into a separate file.
 * David Kirschbaum
 * Toad Hall
 */

#define VERSION  "UnZip:  Zipfile Extract v3.10 (C) of 08-16-90;  (C) 1989 Samuel H. Smith"

/* #define NOSKIP 1 */	/* v3.04 Enable if you do NOT want
			 * skip_to_signature() enabled */

typedef unsigned char byte; /* code assumes UNSIGNED bytes */
typedef long longint;
typedef unsigned short UWORD;
typedef char boolean;

/* v2.0g Allan Bjorklund added this.  Confirm it doesn't make your system
 * choke and die!  (Could be it belongs down in the !__STDC__ section
 * along with *malloc()
 */
#ifdef MTS		/* v2.0h No one else seems to want it tho... */
#include <sys/file.h>   /* v2.0g Chitra says MTS needs this for O_BINARY */
#endif

#define STRSIZ 256

#include <stdio.h>
 /* this is your standard header for all C compiles */
#include <ctype.h>

#ifdef __TURBOC__           /* v2.0b */
#include <timeb.h>      /* for structure ftime */
#include <io.h>         /* for setftime(), dup(), creat() */
#include <mem.h>        /* for memcpy() */
#include <stat.h>       /* for S_IWRITE, S_IREAD */
#endif

#ifdef __STDC__

#include <stdlib.h>
 /* this include defines various standard library prototypes */

#else

char *malloc();         /* who knows WHERE this is prototyped... */

#endif

#define min(a,b) ((a) < (b) ? (a) : (b))

#ifndef ZMEM                            /* v2.0f use your system's stuff */
#define zmemcpy memcpy
#define zmemset memset
#endif

/* ----------------------------------------------------------- */
/*
 * Zipfile layout declarations
 *
 */

/* Macros for accessing the longint header fields.  These fields
   are defined as array of char to prevent a 32-bit compiler from
   padding the struct so that longints start on a 4-byte boundary.
   This will not work on a machine that can access longints only
   if they start on a 4-byte boundary.
*/

#ifndef NOTINT16    /* v2.0c */
#define LONGIP(l) ((longint *) &((l)[0]))
#define LONGI(l) (*(LONGIP(l)))
#else       /* Have to define, since used for HIGH_LOW */
#define LONGIP(I) &I
#define LONGI(I) I
#endif

typedef longint signature_type;

#define LOCAL_FILE_HEADER_SIGNATURE  0x04034b50L

#ifndef NOTINT16            /* v2.0c */
typedef struct local_file_header {
    UWORD version_needed_to_extract;
    UWORD general_purpose_bit_flag;
    UWORD compression_method;
    UWORD last_mod_file_time;
    UWORD last_mod_file_date;
    byte crc32[4];
    byte compressed_size[4];
    byte uncompressed_size[4];
    UWORD filename_length;
    UWORD extra_field_length;
} local_file_header;

#else   /* NOTINT16 */
typedef struct local_file_header {
    UWORD version_needed_to_extract;
    UWORD general_purpose_bit_flag;
    UWORD compression_method;
    UWORD last_mod_file_time;
    UWORD last_mod_file_date;
    longint crc32;              /* v2.0e */
    longint compressed_size;
    longint uncompressed_size;  /* v2.0e */
    UWORD filename_length;
    UWORD extra_field_length;
} local_file_header;

typedef struct local_byte_header {
    byte version_needed_to_extract[2];
    byte general_purpose_bit_flag[2];
    byte compression_method[2];
    byte last_mod_file_time[2];
    byte last_mod_file_date[2];
    byte crc32[4];
    byte compressed_size[4];
    byte uncompressed_size[4];
    byte filename_length[2];
    byte extra_field_length[2];
} local_byte_header;
#endif

#define CENTRAL_FILE_HEADER_SIGNATURE  0x02014b50L

#ifndef NOTINT16            /* v2.0c */
typedef struct central_directory_file_header {
    UWORD version_made_by;
    UWORD version_needed_to_extract;
    UWORD general_purpose_bit_flag;
    UWORD compression_method;
    UWORD last_mod_file_time;
    UWORD last_mod_file_date;
    byte crc32[4];
    byte compressed_size[4];
    byte uncompressed_size[4];
    UWORD filename_length;
    UWORD extra_field_length;
    UWORD file_comment_length;
    UWORD disk_number_start;
    UWORD internal_file_attributes;
    byte external_file_attributes[4];
    byte relative_offset_local_header[4];
} central_directory_file_header;

#else   /* NOTINT16 */
typedef struct central_directory_file_header {
    UWORD version_made_by;
    UWORD version_needed_to_extract;
    UWORD general_purpose_bit_flag;
    UWORD compression_method;
    UWORD last_mod_file_time;
    UWORD last_mod_file_date;
    longint crc32;                  /* v2.0e */
    longint compressed_size;        /* v2.0e */
    longint uncompressed_size;      /* v2.0e */
    UWORD filename_length;
    UWORD extra_field_length;
    UWORD file_comment_length;
    UWORD disk_number_start;
    UWORD internal_file_attributes;
    longint external_file_attributes;       /* v2.0e */
    longint relative_offset_local_header;   /* v2.0e */
} central_directory_file_header;

typedef struct central_directory_byte_header {
    byte version_made_by[2];
    byte version_needed_to_extract[2];
    byte general_purpose_bit_flag[2];
    byte compression_method[2];
    byte last_mod_file_time[2];
    byte last_mod_file_date[2];
    byte crc32[4];
    byte compressed_size[4];
    byte uncompressed_size[4];
    byte filename_length[2];
    byte extra_field_length[2];
    byte file_comment_length[2];
    byte disk_number_start[2];
    byte internal_file_attributes[2];
    byte external_file_attributes[4];
    byte relative_offset_local_header[4];
} central_directory_byte_header;
#endif

#define END_CENTRAL_DIR_SIGNATURE  0x06054b50L

#ifndef NOTINT16            /* v2.0c */
typedef struct end_central_dir_record {
    UWORD number_this_disk;
    UWORD number_disk_with_start_central_directory;
    UWORD total_entries_central_dir_on_this_disk;
    UWORD total_entries_central_dir;
    byte size_central_directory[4];
    byte offset_start_central_directory[4];
    UWORD zipfile_comment_length;
} end_central_dir_record;

#else   /* NOTINT16 */
typedef struct end_central_dir_record {
    UWORD number_this_disk;
    UWORD number_disk_with_start_central_directory;
    UWORD total_entries_central_dir_on_this_disk;
    UWORD total_entries_central_dir;
    longint size_central_directory;         /* v2.0e */
    longint offset_start_central_directory; /* v2.0e */
    UWORD zipfile_comment_length;
} end_central_dir_record;

typedef struct end_central_byte_record {
    byte number_this_disk[2];
    byte number_disk_with_start_central_directory[2];
    byte total_entries_central_dir_on_this_disk[2];
    byte total_entries_central_dir[2];
    byte size_central_directory[4];
    byte offset_start_central_directory[4];
    byte zipfile_comment_length[2];
} end_central_byte_record;
#endif  /* NOTINT16 */

#define DLE 144

#define max_bits 13
#define init_bits 9
#define hsize 8192
#define first_ent 257
#define clear 256

/* ============================================================= */
/*
 * Host operating system details
 *
 */

#ifdef UNIX

/* On some systems the contents of sys/param.h duplicates the
   contents of sys/types.h, so you don't need (and can't use)
   sys/types.h. */

#include <sys/types.h>
#include <sys/param.h>

#define ZSUFX ".zip"
#ifndef BSIZE
#define BSIZE DEV_BSIZE     /* v2.0c assume common for all Unix systems */
#endif

#ifndef BSD                 /* v2.0b */
#include <time.h>
struct tm *gmtime(), *localtime();
#else   /* BSD */
#include <sys/time.h>
#endif

#else   /* !UNIX */

#define BSIZE 512   /* disk block size */
#define ZSUFX ".ZIP"

#endif

#if defined(V7) || defined(BSD)

#define strchr index
#define strrchr rindex

#endif

/*  v3.03 Everybody seems to need this.
 * this include file defines
 *             #define S_IREAD 0x0100  (* owner may read *)
 *             #define S_IWRITE 0x0080 (* owner may write *)
 * as used in the creat() standard function
 */
#ifndef __TURBOC__	/* it's already included */
#include <sys/stat.h>	/* for S_IWRITE, S_IREAD  v3.03 */
#endif

#ifdef __STDC__

#include <string.h>
 /* this include defines strcpy, strcmp, etc. */

#else

char *strchr(), *strrchr();

#endif

long lseek();

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

#ifdef V7

#define O_RDONLY  0
#define O_WRONLY  1
#define O_RDWR    2

#else   /* !V7 */

#include <fcntl.h>
 /*
  * this include file defines
  *             #define O_BINARY 0x8000  (* no cr-lf translation *)
  * as used in the open() standard function
  */

#endif  /* V7 */

#ifdef __TURBOC__
/* v2.0b Local Prototypes */
    /* In CRC32.C */
extern void UpdateCRC(register unsigned char *s, register int len);
    /* In MATCH.C */
extern int match(char *string, char *pattern);
    /* v2.0j in mapname.c */
extern int mapped_name(void);

#ifdef NOTINT16         /* v2.0c */
/* The next two are only prototyped here for debug testing on my PC
 * with Turbo C.
 */

UWORD makeword(byte *b);
longint makelong(byte *sig);    /* v2.0e */
#endif  /* NOTINT16 */

int ReadByte(UWORD *x);
int FillBitBuffer(register int bits);
void LoadFollowers(void);
void FlushOutput(void);
void partial_clear(void);
int create_output_file(void);
void unShrink(void);
void unReduce(void);
void unImplode(void);
void set_file_time(void);
int readbuf(int fd, char *buf, register unsigned size);
void get_string(int len, char *s);
void dir_member(void);
void extract_member(void);
void skip_member(void);
void process_local_file_header(char **fnamev);
void process_central_file_header(void);
void process_end_central_dir(void);
int open_input_file(void);
void process_headers(void);
void usage(void);
void process_zipfile(void);
#endif  /* __TURBOC__ */

/* ------------------------------------------------------------- */

/* v3.05 Deleting  crc32.h and incorporating its two lines right here */
/* #include "crc32.h" */

unsigned long crc32val;
#ifndef __TURBOC__	/* it's already been prototyped above */
void UpdateCRC();
#endif

#define LF 10		/* '\n' on ascii machines.  Must be 10 due to EBCDIC */
#define CR 13		/* '\r' on ascii machines.  Must be 13 due to EBCDIC */

#ifdef EBCDIC
extern unsigned char ebcdic [];
#define ascii_to_native(c) ebcdic[(c)]
#else
#define ascii_to_native(c) (c)
#endif

#define OUTB(intc) { *outptr++=intc; if (++outcnt==OUTBUFSIZ) FlushOutput(); }

/*
 *  macro OUTB(intc)
 *  {
 *      *outptr++=intc;
 *      if (++outcnt==OUTBUFSIZ)
 *          FlushOutput();
 *  }
 *
 */

#define READBIT(nbits,zdest) { if (nbits <= bits_left) { zdest = (int)(bitbuf & mask_bits[nbits]); bitbuf >>= nbits; bits_left -= nbits; } else zdest = FillBitBuffer(nbits);}

/*
 * macro READBIT(nbits,zdest)
 *  {
 *      if (nbits <= bits_left) {
 *          zdest = (int)(bitbuf & mask_bits[nbits]);
 *          bitbuf >>= nbits;
 *          bits_left -= nbits;
 *      } else
 *          zdest = FillBitBuffer(nbits);
 *  }
 *
 */
