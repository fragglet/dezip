/*
 * Copyright 1989 Samuel H. Smith;  All rights reserved
 *
 * Do not distribute modified versions without my permission.
 * Do not remove or alter this notice or any other copyright notice.
 * If you use this in your own program you must distribute source code.
 * Do not use any of this in a commercial product.
 *
 */

/*
 * UnZip - A simple zipfile extract utility
 *
 * Compile-time definitions:
 * See the Makefile for details, explanations, and all the current
 * system makerules.
 *
 * If you have to add a new one for YOUR system, please forward the
 * new Makefile to kirsch@usasoc.soc.mil for distribution.
 * Be SURE to give FULL details on your system (hardware, OS, versions,
 * processor, whatever) that made it unique.
 *
 * REVISION HISTORY : See history.v30 (or whatever current version is)
 *
 */

#define VERSION  "UnZip:  Zipfile Extract v3.0 (C) of 05-03-90;  (C) 1989 Samuel H. Smith"

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
#include <stat.h>       /* for S_IWRITE, S_IREAD */
#include <mem.h>        /* for memcpy() */
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
/*
 * SEE HOST OPERATING SYSTEM SPECIFICS SECTION STARTING NEAR LINE 180
 *
 */


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

char *fnames[2] = { /* default filenames vector */
    "*",
    NULL
};
char **fnv = &fnames[0];

int tflag;      /* -t: test */
int vflag;      /* -v: view directory */
int cflag;      /* -c: output to stdout (JC) */
int aflag;      /* -a: do ascii to ebcdic translation 2.0f */
                /*     OR <cr><nl> to <sp><nl> conversion  */
int mflag;	/* -m: map member filenames to lower case v2.0j */

int members;
longint csize;
longint ucsize;
longint tot_csize;
longint tot_ucsize;


/* ----------------------------------------------------------- */
/*
 * input file variables
 *
 */

#define INBUFSIZ BUFSIZ     /* same as stdio uses */
byte *inbuf;            /* input file buffer - any size is legal */
byte *inptr;

int incnt;
UWORD bitbuf;
int bits_left;
boolean zipeof;

int zipfd;
char zipfn[STRSIZ];
local_file_header lrec;

/* ----------------------------------------------------------- */
/*
 * output stream variables
 *
 */

#define OUTBUFSIZ 0x2000        /* unImplode needs power of 2, >= 0x2000 */
byte *outbuf;                   /* buffer for rle look-back */
byte *outptr;
byte *outout;                 /* Scratch pad for ascebc trans v2.0g */

longint outpos;         /* absolute position in outfile */
int outcnt;             /* current position in outbuf */

int outfd;
char filename[STRSIZ];
char extra[STRSIZ];
char comment[STRSIZ];       /* v2.0b made it global for displays */

#define DLE 144


/* ----------------------------------------------------------- */
/*
 * shrink/reduce working storage
 *
 */

int factor;
/* really need only 256, but prefix_of, which shares the same
   storage, is just over 16K */
byte followers[257][64];    /* also lzw prefix_of, s-f lit_nodes */
byte Slen[256];

#define max_bits 13
#define init_bits 9
#define hsize 8192
#define first_ent 257
#define clear 256

typedef short hsize_array_integer[hsize+1]; /* was used for prefix_of */
typedef byte hsize_array_byte[hsize+1];

short *prefix_of = (short *) followers; /* share reduce/shrink storage */
hsize_array_byte suffix_of;     /* also s-f length_nodes */
hsize_array_byte stack;         /* also s-f distance_nodes */

int codesize;
int maxcode;
int free_ent;
int maxcodemax;
int offset;
int sizex;



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

#ifndef UNIX

#include <sys/stat.h>

/*
 * this include file defines
 *             #define S_IREAD 0x0100  (* owner may read *)
 *             #define S_IWRITE 0x0080 (* owner may write *)
 * as used in the creat() standard function
 */

#endif  /* !UNIX */

#ifdef __TURBOC__
/* v2.0b Local Prototypes */
    /* In CRC32.C */
extern void UpdateCRC(register unsigned char *s, register int len);
    /* In MATCH.C */
extern int match(char *string, char *pattern);
    /* v2.0e in ASCEBC.C */
extern void ae_buf(register unsigned char *outbuf,register int outcnt);
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


void set_file_time()
 /*
  * set the output file date/time stamp according to information from the
  * zipfile directory record for this file
  */
{
#ifndef UNIX
    union {
        struct ftime ft;        /* system file time record */
        struct {
                UWORD ztime;     /* date and time words */
                UWORD zdate;     /* .. same format as in .ZIP file */
        } zt;
    } td;

    /*
     * set output file date and time - this is optional and can be
     * deleted if your compiler does not easily support setftime()
     */

    td.zt.ztime = lrec.last_mod_file_time;
    td.zt.zdate = lrec.last_mod_file_date;

    setftime(outfd, &td.ft);

#else   /* UNIX */

    time_t times[2];
    struct tm *tmbuf;
    long m_time;
    int yr, mo, dy, hh, mm, ss, leap, days = 0;
#ifdef BSD
    struct timeval tv;
    struct timezone tz;
#endif

    /*
     * These date conversions look a little wierd, so I'll explain.
     * UNIX bases all file modification times on the number of seconds
     * elapsed since Jan 1, 1970, 00:00:00 GMT.  Therefore, to maintain
     * compatibility with MS-DOS archives, which date from Jan 1, 1980,
     * with NO relation to GMT, the following conversions must be made:
     *      the Year (yr) must be incremented by 10;
     *      the Date (dy) must be decremented by 1;
     *      and the whole mess must be adjusted by TWO factors:
     *          relationship to GMT (ie.,Pacific Time adds 8 hrs.),
     *          and whether or not it is Daylight Savings Time.
     * Also, the usual conversions must take place to account for leap years,
     * etc.
     *                                     C. Seaman
     */

    yr = (((lrec.last_mod_file_date >> 9) & 0x7f) + 10);  /* dissect date */
    mo = ((lrec.last_mod_file_date >> 5) & 0x0f);
    dy = ((lrec.last_mod_file_date & 0x1f) - 1);

    hh = ((lrec.last_mod_file_time >> 11) & 0x1f);        /* dissect time */
    mm = ((lrec.last_mod_file_time >> 5) & 0x3f);
    ss = ((lrec.last_mod_file_time & 0x1f) * 2);

    /* leap = # of leap years from 1970 up to but not including
       the current year */

    leap = ((yr+1969)/4);              /* Leap year base factor */

    /* How many days from 1970 to this year? */
    days = (yr * 365) + (leap - 492);

    switch(mo)                 /* calculate expired days this year */
    {
    case 12:
        days += 30;
    case 11:
        days += 31;
    case 10:
        days += 30;
    case 9:
        days += 31;
    case 8:
        days += 31;
    case 7:
        days += 30;
    case 6:
        days += 31;
    case 5:
        days += 30;
    case 4:
        days += 31;
    case 3:
        days += 28;                    /* account for leap years */
        if (((yr+1970) % 4 == 0) && (yr+1970) != 2000)
            ++days;
    case 2:
        days += 31;
    }

    /* convert date & time to seconds relative to 00:00:00, 01/01/1970 */
    m_time = ((days + dy) * 86400) + (hh * 3600) + (mm * 60) + ss;

#ifdef BSD
    gettimeofday(&tv, &tz);
/* This program is TOO smart about daylight savings time.
 * Adjusting for it throws our file times off by one hour if it's true.
 * Remming it out.
 *
 *  if (tz.tz_dsttime != 0)
 *      m_time -= 3600;
 */
    m_time += tz.tz_minuteswest * 60;  /* account for timezone differences */
#else   /* !BSD */
    tmbuf = localtime(&m_time);
    hh = tmbuf->tm_hour;
    tmbuf = gmtime(&m_time);
    hh = tmbuf->tm_hour - hh;
    if (hh < 0)
    hh += 24;
    m_time += (hh * 3600);             /* account for timezone differences */
#endif

    times[0] = m_time;             /* set the stamp on the file */
    times[1] = m_time;
    utime(filename, times);
#endif  /* UNIX */
}


int create_output_file()
 /* return non-0 if creat failed */
{   /* create the output file with READ and WRITE permissions */
    if (cflag) {        /* output to stdout (a copy of it, really) */
        outfd = dup(1);
        return 0;
        }
#ifndef UNIX
    outfd = creat(filename, S_IWRITE | S_IREAD);
#else
    outfd = creat(filename, 0666);  /* let umask strip unwanted perm's */
#endif

    if (outfd < 1) {
        fprintf(stderr, "Can't create output: %s\n", filename);
        return 1;
    }

    /*
     * close the newly created file and reopen it in BINARY mode to
     * disable all CR/LF translations
     */
#ifndef UNIX
    close(outfd);
    outfd = open(filename, O_RDWR | O_BINARY);
#endif
    return 0;
}


int open_input_file()
 /* return non-0 if open failed */
{
    /*
     * open the zipfile for reading and in BINARY mode to prevent cr/lf
     * translation, which would corrupt the bitstreams
     */

#ifndef UNIX
    zipfd = open(zipfn, O_RDONLY | O_BINARY);
#else
    zipfd = open(zipfn, O_RDONLY);
#endif
    if (zipfd < 1) {
        fprintf(stderr, "Can't open input file: %s\n", zipfn);
        return (1);
    }
    return 0;
}


#ifdef NOTINT16     /* v2.0c */
UWORD makeword(b)
byte * b;
 /* convert Intel style 'short' integer to non-Intel non-16-bit
  * host format
  */
{
/*
    return  ( ((UWORD) (b[1]) << 8)
            | (UWORD) (b[0])
            );
*/
    return  ( ( b[1] << 8)
            | b[0]
            );
}

longint makelong(sig)
byte *sig;
 /* convert intel style 'long' variable to non-Intel non-16-bit
  * host format
  */
{
    return ( ((longint) sig[3]) << 24)
          + ( ((longint) sig[2]) << 16)
          + ( ((longint) sig[1]) << 8)
          +   ((longint)  sig[0]) ;
}
#endif  /* NOTINT16 */

#ifdef HIGH_LOW

void swap_bytes(wordp)
UWORD *wordp;
 /* convert Intel style 'short int' variable to host format */
{
    char *charp = (char *) wordp;
    char temp;

    temp = charp[0];
    charp[0] = charp[1];
    charp[1] = temp;
}

void swap_lbytes(longp)
longint *longp;
 /* convert intel style 'long' variable to host format */
{
    char *charp = (char *) longp;
    char temp[4];

    temp[3] = charp[0];
    temp[2] = charp[1];
    temp[1] = charp[2];
    temp[0] = charp[3];

    charp[0] = temp[0];
    charp[1] = temp[1];
    charp[2] = temp[2];
    charp[3] = temp[3];
}

#endif /* HIGH_LOW */



/* ============================================================= */

int readbuf(fd, buf, size)
int fd;
char *buf;
register unsigned size;
{
    register int count;
    int n;

    n = size;
    while (size)  {
        if (incnt == 0)  {
            if ((incnt = read(fd, inbuf, INBUFSIZ)) <= 0)
                return(incnt);
            inptr = inbuf;
        }
        count = min(size, incnt);
        zmemcpy(buf, inptr, count);
        buf += count;
        inptr += count;
        incnt -= count;
        size -= count;
    }
    return(n);
}

int ReadByte(x)
UWORD *x;
 /* read a byte; return 8 if byte available, 0 if not */
{
    if (csize-- <= 0)
        return 0;
    if (incnt == 0)  {
        if ((incnt = read(zipfd, inbuf, INBUFSIZ)) <= 0)
            return 0;
        inptr = inbuf;
    }
    *x = *inptr++;
    --incnt;
    return 8;
}


/* ------------------------------------------------------------- */
static UWORD mask_bits[] =
        {0,     0x0001, 0x0003, 0x0007, 0x000f,
                0x001f, 0x003f, 0x007f, 0x00ff,
                0x01ff, 0x03ff, 0x07ff, 0x0fff,
                0x1fff, 0x3fff, 0x7fff, 0xffff
        };


int FillBitBuffer(bits)
register int bits;
{
    /* get the bits that are left and read the next UWORD */
        register int result = bitbuf;
    UWORD temp;
    int sbits = bits_left;
    bits -= bits_left;

    /* read next UWORD of input */
    bits_left = ReadByte(&bitbuf);
    bits_left += ReadByte(&temp);

    bitbuf |= (temp << 8);
    if (bits_left == 0)
        zipeof = 1;

    /* get the remaining bits */
        result = result | (int) ((bitbuf & mask_bits[bits]) << sbits);
        bitbuf >>= bits;
        bits_left -= bits;
        return result;
}

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


/* ------------------------------------------------------------- */

#include "crc32.h"


/* ------------------------------------------------------------- */

void FlushOutput()
 /* flush contents of output buffer */
{

    UpdateCRC(outbuf, outcnt);

    if (aflag) {     /* ASCII->EBCDIC translation for buffers. 2.0f */
                     /*  Or <cr><nl> to <sp><nl> conversion */
        zmemcpy(outout,outbuf,outcnt);  /* Copy for translation v2.0g */
        ae_buf(outout, &outcnt);
    }

    if (!tflag) {       /* not testing, so we DO output */

        /* v2.0g If we're translating, output data is in outout.
         * Else it's in outbuf like always, and outout points to the
         * same place as outbuf.  (obtuse, ne?)
         */

        if (write(outfd, outout, outcnt) != outcnt) {   /* v2.0g */
            fprintf(stderr," File write error\n");
            exit(1);
        }
    }

    outpos += outcnt;
    outcnt = 0;
    outptr = outbuf;
}


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


/* ----------------------------------------------------------- */

void LoadFollowers()
{
    register int x;
    register int i;

    for (x = 255; x >= 0; x--) {
        READBIT(6,Slen[x]);
        for (i = 0; i < Slen[x]; i++) {
            READBIT(8,followers[x][i]);
        }
    }
}


/* ----------------------------------------------------------- */
/*
 * The Reducing algorithm is actually a combination of two
 * distinct algorithms.  The first algorithm compresses repeated
 * byte sequences, and the second algorithm takes the compressed
 * stream from the first algorithm and applies a probabilistic
 * compression method.
 */

int L_table[] = {0, 0x7f, 0x3f, 0x1f, 0x0f};

int D_shift[] = {0, 0x07, 0x06, 0x05, 0x04};
int D_mask[]  = {0, 0x01, 0x03, 0x07, 0x0f};

int B_table[] = {8, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5,
                 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
                 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7,
                 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                 8, 8, 8, 8};

/* ----------------------------------------------------------- */

void unReduce()
 /* expand probablisticly reduced data */
{
    register int lchar;
    int nchar;
    int ExState;
    int V;
    int Len;

    factor = lrec.compression_method - 1;
    ExState = 0;
    lchar = 0;
    LoadFollowers();

    while (((outpos+outcnt) < ucsize) && (!zipeof)) {
        if (Slen[lchar] == 0)
            READBIT(8,nchar)      /* ; */
        else
        {
            READBIT(1,nchar);
            if (nchar != 0)
                READBIT(8,nchar)      /* ; */
            else
            {
                int follower;
                int bitsneeded = B_table[Slen[lchar]];
                READBIT(bitsneeded,follower);
                nchar = followers[lchar][follower];
            }
        }

        /* expand the resulting byte */
        switch (ExState) {

        case 0:
            if (nchar != DLE)
                OUTB(nchar) /*;*/
            else
                ExState = 1;
            break;

        case 1:
            if (nchar != 0) {
                V = nchar;
                Len = V & L_table[factor];
                if (Len == L_table[factor])
                    ExState = 2;
                else
                    ExState = 3;
            }
            else {
                OUTB(DLE);
                ExState = 0;
            }
            break;

        case 2: {
                Len += nchar;
                ExState = 3;
            }
            break;

        case 3: {
                register int i = Len + 3;
                int offset = (((V >> D_shift[factor]) &
                                     D_mask[factor]) << 8) + nchar + 1;
                longint op = (outpos+outcnt) - offset;

                /* special case- before start of file */
                while ((op < 0L) && (i > 0)) {
                    OUTB(0);
                    op++;
                    i--;
                }

                /* normal copy of data from output buffer */
                {
                    register int ix = (int) (op % OUTBUFSIZ);

                    /* do a block memory copy if possible */
                    if ( ((ix    +i) < OUTBUFSIZ) &&
                      ((outcnt+i) < OUTBUFSIZ) ) {
                        zmemcpy(outptr,&outbuf[ix],i);
                        outptr += i;
                        outcnt += i;
                    }

                    /* otherwise copy byte by byte */
                    else while (i--) {
                        OUTB(outbuf[ix]);
                        if (++ix >= OUTBUFSIZ)
                            ix = 0;
                    }
                }

                ExState = 0;
            }
            break;
        }

        /* store character for next iteration */
        lchar = nchar;
    }
}


/* ------------------------------------------------------------- */
/*
 * Shrinking is a Dynamic Ziv-Lempel-Welch compression algorithm
 * with partial clearing.
 *
 */

void partial_clear()
{
    register int pr;
    register int cd;

    /* mark all nodes as potentially unused */
    for (cd = first_ent; cd < free_ent; cd++)
        prefix_of[cd] |= 0x8000;

    /* unmark those that are used by other nodes */
    for (cd = first_ent; cd < free_ent; cd++) {
        pr = prefix_of[cd] & 0x7fff;    /* reference to another node? */
        if (pr >= first_ent)            /* flag node as referenced */
            prefix_of[pr] &= 0x7fff;
    }

    /* clear the ones that are still marked */
    for (cd = first_ent; cd < free_ent; cd++)
        if ((prefix_of[cd] & 0x8000) != 0)
            prefix_of[cd] = -1;

    /* find first cleared node as next free_ent */
    cd = first_ent;
    while ((cd < maxcodemax) && (prefix_of[cd] != -1))
        cd++;
    free_ent = cd;
}


/* ------------------------------------------------------------- */

void unShrink()
{
#define  GetCode(dest) READBIT(codesize,dest)

    register int code;
    register int stackp;
    int finchar;
    int oldcode;
    int incode;


    /* decompress the file */
    maxcodemax = 1 << max_bits;
    codesize = init_bits;
    maxcode = (1 << codesize) - 1;
    free_ent = first_ent;
    offset = 0;
    sizex = 0;

    for (code = maxcodemax; code > 255; code--)
        prefix_of[code] = -1;

    for (code = 255; code >= 0; code--) {
        prefix_of[code] = 0;
        suffix_of[code] = code;
    }

    GetCode(oldcode);
    if (zipeof)
        return;
    finchar = oldcode;

    OUTB(finchar);

    stackp = hsize;

    while (!zipeof) {
        GetCode(code);
        if (zipeof)
            return;

        while (code == clear) {
            GetCode(code);
            switch (code) {

            case 1:{
                    codesize++;
                    if (codesize == max_bits)
                        maxcode = maxcodemax;
                    else
                        maxcode = (1 << codesize) - 1;
                }
                break;

            case 2:
                partial_clear();
                break;
            }

            GetCode(code);
            if (zipeof)
                return;
        }


        /* special case for KwKwK string */
        incode = code;
        if (prefix_of[code] == -1) {
            stack[--stackp] = finchar;
            code = oldcode;
        }


        /* generate output characters in reverse order */
        while (code >= first_ent) {
            stack[--stackp] = suffix_of[code];
            code = prefix_of[code];
        }

        finchar = suffix_of[code];
        stack[--stackp] = finchar;


        /* and put them out in forward order, block copy */
        if ((hsize-stackp+outcnt) < OUTBUFSIZ) {
            zmemcpy(outptr,&stack[stackp],hsize-stackp);
            outptr += hsize-stackp;
            outcnt += hsize-stackp;
            stackp = hsize;
        }

        /* output byte by byte if we can't go by blocks */
        else while (stackp < hsize)
            OUTB(stack[stackp++]);


        /* generate new entry */
        code = free_ent;
        if (code < maxcodemax) {
            prefix_of[code] = oldcode;
            suffix_of[code] = finchar;

            do
                code++;
            while ((code < maxcodemax) && (prefix_of[code] != -1));

            free_ent = code;
        }

        /* remember previous code */
        oldcode = incode;
    }
}


/* ------------------------------------------------------------- */
/*
 * Imploding
 * ---------
 *
 * The Imploding algorithm is actually a combination of two distinct
 * algorithms.  The first algorithm compresses repeated byte sequences
 * using a sliding dictionary.  The second algorithm is used to compress
 * the encoding of the sliding dictionary ouput, using multiple
 * Shannon-Fano trees.
 *
 */

#define LITVALS     256
#define DISTVALS    64
#define LENVALS     64
#define MAXSF       LITVALS

   typedef struct sf_entry {
       byte         Value;
       byte         BitLength;
   } sf_entry;

   typedef struct sf_tree {   /* a shannon-fano "tree" (table) */
      sf_entry     entry[MAXSF];
      int          entries;
      int          MaxLength;
   } sf_tree;

   typedef sf_tree      *sf_treep;

   typedef struct sf_node {   /* node in a true shannon-fano tree */
      UWORD         left;      /* 0 means leaf node */
      UWORD         right;     /* or value if leaf node */
   } sf_node;

   sf_tree  lit_tree;
   sf_tree  length_tree;
   sf_tree  distance_tree;
   /* s-f storage is shared with that used by other comp. methods */
   sf_node  *lit_nodes = (sf_node *) followers;     /* 2*LITVALS nodes */
   sf_node  *length_nodes = (sf_node *) suffix_of;  /* 2*LENVALS nodes */
   sf_node  *distance_nodes = (sf_node *) stack;    /* 2*DISTVALS nodes */
   boolean  lit_tree_present;
   boolean  eightK_dictionary;
   int      minimum_match_length;
   int      dict_bits;

#ifdef __TURBOC__
/* v2.0b More local prototypes */
void ReadLengths(sf_tree *tree);
void SortLengths(sf_tree *tree);
void GenerateTrees(sf_tree *tree, sf_node *nodes);
void LoadTree(sf_tree *tree, int treesize, sf_node *nodes);
void LoadTrees(void);
void ReadTree(register sf_node *nodes, int *dest);
#endif

void         SortLengths(tree)
sf_tree *tree;
  /* Sort the Bit Lengths in ascending order, while retaining the order
    of the original lengths stored in the file */
{
    register sf_entry *ejm1;    /* entry[j - 1] */
    register int j;
    register sf_entry *entry;
    register int i;
    sf_entry tmp;
    int entries;
    unsigned a, b;

    entry = &tree->entry[0];
    entries = tree->entries;

    for (i = 0; ++i < entries; )  {
        tmp = entry[i];
        b = tmp.BitLength;
        j = i;
        while ((j > 0)
        && ((a = (ejm1 = &entry[j - 1])->BitLength) >= b))  {
            if ((a == b) && (ejm1->Value <= tmp.Value))
                break;
            *(ejm1 + 1) = *ejm1;    /* entry[j] = entry[j - 1] */
            --j;
        }
        entry[j] = tmp;
    }
}


/* ----------------------------------------------------------- */

void         ReadLengths(tree)
sf_tree *tree;
{
   int    treeBytes;
   int    i;
   int    num, len;

  /* get number of bytes in compressed tree */
   READBIT(8,treeBytes);
   treeBytes++;
   i = 0;

   tree->MaxLength = 0;

/* High 4 bits: Number of values at this bit length + 1. (1 - 16)
 * Low  4 bits: Bit Length needed to represent value + 1. (1 - 16)
 */
   while (treeBytes > 0)
   {
      READBIT(4,len); len++;
      READBIT(4,num); num++;

      while (num > 0)
      {
         if (len > tree->MaxLength)
            tree->MaxLength = len;
         tree->entry[i].BitLength = len;
         tree->entry[i].Value = i;
         i++;
         num--;
      }

      treeBytes--;
   }
}


/* ----------------------------------------------------------- */

void         GenerateTrees(tree, nodes)
sf_tree *tree;
sf_node *nodes;
     /* Generate the Shannon-Fano trees */
{
    int codelen, i, j, lvlstart, next, parents;

    i = tree->entries - 1;          /* either 255 or 63 */
    lvlstart = next = 1;

    /* believe it or not, there may be a 1-bit code */

    for (codelen = tree->MaxLength; codelen >= 1; --codelen)  {

        /* create leaf nodes at level <codelen> */

        while ((i >= 0) && (tree->entry[i].BitLength == codelen))  {
            nodes[next].left = 0;
            nodes[next].right = tree->entry[i].Value;
            ++next;
            --i;
        }

        /* create parent nodes for all nodes at level <codelen>,
           but don't create the root node here */

        parents = next;
        if (codelen > 1)  {
            for (j = lvlstart; j <= parents-2; j += 2)  {
                nodes[next].left = j;
                nodes[next].right = j + 1;
                ++next;
            }
        }
        lvlstart = parents;
    }

    /* create root node */

    nodes[0].left = next - 2;
    nodes[0].right = next - 1;
}


/* ----------------------------------------------------------- */

void         LoadTree(tree, treesize, nodes)
sf_tree *tree;
int treesize;
sf_node *nodes;
     /* allocate and load a shannon-fano tree from the compressed file */
{
   tree->entries = treesize;
   ReadLengths(tree);
   SortLengths(tree);
   GenerateTrees(tree, nodes);
}


/* ----------------------------------------------------------- */

void         LoadTrees()
{
   eightK_dictionary = (lrec.general_purpose_bit_flag & 0x02) != 0; /* bit 1 */
   lit_tree_present = (lrec.general_purpose_bit_flag & 0x04) != 0;  /* bit 2 */

   if (eightK_dictionary)
      dict_bits = 7;
   else
      dict_bits = 6;

   if (lit_tree_present)
   {
      minimum_match_length = 3;
      LoadTree(&lit_tree,256,lit_nodes);
   }
   else
      minimum_match_length = 2;

   LoadTree(&length_tree,64,length_nodes);
   LoadTree(&distance_tree,64,distance_nodes);
}


/* ----------------------------------------------------------- */

#ifndef ASM

void         ReadTree(nodes, dest)
register sf_node *nodes;
int *dest;
     /* read next byte using a shannon-fano tree */
{
    register int cur;
    register int left;
    UWORD b;

    for (cur = 0; ; )  {
        if ((left = nodes[cur].left) == 0)  {
            *dest = nodes[cur].right;
            return;
        }
        READBIT(1, b);
        cur = (b ? nodes[cur].right : left);
    }
}

#endif

/* ----------------------------------------------------------- */

void         unImplode()
     /* expand imploded data */
{
   register int srcix;
   register int Length;
   register int limit;
   int          lout;
   int          Distance;

   LoadTrees();

#ifdef DEBUG
   printf("\n");
#endif
   while ((!zipeof) && ((outpos+outcnt) < ucsize))
   {
      READBIT(1,lout);

      if (lout != 0)   /* encoded data is literal data */
      {
         if (lit_tree_present)  /* use Literal Shannon-Fano tree */  {
            ReadTree(lit_nodes,&lout);
#ifdef DEBUG
        printf("lit=%d\n", lout);
#endif
     }
         else
            READBIT(8,lout);

         OUTB(lout);
      }
      else             /* encoded data is sliding dictionary match */
      {
         READBIT(dict_bits,Distance);

         ReadTree(distance_nodes,&lout);
#ifdef DEBUG
     printf("d=%5d (%2d,%3d)", (lout << dict_bits) | Distance, lout,
            Distance);
#endif
         Distance |= (lout << dict_bits);
         /* using the Distance Shannon-Fano tree, read and decode the
            upper 6 bits of the Distance value */

         ReadTree(length_nodes,&lout);
     Length = lout;
#ifdef DEBUG
     printf("\tl=%3d\n", Length);
#endif
         /* using the Length Shannon-Fano tree, read and decode the
            Length value */

         if (Length == 63)
         {
            READBIT(8,lout);
            Length += lout;
         }
         Length += minimum_match_length;

        /* move backwards Distance+1 bytes in the output stream, and copy
          Length characters from this position to the output stream.
          (if this position is before the start of the output stream,
          then assume that all the data before the start of the output
          stream is filled with zeros.  Requires initializing outbuf
          for each file.) */

        srcix = (outcnt - (Distance + 1)) & (OUTBUFSIZ-1);
        limit = OUTBUFSIZ - Length;
        if ((srcix <= limit) && (outcnt < limit))  {
            zmemcpy(outptr, &outbuf[srcix], Length);
            outptr += Length;
            outcnt += Length;
        }
        else {
            while (Length--)  {
                OUTB(outbuf[srcix++]);
                srcix &= OUTBUFSIZ-1;
            }
        }

      }
   }
}



/* ---------------------------------------------------------- */

/*
 Length  Method   Size  Ratio   Date    Time   CRC-32    Name
 ------  ------   ----- -----   ----    ----   ------    ----
  44004  Implode  13041  71%  11-02-89  19:34  88420727  DIFF3.C
 */

void dir_member()
{
    char *method;
    int ratio;
    int yr, mo, dy, hh, mm;

    yr = (((lrec.last_mod_file_date >> 9) & 0x7f) + 80);
    mo = ((lrec.last_mod_file_date >> 5) & 0x0f);
    dy = (lrec.last_mod_file_date & 0x1f);

    hh = ((lrec.last_mod_file_time >> 11) & 0x1f);
    mm = ((lrec.last_mod_file_time >> 5) & 0x3f);

    switch (lrec.compression_method)  {
    case 0:
        method = "Stored";
        break;
    case 1:
        method = "Shrunk";
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        method = "Reduced";
        break;
    case 6:
        method = "Implode";
        break;
    }

    if (ucsize != 0)  {
        ratio = (int) ((1000L * (ucsize - csize)) / ucsize);
        if ((ratio % 10) >= 5)
            ratio += 10;
    }
    else
        ratio = 0;  /* can .zip contain 0-size file? */

#ifdef NOTINT16     /* v2.0c */
    printf("%7ld  %-7s%7ld %3d%%  %02d-%02d-%02d  %02d:%02d  \
%08lx  %s\n", ucsize, method, csize,
        ratio / 10, mo, dy, yr, hh, mm,
        lrec.crc32, filename);
#else   /* !NOTINT16 */
    printf("%7ld  %-7s%7ld %3d%%  %02d-%02d-%02d  %02d:%02d  \
%08lx  %s\n", ucsize, method, csize,
        ratio / 10, mo, dy, yr, hh, mm,
        LONGI(lrec.crc32), filename);
#endif  /* NOTINT16 */
    tot_ucsize += ucsize;
    tot_csize += csize;
    ++members;
}

/* ---------------------------------------------------------- */

void skip_member()
{
    register long pos;
    long endbuf;
    int offset;

    endbuf = lseek(zipfd, 0L, SEEK_CUR);    /* 1st byte beyond inbuf */
    pos = endbuf - incnt;                   /* 1st compressed byte */
    pos += csize;                           /* next header signature */
    if (pos < endbuf)  {
        incnt -= csize;
        inptr += csize;
    }
    else  {
        offset = pos % BSIZE;               /* offset within block */
        pos = (pos / BSIZE) * BSIZE;        /* block start */
        lseek(zipfd, pos, SEEK_SET);
        incnt = read(zipfd, inbuf, INBUFSIZ);
        incnt -= offset;
        inptr = inbuf + offset;
    }
}

/* ---------------------------------------------------------- */

void extract_member()
{
        UWORD     b;
/* for test reasons */

    bits_left = 0;
    bitbuf = 0;
    outpos = 0L;
    outcnt = 0;
    outptr = outbuf;
    zipeof = 0;
    crc32val = 0xFFFFFFFFL;

    zmemset(outbuf, 0, OUTBUFSIZ);
    if (aflag)                          /* if we have scratchpad.. v2.0g */
        zmemset(outout, 0, OUTBUFSIZ);  /* ..clear it out v2.0g */

    if (tflag)
        fprintf(stderr, "Testing: %-12s ", filename);
    else {
        if(!mapped_name())	/* member name conversion failed  v2.0j */
	    exit(1);		/* choke and die v2.0j */

        /* create the output file with READ and WRITE permissions */
        if (create_output_file())
            exit(1);
    }
    switch (lrec.compression_method) {

    case 0: {   /* stored */
            if (!tflag)
                fprintf(stderr, " Extracting: %-12s ", filename);
            if (cflag) fprintf(stderr, "\n");
            while (ReadByte(&b))
                OUTB(b);
        }
        break;

    case 1: {       /* shrunk */
            if (!tflag)
                fprintf(stderr, "UnShrinking: %-12s ", filename);
            if (cflag) fprintf(stderr, "\n");
            unShrink();
        }
        break;

    case 2:
    case 3:
    case 4:
    case 5: {
            if (!tflag)
                fprintf(stderr, "  Expanding: %-12s ", filename);
            if (cflag) fprintf(stderr, "\n");
            unReduce();
        }
        break;

    case 6: {
            if (!tflag)
                fprintf(stderr, "  Exploding: %-12s ", filename);
            if (cflag) fprintf(stderr, "\n");
            unImplode();
        }
        break;

    default:
        fprintf(stderr, "Unknown compression method.");
    }

    /* write the last partial buffer, if any */
    if (outcnt > 0) {
        UpdateCRC(outbuf, outcnt);

                                /* Ascii->ebcdic or <cr> -> <sp> translation */
        if (aflag) {            /* v2.0f */
            zmemcpy(outout,outbuf,outcnt);      /* v2.0g */
            ae_buf(outout, &outcnt);             /* v2.0g */
        }

        if (!tflag) {                           /* not testing, so output */

            /* v2.0g If we're translating, output data is in outout.
             * Else it's in outbuf like always, and outout points to the
             * same place as outbuf.  (obtuse, ne?)
             */

            write(outfd, outout, outcnt);       /* v2.0g */
        }
    }

    if (!tflag)  {
#ifndef UNIX
        /* set output file date and time */
        set_file_time();
        close(outfd);
#else
        close(outfd);
        /* set output file date and time */
        set_file_time();
#endif
    }

    crc32val = ~crc32val;
#ifdef NOTINT16     /* v2.0c */
    if (crc32val != lrec.crc32)
        fprintf(stderr, " Bad CRC %08lx  (should be %08lx)", crc32val,
            lrec.crc32);
#else   /* !NOTINT16 */
    if (crc32val != LONGI(lrec.crc32))
        fprintf(stderr, " Bad CRC %08lx  (should be %08lx)", crc32val,
            LONGI(lrec.crc32));
#endif  /* NOTINT16 */

    else if (tflag)
        fprintf(stderr, " OK");

    fprintf(stderr, "\n");
}


/* ---------------------------------------------------------- */

void get_string(len, s)
int len;
char *s;
{
    readbuf(zipfd, s, len);
    s[len] = 0;

#ifdef EBCDIC           /* translate the filename to ebcdic */
    a_to_e( s );        /* A.B.  03/21/90                   */
#endif
}


/* ---------------------------------------------------------- */

void process_local_file_header(fnamev)
char **fnamev;
{
    int extracted;
#ifdef NOTINT16     /* v2.0c */
    local_byte_header brec;
#endif

#ifndef NOTINT16    /* v2.0c */
    readbuf(zipfd, (char *) &lrec, sizeof(lrec));   /* v2.0b */
#else   /* NOTINT16 */
    readbuf(zipfd, (char *) &brec, sizeof(brec));

    lrec.version_needed_to_extract =
        makeword(brec.version_needed_to_extract);
    lrec.general_purpose_bit_flag =
        makeword(brec.general_purpose_bit_flag);
    lrec.compression_method =
        makeword(brec.compression_method);
    lrec.last_mod_file_time =
        makeword(brec.last_mod_file_time);
    lrec.last_mod_file_date =
        makeword(brec.last_mod_file_date);
    lrec.crc32 =
        makelong(brec.crc32);
    lrec.compressed_size =
        makelong(brec.compressed_size);
    lrec.uncompressed_size =
        makelong(brec.uncompressed_size);
    lrec.filename_length =
        makeword(brec.filename_length);
    lrec.extra_field_length =
        makeword(brec.extra_field_length);
#endif  /* NOTINT16 */

#ifdef HIGH_LOW
    swap_bytes(&lrec.filename_length);
    swap_bytes(&lrec.extra_field_length);
    swap_lbytes(LONGIP(lrec.compressed_size));
    swap_lbytes(LONGIP(lrec.uncompressed_size));
    swap_bytes(&lrec.compression_method);
    swap_bytes(&lrec.version_needed_to_extract);
    swap_bytes(&lrec.general_purpose_bit_flag);
    swap_bytes(&lrec.last_mod_file_time);
    swap_bytes(&lrec.last_mod_file_date);
    swap_lbytes(LONGIP(lrec.crc32));
#endif  /* HIGH_LOW */

#ifdef NOTINT16     /* v2.0c */
    csize = lrec.compressed_size;
    ucsize = lrec.uncompressed_size;
#else   /* !NOTINT16 */
    csize = LONGI(lrec.compressed_size);
    ucsize = LONGI(lrec.uncompressed_size);
#endif  /* NOTINT16 */

    get_string(lrec.filename_length, filename);
    get_string(lrec.extra_field_length, extra);

    extracted = 0;
    for (--fnamev; *++fnamev; )  {
        if (match(filename, *fnamev))  {
            if (vflag)
                dir_member();
            else  {
                extract_member();
                extracted = 1;
            }
            break;
        }
    }
    if (!extracted)
        skip_member();
}


/* ---------------------------------------------------------- */

void process_central_file_header()
{
    central_directory_file_header rec;
    char filename[STRSIZ];
    char extra[STRSIZ];
/*  char comment[STRSIZ]; v2.0b using global comment so we can display it */

#ifdef NOTINT16     /* v2.0c */
    central_directory_byte_header byterec;
#endif

#ifndef NOTINT16    /* v2.0c */
    readbuf(zipfd, (char *) &rec, sizeof(rec)); /* v2.0b */
#else   /* NOTINT16 */
    readbuf(zipfd, (char *) &byterec, sizeof(byterec) );        /* v2.0c */

    rec.version_made_by =
        makeword(byterec.version_made_by);
    rec.version_needed_to_extract =
        makeword(byterec.version_needed_to_extract);
    rec.general_purpose_bit_flag =
        makeword(byterec.general_purpose_bit_flag);
    rec.compression_method =
        makeword(byterec.compression_method);
    rec.last_mod_file_time =
        makeword(byterec.last_mod_file_time);
    rec.last_mod_file_date =
        makeword(byterec.last_mod_file_date);
    rec.crc32 =
        makelong(byterec.crc32);
    rec.compressed_size =
        makelong(byterec.compressed_size);
    rec.uncompressed_size =
        makelong(byterec.uncompressed_size);
    rec.filename_length =
        makeword(byterec.filename_length);
    rec.extra_field_length =
        makeword(byterec.extra_field_length);
    rec.file_comment_length =
        makeword(byterec.file_comment_length);
    rec.disk_number_start =
        makeword(byterec.disk_number_start);
    rec.internal_file_attributes =
        makeword(byterec.internal_file_attributes);
    rec.external_file_attributes =
        makeword(byterec.external_file_attributes);
    rec.relative_offset_local_header =
        makelong(byterec.relative_offset_local_header);
#endif  /* NOTINT16 */

#ifdef HIGH_LOW
    swap_bytes(&rec.filename_length);
    swap_bytes(&rec.extra_field_length);
    swap_bytes(&rec.file_comment_length);
#endif

    get_string(rec.filename_length, filename);
    get_string(rec.extra_field_length, extra);
    get_string(rec.file_comment_length, comment);
}


/* ---------------------------------------------------------- */

void process_end_central_dir()
{
    end_central_dir_record rec;
/*  char comment[STRSIZ]; v2.0b made global */

#ifdef NOTINT16     /* v2.0c */
    end_central_byte_record byterec;
#endif

#ifndef NOTINT16    /* v2.0c */
    readbuf(zipfd, (char *) &rec, sizeof(rec)); /* v2.0b */
#else   /* NOTINT16 */
    readbuf(zipfd, (char *) &byterec, sizeof(byterec) );

    rec.number_this_disk =
        makeword(byterec.number_this_disk);
    rec.number_disk_with_start_central_directory =
        makeword(byterec.number_disk_with_start_central_directory);
    rec.total_entries_central_dir_on_this_disk =
        makeword(byterec.total_entries_central_dir_on_this_disk);
    rec.total_entries_central_dir =
        makeword(byterec.total_entries_central_dir);
    rec.size_central_directory =
        makelong(byterec.size_central_directory);
    rec.offset_start_central_directory =
        makelong(byterec.offset_start_central_directory);
    rec.zipfile_comment_length =
        makeword(byterec.zipfile_comment_length);
#endif  /* NOTINT16 */

#ifdef HIGH_LOW
    swap_bytes(&rec.zipfile_comment_length);
#endif

    /* There seems to be no limit to the zipfile
       comment length.  Some zipfiles have comments
       longer than 256 bytes.  Currently no use is
       made of the comment anyway.
     */
/* #if 0
 * v2.0b Enabling comment display
 */
    get_string(rec.zipfile_comment_length, comment);
/* #endif */
}


/* ---------------------------------------------------------- */

void process_headers()
{
    int ratio;
    long sig;

#ifdef NOTINT16     /* v2.0c */
    byte sigbyte[4];
#endif

    if (vflag)  {
        members = 0;
        tot_ucsize = tot_csize = 0;
        printf("\n Length  Method   Size  Ratio   Date    Time   \
CRC-32    Name\n ------  ------   ----- -----   ----    ----   ------    \
----\n");
    }

    while (1) {
#ifdef NOTINT16     /* v2.0c */
    if (readbuf(zipfd, (char *) sigbyte, 4) != 4)
#else   /* !NOTINT16 */
    if (readbuf(zipfd, (char *) &sig, sizeof(sig)) != sizeof(sig))
#endif  /* NOTINT16 */
        return;

#ifdef NOTINT16     /* v2.0c */
        sig = makelong(sigbyte);
#endif

#ifdef HIGH_LOW
        swap_lbytes(&sig);
#endif

        if (sig == LOCAL_FILE_HEADER_SIGNATURE)
            process_local_file_header(fnv);
        else if (sig == CENTRAL_FILE_HEADER_SIGNATURE)
            process_central_file_header();
        else if (sig == END_CENTRAL_DIR_SIGNATURE) {
            process_end_central_dir();
            break;
        }
        else {
            fprintf(stderr, "Invalid Zipfile Header\n");
            return;
        }
    }
    if (vflag)  {
        if (tot_ucsize != 0)  {
            ratio = (int) ((1000L * (tot_ucsize-tot_csize))
                    / tot_ucsize);
            if ((ratio % 10) >= 5)
                ratio += 10;
        }
        else
            ratio = 0;
        printf(" ------          ------  \
---                             -------\n\
%7ld         %7ld %3d%%                             %7d\n",
        tot_ucsize, tot_csize, ratio / 10, members);

        if( comment[0] )                /* v2.0b */
            printf("%s\n",comment);     /* v2.0b */
    }
}


/* ---------------------------------------------------------- */

void process_zipfile()
{
    /*
     * open the zipfile for reading and in BINARY mode to prevent cr/lf
     * translation, which would corrupt the bitstreams
     */

    if (open_input_file())
        exit(1);

    process_headers();

    close(zipfd);
}

/* ---------------------------------------------------------- */

void usage()        /* v2.0j */
{

#ifdef EBCDIC                              /* A.B. 03/20/90   */
  char *astring = "-a  ascii to ebcdic conversion";
#else
  char *astring = "-a  carriage return to space conversion";
#endif

fprintf(stderr, "\n%s\nCourtesy of:  S.H.Smith  and  The Tool Shop BBS,  (602) 279-2673.\n\n",VERSION);
fprintf(stderr, "Usage:  unzip [-tcv] file[.zip] [filespec...]\n\
  -t  test member files\n\
  -c  output to stdout\n\
  %s\n\
  -m  map extracted filenames to lowercase\n\
  -v  view directory\n",astring);
    exit(1);
}

/* ---------------------------------------------------------- */

/*
 * main program
 *
 */

void main(argc, argv)
int argc;
char **argv;
{
    char *s;
    int c;

#ifdef DEBUG_STRUC                      /* v2.0e */
printf("local_file_header size: %X\n",
    sizeof(struct local_file_header) );
printf("local_byte_header size: %X\n",
    sizeof(struct local_byte_header) );

printf("central directory header size: %X\n",
    sizeof(struct central_directory_file_header) );
printf("central directory byte header size: %X\n",
    sizeof(struct central_directory_byte_header) );

printf("end central dir record size: %X\n",
    sizeof(struct end_central_dir_record) );
printf("end central dir byte record size: %X\n",
    sizeof(struct end_central_byte_record) );
#endif

    while (--argc > 0 && (*++argv)[0] == '-')  {
        s = argv[0] + 1;
#ifndef __TURBOC__
        while (c = *s++)  {
#else
        while ((c = *s++) != '\0') {      /* v2.0b */
#endif

/* Something is SERIOUSLY wrong with my host's tolower() function! */
/* #ifndef BSD */
/*            switch (tolower(c))  { */
/* #else */
            switch(c) {
/* #endif */
            case ('T'):
            case ('t'):
                ++tflag;
                break;
            case ('V'):
            case ('v'):
                ++vflag;
                break;
            case ('C'):
            case ('c'):
                ++cflag;
#ifdef EBCDIC
                ++aflag;       /* This is so you can read it on the screen */
#endif                         /*  A.B.  03/24/90                          */
                break;
            case ('A'):
            case ('a'):
                ++aflag;
                break;
	    case ('M'):		/* map filename flag v2.0j */
	    case ('m'):
		++mflag;
		break;
            default:
                usage();
                break;
            }
        }
    }
/*  I removed the filter for the a and c flags so they could be used together */
/*  Especially on EBCDIC based systems where you would want to read the text  */
/*  on the screen.  Allan Bjorklund.  03/24/90                                */

    if ((tflag && vflag) || (tflag && cflag) || (vflag && cflag) ||
        (tflag && aflag) || (aflag && vflag))

    {
        fprintf(stderr, "only one of -t, -c, -a, or -v\n");
        exit(1);
    }
    if (argc-- == 0)
        usage();

    /* .ZIP default if none provided by user */
    strcpy(zipfn, *argv++);
/* v2.0b This doesn't permit paths like "..\dir\filename" */

#ifdef OLDSTUF
    if (strchr(zipfn, '.') == NULL)
        strcat(zipfn, ZSUFX);
#else    /* v2.0b New code */

    c = strlen(zipfn);
    if ( (c < 5)                             /* less than x.zip */
      || (strcmp (&zipfn[c-4], ZSUFX) != 0)) /* v2.0b type doesn't
                                              * match */
        strcat (zipfn, ZSUFX);
#endif

    /* if any member file specs on command line, set filename
       pointer to point to them. */

    if (argc != 0)
        fnv = argv;

        /* allocate i/o buffers */
    inbuf = (byte *) (malloc(INBUFSIZ));
    outbuf = (byte *) (malloc(OUTBUFSIZ));

    /* v2.0g Hacked Allan's code.  No need allocating an ascebc
     * scratch buffer unless we're doing translation.
     */

    if(aflag)                           /* we need an ascebc scratch v2.0g */
        outout = (byte *) (malloc(OUTBUFSIZ)); /* ..so allocate it v2.0g */
    else
        outout = outbuf;                /* just point to outbuf v2.0g */

    if ((inbuf == NULL) || (outbuf == NULL) || (outout == NULL)) {  /* v2.0g */
        fprintf(stderr, "Can't allocate buffers!\n");
        exit(1);
    }

    /* do the job... */
    process_zipfile();
    exit(0);
}
