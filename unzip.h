/*---------------------------------------------------------------------------

  unzip.h

  This header file is used by all of the unzip source files.  Its contents
  are divided into six more-or-less separate sections:  OS-dependent includes,
  (mostly) OS-independent defines, typedefs, function prototypes (or "proto-
  types," in the case of non-ANSI compilers), macros, and global-variable
  declarations.

  ---------------------------------------------------------------------------*/



/***************************/
/*  OS-Dependent Includes  */
/***************************/

#include <stdio.h>       /* this is your standard header for all C compiles */
#include <ctype.h>
#include <errno.h>       /* used in mapped_name() */
#define DECLARE_ERRNO    /* everybody except MSC 6.0 */
#ifdef VMS               /* sigh...you just KNEW someone had to break this.  */
#  include <types.h>     /*  (placed up here instead of in VMS section below */
#  include <stat.h>      /*   because types.h is used in some other headers) */
#else  /* almost everybody */
#  if defined(THINK_C) || defined(MPW) /* for Macs */
#    include <stddef.h>
#  else
#    include <sys/types.h> /* off_t, time_t, dev_t, ... */
#    include <sys/stat.h>  /* Everybody seems to need this. */
#  endif
#endif                   /*   This include file defines
                          *     #define S_IREAD 0x0100  (owner may read)
                          *     #define S_IWRITE 0x0080 (owner may write)
                          *   as used in the creat() standard function.  Must
                          *   be included AFTER sys/types.h for most systems.
                          */


/*---------------------------------------------------------------------------
    Next, a word from our Unix (mostly) sponsors:
  ---------------------------------------------------------------------------*/

#ifdef UNIX               /* On some systems sys/param.h partly duplicates   */
#  include <sys/param.h>  /*  the contents of sys/types.h, so you don't need */
                          /*  (and can't use) sys/types.h. (or param.h???)   */
#  ifndef BSIZE
#    define BSIZE   DEV_BSIZE   /* assume common for all Unix systems */
#  endif

#  ifndef BSD
#    define NO_MKDIR            /* for mapped_name() */
#    include <time.h>
     struct tm *gmtime(), *localtime();
#  else   /* BSD */
#    include <sys/time.h>
#    include <sys/timeb.h>
#  endif

#else   /* !UNIX */
#  define BSIZE   512           /* disk block size */
#endif

#if defined(V7) || defined(BSD)
#  define strchr    index
#  define strrchr   rindex
#endif

/*---------------------------------------------------------------------------
    And now, our MS-DOS and OS/2 corner:
  ---------------------------------------------------------------------------*/

#ifdef __TURBOC__
#  define DOS_OS2             /* Turbo C under DOS, MSC under DOS or OS2    */
#  include <sys/timeb.h>      /* for structure ftime                        */
#  include <mem.h>            /* for memcpy()                               */
#else                         /* NOT Turbo C...                             */
#  ifdef MSDOS                /*   but still MS-DOS, so we'll assume it's   */
#    ifndef MSC               /*   Microsoft's compiler and fake the ID, if */
#      define MSC             /*   necessary (it is in 5.0; apparently not  */
#    endif                    /*   in 5.1 and 6.0)                          */
#    include <dos.h>          /* _dos_setftime()                            */
#  endif
#  ifdef OS2                  /* stuff for DOS and OS/2 family version */
#    ifndef MSC
#      define MSC
#    endif
#    include <os2.h>          /* DosQFileInfo(), DosSetFileInfo()? */
#  endif
#endif

#ifdef MSC                    /* defined for all versions of MSC now         */
#  define DOS_OS2             /* Turbo C under DOS, MSC under DOS or OS/2    */
#  ifndef __STDC__            /* MSC 5.0 and 5.1 aren't truly ANSI-standard, */
#    define __STDC__          /*   but they understand prototypes...so       */
#  endif                      /*   they're close enough for our purposes     */
#  if defined(_MSC_VER) && (_MSC_VER >= 600)      /* new with 5.1 or 6.0 ... */
#    undef DECLARE_ERRNO      /* errno is now a function in a dynamic link   */
#  endif                      /*   library (or something)--incompatible with */
#endif                        /*   the usual "extern int errno" declaration  */

#ifdef DOS_OS2                /* defined for both Turbo C, MSC */
#  include <io.h>             /* lseek(), open(), setftime(), dup(), creat() */
#endif

/*---------------------------------------------------------------------------
    Followed by some VMS (mostly) stuff:
  ---------------------------------------------------------------------------*/

#ifdef VMS
#  include <time.h>             /* the usual non-BSD time functions */
#  include <file.h>             /* same things as fcntl.h has */
#  define UNIX                  /* can share most of same code from now on */
#  define RETURN   return_VMS   /* VMS interprets return codes incorrectly */
#else
#  define RETURN   return       /* only used in main() */
#  ifdef V7
#    define O_RDONLY  0
#    define O_WRONLY  1
#    define O_RDWR    2
#  else
#    ifdef MTS
#      include <sys/file.h>     /* MTS uses this instead of fcntl.h */
#    else
#      include <fcntl.h>
#    endif
#  endif
#endif
/*
 *   fcntl.h (above):   This include file defines
 *                        #define O_BINARY 0x8000  (no cr-lf translation)
 *                      as used in the open() standard function.
 */

/*---------------------------------------------------------------------------
    And some Mac stuff for good measure:
  ---------------------------------------------------------------------------*/

#ifdef THINK_C
#  define MACOS
#  define NOTINT16
#  ifndef __STDC__            /* THINK_C isn't truly ANSI-standard, */
#    define __STDC__          /*   but it understands prototypes...so */
#  endif                      /*   it's close enough for our purposes */
#  include <time.h>
#  include <unix.h>
#  include "macstat.h"
#endif
#ifdef MPW                    /* not tested yet - should be easy enough tho */
#  define MACOS
#  define NOTINT16
#  include <time.h>
#  include <fcntl.h>
#  include "macstat.h"
#endif

/*---------------------------------------------------------------------------
    And finally, some random extra stuff:
  ---------------------------------------------------------------------------*/

#ifdef __STDC__
#  include <stdlib.h>      /* standard library prototypes, malloc(), etc. */
#  include <string.h>      /* defines strcpy, strcmp, memcpy, etc. */
#else
   char *malloc();
   char *strchr(), *strrchr();
   long lseek();
#endif





/*************/
/*  Defines  */
/*************/

#define INBUFSIZ          BUFSIZ   /* same as stdio uses */
#define DIR_BLKSIZ        64       /* number of directory entries per block
                                    *  (should fit in 4096 bytes, usually) */
#define FILNAMSIZ         (1025)
#ifdef MTS
#  undef FILENAME_MAX              /* the MTS value is too low: use default */
#endif
#ifndef FILENAME_MAX               /* make sure FILENAME_MAX always exists  */
#  define FILENAME_MAX    (FILNAMSIZ - 1)
#endif
#ifdef ZIPINFO
#  define OUTBUFSIZ       BUFSIZ   /* zipinfo needs less than unzip does    */
#else
#  define OUTBUFSIZ       0x2000   /* unImplode needs power of 2, >= 0x2000 */
#endif

#define ZSUFX             ".zip"
#define CENTRAL_HDR_SIG   "\120\113\001\002"   /* the infamous "PK" */
#define LOCAL_HDR_SIG     "\120\113\003\004"   /*  signature bytes  */
#define END_CENTRAL_SIG   "\120\113\005\006"

#define SKIP              0    /* choice of activities for do_string() */
#define DISPLAY           1
#define FILENAME          2

#define DOS_OS2_FAT_      0    /* version_made_by codes (central dir) */
#define AMIGA_            1
#define VMS_              2    /* MAKE SURE THESE ARE NOT DEFINED ON     */
#define UNIX_             3    /* THE RESPECTIVE SYSTEMS!!  (Like, for   */
#define VM_CMS_           4    /* instance, "UNIX":  CFLAGS = -O -DUNIX) */
#define ATARI_            5
#define OS2_HPFS_         6
#define MAC_              7
#define Z_SYSTEM_         8
#define CPM_              9
/* #define TOPS20_   10  (we're going to need this soon...)  */
#define NUM_HOSTS         10   /* index of last system + 1 */

#define STORED            0    /* compression methods */
#define SHRUNK            1
#define REDUCED1          2
#define REDUCED2          3
#define REDUCED3          4
#define REDUCED4          5
#define IMPLODED          6
#define NUM_METHODS       7    /* index of last method + 1 */
/* don't forget to update list_files() appropriately if NUM_METHODS changes */

#ifndef MACOS
#  define TRUE            1    /* sort of obvious */
#  define FALSE           0
#endif

#define UNZIP_VERSION   11     /* compatible with PKUNZIP 1.1 */

#ifndef UNZIP_OS               /* not used yet, but will need for Zip  */
#  ifdef UNIX                  /* (could be defined in Makefile or...) */
#    define UNZIP_OS    UNIX_
#  endif
#  ifdef DOS_OS2
#    define UNZIP_OS    DOS_OS2_FAT_
#  endif
#  ifdef VMS
#    define UNZIP_OS    VMS_
#  endif
#  ifdef MTS
#    define UNZIP_OS    UNIX_
#  endif
#  ifdef MACOS
#    define UNZIP_OS    MAC_
#  endif
#  ifndef UNZIP_OS             /* still not defined:  default setting */
#    define UNZIP_OS    UNKNOWN
#  endif
#endif

/*---------------------------------------------------------------------------
    Macros for accessing the ULONG header fields.  When NOTINT16 is *not*
    defined, these fields are allocated as arrays of char within the structs.
    This prevents 32-bit compilers from padding the structs so that ULONGs
    start on 4-byte boundaries (this will not work on machines that can ONLY
    access ULONGs if they start on 4-byte boundaries).  If NOTINT16 *is*
    defined, however, the original data are individually copied into working
    structs consisting of UWORDs and ULONGs (which may therefore be padded
    arbitrarily), so the ULONGs are accessed normally.
  ---------------------------------------------------------------------------*/

#ifdef NOTINT16
#  define ULONG_(X)   X
#else
#  define ULONG_(X)   (*((ULONG *) (X)))
#endif

/*---------------------------------------------------------------------------
    True sizes of the various headers, as defined by Phil Katz--so it is not
    likely that these will ever change.  But if they do, make sure both these
    defines AND the typedefs below get updated accordingly.
  ---------------------------------------------------------------------------*/

#define LREC_SIZE     26    /* lengths of local file headers, central */
#define CREC_SIZE     42    /*  directory headers, and the end-of-    */
#define ECREC_SIZE    18    /*  central-dir record, respectively      */


#define MAX_BITS      13                 /* used in unShrink() */
#define HSIZE         (1 << MAX_BITS)    /* size of global work area */

#define LF   10   /* '\n' on ASCII machines.  Must be 10 due to EBCDIC */
#define CR   13   /* '\r' on ASCII machines.  Must be 13 due to EBCDIC */

#ifdef EBCDIC
#  define ascii_to_native(c)   ebcdic[(c)]
#endif

#ifndef SEEK_SET        /* These should all be declared in stdio.h!  But   */
#  define SEEK_SET  0   /*  since they're not (in many cases), do so here. */
#  define SEEK_CUR  1
#  define SEEK_END  2
#endif





/**************/
/*  Typedefs  */
/**************/

typedef unsigned char    byte;       /* code assumes UNSIGNED bytes */
typedef long             longint;
typedef unsigned short   UWORD;
typedef unsigned long    ULONG;
typedef char             boolean;

/*---------------------------------------------------------------------------
    Zipfile layout declarations.  If these headers ever change, make sure the
    ??REC_SIZE defines (above) change with them!
  ---------------------------------------------------------------------------*/

#ifdef NOTINT16

   typedef byte   local_byte_header[ LREC_SIZE ];
#      define L_VERSION_NEEDED_TO_EXTRACT_0     0
#      define L_VERSION_NEEDED_TO_EXTRACT_1     1
#      define L_GENERAL_PURPOSE_BIT_FLAG        2
#      define L_COMPRESSION_METHOD              4
#      define L_LAST_MOD_FILE_TIME              6
#      define L_LAST_MOD_FILE_DATE              8
#      define L_CRC32                           10
#      define L_COMPRESSED_SIZE                 14
#      define L_UNCOMPRESSED_SIZE               18
#      define L_FILENAME_LENGTH                 22
#      define L_EXTRA_FIELD_LENGTH              24

   typedef byte   central_directory_byte_header[ CREC_SIZE ];
#      define C_VERSION_MADE_BY_0               0
#      define C_VERSION_MADE_BY_1               1
#      define C_VERSION_NEEDED_TO_EXTRACT_0     2
#      define C_VERSION_NEEDED_TO_EXTRACT_1     3
#      define C_GENERAL_PURPOSE_BIT_FLAG        4
#      define C_COMPRESSION_METHOD              6
#      define C_LAST_MOD_FILE_TIME              8
#      define C_LAST_MOD_FILE_DATE              10
#      define C_CRC32                           12
#      define C_COMPRESSED_SIZE                 16
#      define C_UNCOMPRESSED_SIZE               20
#      define C_FILENAME_LENGTH                 24
#      define C_EXTRA_FIELD_LENGTH              26
#      define C_FILE_COMMENT_LENGTH             28
#      define C_DISK_NUMBER_START               30
#      define C_INTERNAL_FILE_ATTRIBUTES        32
#      define C_EXTERNAL_FILE_ATTRIBUTES        34
#      define C_RELATIVE_OFFSET_LOCAL_HEADER    38

   typedef byte   end_central_byte_record[ ECREC_SIZE+4 ];
/*     define SIGNATURE                         0   space-holder only */
#      define NUMBER_THIS_DISK                  4
#      define NUM_DISK_WITH_START_CENTRAL_DIR   6
#      define NUM_ENTRIES_CENTRL_DIR_THS_DISK   8
#      define TOTAL_ENTRIES_CENTRAL_DIR         10
#      define SIZE_CENTRAL_DIRECTORY            12
#      define OFFSET_START_CENTRAL_DIRECTORY    16
#      define ZIPFILE_COMMENT_LENGTH            20


   typedef struct local_file_header {                 /* LOCAL */
       byte version_needed_to_extract[2];
       UWORD general_purpose_bit_flag;
       UWORD compression_method;
       UWORD last_mod_file_time;
       UWORD last_mod_file_date;
       ULONG crc32;
       ULONG compressed_size;
       ULONG uncompressed_size;
       UWORD filename_length;
       UWORD extra_field_length;
   } local_file_header;

   typedef struct central_directory_file_header {     /* CENTRAL */
       byte version_made_by[2];
       byte version_needed_to_extract[2];
       UWORD general_purpose_bit_flag;
       UWORD compression_method;
       UWORD last_mod_file_time;
       UWORD last_mod_file_date;
       ULONG crc32;
       ULONG compressed_size;
       ULONG uncompressed_size;
       UWORD filename_length;
       UWORD extra_field_length;
       UWORD file_comment_length;
       UWORD disk_number_start;
       UWORD internal_file_attributes;
       ULONG external_file_attributes;
       ULONG relative_offset_local_header;
   } central_directory_file_header;

   typedef struct end_central_dir_record {            /* END CENTRAL */
       UWORD number_this_disk;
       UWORD num_disk_with_start_central_dir;
       UWORD num_entries_centrl_dir_ths_disk;
       UWORD total_entries_central_dir;
       ULONG size_central_directory;
       ULONG offset_start_central_directory;
       UWORD zipfile_comment_length;
   } end_central_dir_record;


#else   /* !NOTINT16:  read data directly into the structure we'll be using */


   typedef struct local_file_header {                 /* LOCAL */
       byte version_needed_to_extract[2];
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

   typedef struct central_directory_file_header {     /* CENTRAL */
       byte version_made_by[2];
       byte version_needed_to_extract[2];
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

   typedef struct end_central_dir_record {            /* END CENTRAL */
       byte _sig_[4];  /* space-holder only */
       UWORD number_this_disk;
       UWORD num_disk_with_start_central_dir;
       UWORD num_entries_centrl_dir_ths_disk;
       UWORD total_entries_central_dir;
       byte size_central_directory[4];
       byte offset_start_central_directory[4];
       UWORD zipfile_comment_length;
   } end_central_dir_record;

#endif  /* !NOTINT16 */





/*************************/
/*  Function Prototypes  */
/*************************/

#ifndef __              /* This amusing little construct was swiped without  */
#  ifdef __STDC__       /*  permission from the fine folks at Cray Research, */
#    define __(X)   X   /*  Inc.  Should probably give them a call and see   */
#  else                 /*  if they mind, but....  Then again, I can't think */
#    define __(X)   ()  /*  of any other way to do this, so maybe it's an    */
#  endif                /*  algorithm?  Whatever, thanks to CRI.  (Note:     */
#endif                  /*  keep interior stuff parenthesized.)              */
/*
 * Toad Hall Note:  Not to worry:  I've seen this somewhere else too,
 * so obviously it's been stolen more than once.
 * That makes it public domain, right?
 */

/*---------------------------------------------------------------------------
    Functions in nunzip.c:
  ---------------------------------------------------------------------------*/

void   usage                         __( (void) );
int    process_zipfile               __( (void) );
int    find_end_central_dir          __( (void) );
int    process_end_central_dir       __( (void) );
int    list_files                    __( (void) );
int    extract_or_test_files         __( (void) );
int    extract_or_test_member        __( (void) );
int    process_central_file_header   __( (void) );
int    process_local_file_header     __( (void) );

/*---------------------------------------------------------------------------
    Functions in file_io.c:
  ---------------------------------------------------------------------------*/

int    change_zipfile_attributes __( (int zip_error) );
int    open_input_file           __( (void) );
int    readbuf                   __( (char *buf, register unsigned size) );
int    create_output_file        __( (void) );
int    FillBitBuffer             __( (register int bits) );
int    ReadByte                  __( (UWORD *x) );
int    FlushOutput               __( (void) );
/*
 * static int   WriteBuffer       __( (int fd, unsigned char *buf, int len) );
 * static int   dos2unix          __( (unsigned char *buf, int len) );
 */
void   set_file_time_and_close   __( (void) );

/*---------------------------------------------------------------------------
    Macintosh file_io functions:
  ---------------------------------------------------------------------------*/

#ifdef MACOS
void   macfstest                 __( (int vrefnum, int wd) );
/*
 * static int   IsHFSDisk        __( (int wAppVRefNum) );
 */
int    mkdir                     __( (char *path, int mode) );
void   SetMacVol                 __( (char *pch, short wVRefNum) );
#endif

/*---------------------------------------------------------------------------
    Uncompression functions (all internal compression routines, enclosed in
    comments below, are prototyped in their respective files and are invisi-
    ble to external functions):
  ---------------------------------------------------------------------------*/

void   unImplode                __( (void) );                  /* unimplod.c */
/*
 * static void   ReadLengths     __( (sf_tree *tree) );
 * static void   SortLengths     __( (sf_tree *tree) );
 * static void   GenerateTrees   __( (sf_tree *tree, sf_node *nodes) );
 * static void   LoadTree        __( (sf_tree *tree, int treesize, sf_node *nodes) );
 * static void   LoadTrees       __( (void) );
 * static void   ReadTree        __( (register sf_node *nodes, int *dest) );
 */

void   unReduce                 __( (void) );                  /* unreduce.c */
/*
 * static void   LoadFollowers   __( (void) );
 */

void   unShrink                 __( (void) );                  /* unshrink.c */
/*
 * static void   partial_clear   __( (void) );
 */

/*---------------------------------------------------------------------------
    Functions in match.c, mapname.c, and misc.c:
  ---------------------------------------------------------------------------*/

int       match         __( (char *string, char *pattern) );      /* match.c */
/*
 * static BOOLEAN   do_list      __( (register char *string, char *pattern) );
 * static void      list_parse   __( (char **patp, char *lowp, char *highp) );
 * static char      nextch       __( (char **patp) );
 */

int       mapped_name   __( (void) );                           /* mapname.c */

void      UpdateCRC     __( (register unsigned char *s, register int len) );
int       do_string     __( (unsigned int len, int option) );      /* misc.c */
UWORD     makeword      __( (byte *b) );                           /* misc.c */
ULONG     makelong      __( (byte *sig) );                         /* misc.c */
void      return_VMS    __( (int zip_error) );                     /* misc.c */
#ifdef ZMEM
   char   *memset       __( (register char *buf, register char init, register unsigned int len) );
   char   *memcpy       __( (register char *dst, register char *src, register unsigned int len) );
#endif      /* These guys MUST be ifdef'd because their definition  */
            /*  conflicts with the standard one.  Others (makeword, */
            /*  makelong, return_VMS) don't matter.                 */





/************/
/*  Macros  */
/************/

#ifndef min    /* MSC defines this in stdlib.h */
#  define min(a,b)   ((a) < (b) ? (a) : (b))
#endif


#define LSEEK(abs_offset) {longint request=(abs_offset), inbuf_offset=request%INBUFSIZ, bufstart=request-inbuf_offset;\
   if(bufstart!=cur_zipfile_bufstart) {cur_zipfile_bufstart=lseek(zipfd,bufstart,SEEK_SET);\
   if((incnt=read(zipfd,inbuf,INBUFSIZ))<=0) return(51); inptr=inbuf+inbuf_offset; incnt-=inbuf_offset;\
   }else {incnt+=(inptr-inbuf)-inbuf_offset; inptr=inbuf+inbuf_offset; }}

/*
 *  Seek to the block boundary of the block which includes abs_offset,
 *  then read block into input buffer and set pointers appropriately.
 *  If block is already in the buffer, just set the pointers.  This macro
 *  is used by process_end_central_dir (unzip.c) and do_string (misc.c).
 *  A slightly modified version is embedded within extract_or_test_files
 *  (unzip.c).  ReadByte and readbuf (file_io.c) are compatible.
 *
 *  macro LSEEK( abs_offset )
 *    {
 *      longint   request = abs_offset;
 *      longint   inbuf_offset = request % INBUFSIZ;
 *      longint   bufstart = request - inbuf_offset;
 *
 *      if (bufstart != cur_zipfile_bufstart) {
 *          cur_zipfile_bufstart = lseek(zipfd, bufstart, SEEK_SET);
 *          if ((incnt = read(zipfd,inbuf,INBUFSIZ)) <= 0)
 *              return(51);
 *          inptr = inbuf + inbuf_offset;
 *          incnt -= inbuf_offset;
 *      } else {
 *          incnt += (inptr-inbuf) - inbuf_offset;
 *          inptr = inbuf + inbuf_offset;
 *      }
 *    }
 *
 */


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


#define NUKE_CRs(buf,len) {register int i,j; for (i=j=0; j<len; (buf)[i++]=(buf)[j++]) if ((buf)[j]=='\r') ++j; len=i;}

/*
 *  Remove all the ASCII carriage returns from buffer buf (length len),
 *  shortening as necessary (note that len gets modified in the process,
 *  so it CANNOT be an expression).  This macro is intended to be used
 *  BEFORE A_TO_N(); hence the check for CR instead of '\r'.  NOTE:  The
 *  if-test gets performed one time too many, but it doesn't matter.
 *
 *  macro NUKE_CRs( buf, len )
 *    {
 *      register int   i, j;
 *
 *      for ( i = j = 0  ;  j < len  ;  (buf)[i++] = (buf)[j++] )
 *        if ( (buf)[j] == CR )
 *          ++j;
 *      len = i;
 *    }
 *
 */


#define TOLOWER(str1,str2) {char *ps1,*ps2; ps1=(str1)-1; ps2=(str2); while(*++ps1) *ps2++=(isupper(*ps1))?tolower(*ps1):*ps1; *ps2='\0';}

/*
 *  Copy the zero-terminated string in str1 into str2, converting any
 *  uppercase letters to lowercase as we go.  str2 gets zero-terminated
 *  as well, of course.  str1 and str2 may be the same character array.
 *
 *  macro TOLOWER( str1, str2 )
 *    {
 *      register char   *ps1, *ps2;
 *
 *      ps1 = (str1) - 1;
 *      ps2 = (str2);
 *      while ( *++ps1 )
 *        *ps2++ = (isupper(*ps1)) ?  tolower(*ps1)  :  *ps1;
 *      *ps2='\0';
 *    }
 *
 *  NOTES:  This macro makes no assumptions about the characteristics of
 *    the tolower() function or macro (beyond its existence), nor does it
 *    make assumptions about the structure of the character set (i.e., it
 *    should work on EBCDIC machines, too).  The fact that either or both
 *    of isupper() and tolower() may be macros has been taken into account;
 *    watch out for "side effects" (in the C sense) when modifying this
 *    macro.
 */


#ifndef ascii_to_native

#  define ascii_to_native(c)   (c)
#  define A_TO_N(str1)

#else

#  define NATIVE   /* Used in main() for '-a' and '-c'. */
#  define A_TO_N(str1) { register unsigned char *ps1; for (ps1 = str1; *ps1; ps1++) *ps1 = (ascii_to_native(*ps1)); }

/*
 *   Translate the zero-terminated string in str1 from ASCII to the native
 *   character set. The translation is performed in-place and uses the
 *   ascii_to_native macro to translate each character.
 *
 *   macro A_TO_N( str1 )
 *     {
 *	 register unsigned char *ps1;
 *
 *	 for ( ps1 = str1; *ps1; ps1++ )
 *	   *ps1 = ( ascii_to_native( *ps1 ) );
 *     }
 *
 *   NOTE: Using the ascii_to_native macro means that is it the only part of
 *     unzip which knows which translation table (if any) is actually in use
 *     to produce the native character set. This makes adding new character
 *     set translation tables easy insofar as all that is needed is an
 *     appropriate ascii_to_native macro definition and the translation
 *     table itself. Currently, the only non-ASCII native character set
 *     implemented is EBCDIC but this may not always be so.
 */

#endif





/*************/
/*  Globals  */
/*************/

   extern int       tflag;
/* extern int       vflag;    (only used in unzip.c)  */
   extern int       cflag;
   extern int       aflag;
   extern int       dflag;
   extern int       Uflag;
   extern int       V_flag;
#ifdef MACOS
   extern int       hfsflag;
#endif
   extern int       lcflag;
   extern unsigned  f_attr;
   extern longint   csize;
   extern longint   ucsize;

   extern short     prefix_of[];
#ifdef MACOS
   extern byte      *suffix_of;
   extern byte      *stack;
#else
   extern byte      suffix_of[];
   extern byte      stack[];
#endif
   extern ULONG     crc32val;
   extern UWORD     mask_bits[];

   extern byte      *inbuf;
   extern byte      *inptr;
   extern int       incnt;
   extern UWORD     bitbuf;
   extern int       bits_left;
   extern boolean   zipeof;
   extern int       zipfd;
   extern char      zipfn[];
   extern local_file_header   lrec;
   extern struct stat         statbuf;
   extern longint   cur_zipfile_bufstart;

   extern byte      *outbuf;
   extern byte      *outptr;
   extern byte      *outout;
   extern longint   outpos;
   extern int       outcnt;
   extern int       outfd;
   extern char      filename[];

#ifdef DECLARE_ERRNO
   extern int       errno;
#endif

#ifdef EBCDIC
   extern byte      ebcdic[];
#endif
