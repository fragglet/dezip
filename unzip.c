/*---------------------------------------------------------------------------

  unzip.c

  This is nearly a complete rewrite of unzip.c, mainly to allow access to
  zipfiles via the central directory (and hence to the OS bytes, so we can
  make intelligent decisions about what to do with the extracted files).
  Based on unzip.c 3.15+ and zipinfo.c 0.90.

  ---------------------------------------------------------------------------

  To compile (partial instructions):

     under Unix (cc):  make <system name>
       (type "make" for list of valid names, or read Makefile for details)

         under MS-DOS (MSC):  make MAKEFILE.DOS
       (or use Makefile if you have MSC 6.0:  "nmake msc_dos")

         under MS-DOS (TurboC):  make -fMAKEFILE.DOS  for command line compiles
       (or use the integrated environment and the included files TCCONFIG.TC
        and UNZIP.PRJ.  Tweak for your environment.)

     under OS/2 (MSC):  cl [etc.] -DOS2 unzip.c, etc.
           (use MAKEFILE.DOS, or add an "msc_os2" target to Makefile, if you
        have MSC 6.0 and NMAKE)

     under Atari OS:  Any Day Now.

     under VMS:  DEFINE LNK$LIBRARY SYS$LIBRARY:VAXCRTL.OLB   (see VMSNOTES)
                 CC UNZIP,FILE_IO,MAPNAME,MATCH,...,UNSHRINK
                 LINK UNZIP,FILE_IO,MAPNAME,MATCH,...,UNSHRINK
                 UNZIP :== $DISKNAME:[DIRECTORY]UNZIP.EXE

     under Macintosh OS:   <slide> <click> <slide> <shuffle> <click>  :)

  ---------------------------------------------------------------------------

     Version:  unzip401.arc/unzip401.tar.Z for Unix, VMS, OS/2 and MS-DOS
     Source:   wsmr-simtel20.army.mil (26.2.0.74) in pd3:<misc.unix>

  ---------------------------------------------------------------------------

  Copyright, originally from version 1.2 (?):

     * Copyright 1989 Samuel H. Smith;  All rights reserved
     *
     * Do not distribute modified versions without my permission.
     * Do not remove or alter this notice or any other copyright notice.
     * If you use this in your own program you must distribute source code.
     * Do not use any of this in a commercial product.

  ---------------------------------------------------------------------------

  Comments from unzip.c, version 3.11:

     * UnZip - A simple zipfile extract utility
     *
     * Compile-time definitions:
     * See the Makefile for details, explanations, and all the current
     * system makerules.
     *
     * If you have to add a new one for YOUR system, please forward the
     * new Makefile context diff to kirsch@usasoc.soc.mil for distribution.
     * Be SURE to give FULL details on your system (hardware, OS, versions,
     * processor, whatever) that made it unique.
     *
     * REVISION HISTORY : See history.400 (or whatever current version is)

  To join Info-ZIP, send a message to Info-ZIP-Request@WSMR-Simtel20.Army.Mil

  ---------------------------------------------------------------------------*/





#include "unzip.h"              /* includes, defines, and macros */

#define VERSION  "v4.01 of 12-04-90"    /* keep this one #define here, so
                                         *  don't have to recompile every-
                                         *  body for each version change */




/**********************/
/*  Global Variables */
/**********************/

int tflag;              /* -t: test */
int vflag;              /* -v: view directory (only used in unzip.c) */
int cflag;              /* -c: output to stdout */
int aflag;              /* -a: do ascii to ebcdic translation OR CR-LF */
                        /*     to LF conversion of extracted files */
int dflag;              /* -d: create containing directories */
int Uflag;              /* -U: leave filenames in upper or mixed case */
int quietflg;           /* -q: produce a lot less output */
int lcflag;             /* convert filename to lowercase */

longint csize;          /* used by list_files(), ReadByte(): must be signed */
longint ucsize;         /* used by list_files(), unReduce(), unImplode() */

/*---------------------------------------------------------------------------
    unShrink/unReduce/unImplode working storage:
  ---------------------------------------------------------------------------*/

/* prefix_of (for unShrink) is biggest storage area, esp. on Crays...space */
/*  is shared by lit_nodes (unImplode) and followers (unReduce) */

short prefix_of[HSIZE + 1];     /* (8193 * sizeof(short)) */
byte suffix_of[HSIZE + 1];      /* also s-f length_nodes (smaller) */
byte stack[HSIZE + 1];          /* also s-f distance_nodes (smaller) */

ULONG crc32val;

UWORD mask_bits[] =
{0, 0x0001, 0x0003, 0x0007, 0x000f,
 0x001f, 0x003f, 0x007f, 0x00ff,
 0x01ff, 0x03ff, 0x07ff, 0x0fff,
 0x1fff, 0x3fff, 0x7fff, 0xffff};

/*---------------------------------------------------------------------------
    Input file variables:
  ---------------------------------------------------------------------------*/

byte *inbuf, *inptr;    /* input buffer (any size is legal) and pointer */
int incnt;

UWORD bitbuf;
int bits_left;
boolean zipeof;

int zipfd;                      /* zipfile file handle */
char zipfn[STRSIZ];

local_file_header lrec;
struct stat statbuf;            /* used by main(), mapped_name() */

longint cur_zipfile_fileptr;    /* used by readbuf(), ReadByte(), */
longint cur_zipfile_bufstart;   /*  and extract_or_test_files() */

/*---------------------------------------------------------------------------
    Output stream variables:
  ---------------------------------------------------------------------------*/

byte *outbuf;                   /* buffer for rle look-back */
byte *outptr;
byte *outout;                   /* scratch pad for ASCII-native trans */
longint outpos;                 /* absolute position in outfile */
int outcnt;                     /* current position in outbuf */

int outfd;
char filename[STRSIZ];

/*---------------------------------------------------------------------------
    unzip.c static global variables (visible only within this file):
  ---------------------------------------------------------------------------*/

static char *fnames[2] =
{"*", NULL};                    /* default filenames vector */
static char **fnv = &fnames[0];
static char sig[5];
static byte *hold;
static int process_all_files;
static int block_offset;
static longint zipfile_length;
static longint bytes_remaining;
static longint end_cent_offset;
static UWORD hostnum;
static UWORD methnum;
/* static UWORD     extnum; */

static central_directory_file_header crec;
static end_central_dir_record ecrec;

/* (original) Bill Davidsen version */
#define QCOND    (!quietflg)    /* -xq[q] kill "extracting: ..." msgs */
#define QCOND2   (which_hdr)    /* file comments with -v, -vq, -vqq */

/* We do this a LOT */
static char *errmsg = "        (please report to info-zip@wsmr-simtel20.army.mil)\n";




/******************/
/*  Main program */
/******************/

main(argc, argv)        /* return PK-type error code (except under VMS) */
int argc;
char *argv[];
{
    char *s;
    int c;



/*---------------------------------------------------------------------------
    Debugging info for checking on structure padding:
  ---------------------------------------------------------------------------*/

#ifdef DEBUG_STRUC
    printf("local_file_header size: %X\n",
           sizeof(struct local_file_header));
    printf("local_byte_header size: %X\n",
           sizeof(struct local_byte_header));
    printf("actual size of local headers: %X\n", LREC_SIZE);

    printf("central directory header size: %X\n",
           sizeof(struct central_directory_file_header));
    printf("central directory byte header size: %X\n",
           sizeof(struct central_directory_byte_header));
    printf("actual size of central dir headers: %X\n", CREC_SIZE);

    printf("end central dir record size: %X\n",
           sizeof(struct end_central_dir_record));
    printf("end central dir byte record size: %X\n",
           sizeof(struct end_central_byte_record));
    printf("actual size of end-central-dir record: %X\n", ECREC_SIZE);
#endif

/*---------------------------------------------------------------------------
    Rip through any command-line options lurking about...
  ---------------------------------------------------------------------------*/

    while (--argc > 0 && (*++argv)[0] == '-') {
        s = argv[0] + 1;
        while ((c = *s++) != 0) {       /* "!= 0":  prevent Turbo C warning */
            switch (c) {
            case ('x'): /* just ignore -x, -e options (extract) */
            case ('e'):
                break;
            case ('q'):
                ++quietflg;
                break;
            case ('t'):
                ++tflag;
                break;
            case ('v'):
                ++vflag;
                /* fall thru */
            case ('l'):
                ++vflag;
                break;
            case ('p'):
                ++cflag;
#ifdef NATIVE
                ++aflag;
#endif
                quietflg += 2;
                break;
            case ('c'):
                ++cflag;
#ifdef NATIVE
                ++aflag;        /* this is so you can read it on the screen */
#endif
                break;
            case ('a'):
                ++aflag;
                break;
#ifdef VMS
            case ('u'): /* switches converted to l.c. unless quoted */
#endif
            case ('U'): /* "uppercase flag" (i.e., don't convert) */
                ++Uflag;
                break;
            case ('d'): /* create parent dirs flag */
                ++dflag;
                break;
            default:
                usage();
                break;
            }
        }
    }

/*---------------------------------------------------------------------------
    Make sure we aren't trying to do too many things here.  [This seems like
    kind of a brute-force way to do things; but aside from that, isn't the
    -a option useful when listing the directory (i.e., for reading zipfile
    comments)?  It's a modifier, not an action in and of itself, so perhaps
    it should not be included in the test--certainly, in the case of zipfile
    testing, it can just be ignored.]
  ---------------------------------------------------------------------------*/

    if ((tflag && vflag) || (tflag && cflag) || (vflag && cflag) ||
        (tflag && aflag) || (aflag && vflag)) {
        fprintf(stderr, "only one of -t, -c, -a, or -v\n");
        RETURN(10);             /* 10:  bad or illegal parameters specified */
    }
    if (argc-- == 0) {
        usage();
        RETURN(10);             /* 10:  bad or illegal parameters specified */
    }
/*---------------------------------------------------------------------------
    Now get the zipfile name from the command line and see if it exists as a
    regular (non-directory) file.  If not, append the ".zip" suffix.  We don't
    immediately check to see if this results in a good name, but we will do so
    later.  In the meantime, see if there are any member filespecs on the com-
    mand line, and if so, set the filename pointer to point at them.
  ---------------------------------------------------------------------------*/

    strcpy(zipfn, *argv++);
    if (stat(zipfn, &statbuf) || (statbuf.st_mode & S_IFMT) == S_IFDIR)
        strcat(zipfn, ZSUFX);

    if (stat(zipfn, &statbuf)) {/* try again */
        fprintf(stderr, "error:  can't find zipfile [ %s ]\n", zipfn);
        RETURN(9);              /* 9:  file not found */
    } else
        zipfile_length = statbuf.st_size;

    if (argc != 0) {
        fnv = argv;
        process_all_files = FALSE;
    } else
        process_all_files = TRUE;       /* for speed */

/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

    inbuf = (byte *) (malloc(INBUFSIZ + 4));    /* 4 extra for hold[] (below) */
    outbuf = (byte *) (malloc(OUTBUFSIZ + 1));  /* 1 extra for string termin. */
    if (aflag)                  /* if need an ascebc scratch, */
        outout = (byte *) (malloc(OUTBUFSIZ));  /*  allocate it... */
    else                        /*  else just point to outbuf */
        outout = outbuf;

    if ((inbuf == NULL) || (outbuf == NULL) || (outout == NULL)) {
        fprintf(stderr, "error:  can't allocate unzip buffers\n");
        RETURN(4);              /* 4-8:  insufficient memory */
    }
    hold = &inbuf[INBUFSIZ];    /* to check for boundary-spanning signatures */

    RETURN(process_zipfile());  /* keep passing errors back... */

}       /* end main() */





/**********************/
/*  Function usage() */
/**********************/

void usage()
{
#ifdef NATIVE
    char *astring = "-a     convert ASCII to native character set";
#else
    char *astring = "-a     convert to Unix/VMS textfile format (CR LF => LF)";
#endif


    fprintf(stderr, "UnZip:  Zipfile Extract %s;  (C) 1989 Samuel H. Smith\n",
                 VERSION);
    fprintf(stderr, "Courtesy of:  S.H.Smith  and  The Tool Shop BBS,  \
(602) 279-2673.\n\n");
    fprintf(stderr, "Versions 3.0 and later brought to you by the fine folks \
at Info-ZIP\n");
    fprintf(stderr, "(Info-ZIP@WSMR-Simtel20.Army.Mil)\n\n");
    fprintf(stderr, "Usage:  unzip [ -xecptlv[qadU] ] file[.zip] [filespec...]\n\
  -x,-e  extract files in archive (default--i.e., this flag is optional)\n\
  -c     extract files to stdout (\"CRT\")\n\
  -p     extract files to stdout and no informational messages (for pipes)\n\
  -t     test files\n\
  -l     list files (short format)\n\
  -v     verbose listing of files\n");
    fprintf(stderr, "\
  -q     perform operations quietly (up to two q's allowed)\n\
  %s\n\
  -d     recreate directory structure contained in archive\n\
  -U     don't map filenames to lowercase for selected (uppercase) OS's\n",
            astring);

#ifdef VMS
    fprintf(stderr, "Make sure the VMS file mode is \"stream-LF\":   \
ATTR /RTYPE=STREAMLF file.zip\n\
Also, remember that non-lowercase filespecs must be quoted \
(e.g., \"Makefile\").\n");
#endif

}       /* end function usage() */





/********************************/
/*  Function process_zipfile() */
/********************************/

int process_zipfile()
{                               /* return PK-type error code */
    int error, error_in_archive;



/*---------------------------------------------------------------------------
    Open the zipfile for reading and in BINARY mode to prevent CR/LF trans-
    lation, which would corrupt the bitstreams.  Then find and process the
    central directory; list, extract or test member files as instructed; and
    close the zipfile.
  ---------------------------------------------------------------------------*/

    if (open_input_file())      /* this should never happen, given the */
        return (9);             /*   stat() test in main(), but... */

    if (find_end_central_dir()) /* not found; nothing to do */
        return (2);             /* 2:  error in zipfile */

    if ((error_in_archive = process_end_central_dir()) > 1)
        return (error_in_archive);

    if (ecrec.number_this_disk == 0) {
        if (vflag)
            error = list_files();       /* LIST 'EM */
        else
            error = extract_or_test_files();    /* EXTRACT OR TEST 'EM */
        if (error > error_in_archive)   /* don't overwrite stronger error */
            error_in_archive = error;   /*  with (for example) a warning */
    } else {
        fprintf(stderr, "\nerror:  zipfile is part of multi-disk archive \
(sorry, no can do).\n");
        fprintf(stderr, errmsg);
/* "        (please report to info-zip@wsmr-simtel20.army.mil)\n" */
        error_in_archive = 11;  /* 11:  no files found */
    }

    close(zipfd);
    return (error_in_archive);

}       /* end function process_zipfile() */





/*************************************/
/*  Function find_end_central_dir() */
/*************************************/

int find_end_central_dir()
{                               /* return 0 if found, 1 otherwise */

/*---------------------------------------------------------------------------
    Initialize file pointer and variables, then find the signature bytes.
  ---------------------------------------------------------------------------*/

    lseek(zipfd, 0L, SEEK_END); /* file pointer -> EOF */
    bytes_remaining = zipfile_length;

    if (scan_back()) {          /* scanned whole file but didn't find signature */
        printf("\nFile:  %s\n", zipfn);
        printf("\n  End-of-central-directory signature not found.  \
Either this file is not\n");
        printf("  a zipfile, or it constitutes one disk of a multi-part \
archive.  In the\n");
        printf("  latter case the central directory and zipfile comment \
will be found on\n");
        printf("  the last disk(s) of this archive.\n");
        return (1);             /* failed */
    }
/*---------------------------------------------------------------------------
    Calculate the address of the end-of-central-directory signature, and print
    it out, if debugging.  Then, since we already know the signature bytes are
    there, point at the beginning of the actual record instead of the sig.
  ---------------------------------------------------------------------------*/

    end_cent_offset = bytes_remaining + block_offset;

#ifdef TEST
    printf("\n  found end-of-central-dir signature at offset %ld (%.8lXh)\n",
           end_cent_offset, end_cent_offset);
    printf("    from beginning of file; offset %d (%.4Xh) within block\n",
           block_offset, block_offset);
#endif

    end_cent_offset += 4L;
    return (0);

}       /* end function find_end_central_dir() */





/**************************/
/*  Function scan_back() */
/**************************/

int scan_back()
{                               /* return 0 if found sig, 1 if not */
/*  function is only called once, so static variables are OK: */
    static int which_block = END, is_first_block = TRUE;
    int block_length, end_block;



/*---------------------------------------------------------------------------
    This routine, like readbuf(), does buffered reads from the input zipfile,
    but the similarity pretty much ends there.  scan_back() reads blocks be-
    ginning at the end of the file and working toward the beginning, and it
    also searches the read-in data (backwards, of course) for the signature
    bytes which mark the end-of-central-directory record.  The structure is
    simple:  loop through the file (backwards) until the signature is found.
    Within the loop are two pieces:  read a block of data, then scan it for
    the signature bytes.

    First, try to read INBUFSIZ bytes into buffer.
  ---------------------------------------------------------------------------*/

    while (which_block != START) {
        if (bytes_remaining > INBUFSIZ) {
            lseek(zipfd, (long) -INBUFSIZ, SEEK_CUR);
            if ((block_length = read(zipfd, inbuf, INBUFSIZ)) <= 0)
                return (1);     /* failed */
            lseek(zipfd, (long) -INBUFSIZ, SEEK_CUR);   /* reset to block start */
            bytes_remaining -= block_length;

#ifdef TEST
            printf("  read a regular block of %d bytes; bytes remaining:  %ld\n",
                   block_length, bytes_remaining);
#endif
        }
        /*-------------------------------------------------------------------
            Less than or equal to INBUFSIZ left:  must be beginning of file.
          -------------------------------------------------------------------*/

        else {                  /* bytes_remaining <= INBUFSIZ */
            lseek(zipfd, 0L, SEEK_SET);
            if ((block_length = read(zipfd, inbuf,
                                (unsigned int) bytes_remaining)) <= 0)
                return (1);     /* failed */
            lseek(zipfd, 0L, SEEK_SET); /* reset to start of block */
            which_block = START;
            bytes_remaining -= block_length;

#ifdef TEST
            printf("  read a short block of %d bytes; bytes remaining:  %ld\n",
                   block_length, bytes_remaining);
#endif
        }       /* end if (bytes_remaining > INBUFSIZ) */

        /*-------------------------------------------------------------------
            Loop through buffer until find a 'P'; then check next three bytes
            for a match with signature string.  end_block is the index of the
            last byte in the current block, not the position just beyond that.
          -------------------------------------------------------------------*/

        if (is_first_block) {   /* first block, so hold[] is empty */
            is_first_block = FALSE;     /*   (no boundary to cross) */
            end_block = block_length - 4;
            if (which_block == END)     /* might equal START... */
                which_block = MIDDLE;
        } else {                        /* not first block: can use hold[]
                                         * to pad end of block */
            strncpy(&inbuf[block_length], hold, 3);     /* first three bytes */
            end_block = block_length - 1;               /* of previous block */
        }

        for (block_offset = end_block; block_offset >= 0; --block_offset)
            if ((ascii_to_native(inbuf[block_offset]) == 'P') &&
                !strncmp(&inbuf[block_offset], END_CENTRAL_SIG, 4))
                return (0);     /* found it! */

        strncpy(hold, inbuf, 3);/* not found, so save first three bytes */

    }                           /* end while (which_block != START) */

/*---------------------------------------------------------------------------
    Searched through whole file without success:  return in shame.
  ---------------------------------------------------------------------------*/

    return (1);                 /* failed */

}       /* end function scan_back() */





/****************************************/
/*  Function process_end_central_dir() */
/****************************************/

int process_end_central_dir()
{                               /* return PK-type error code */
#ifdef NOTINT16
    end_central_byte_record byterec;
#endif



/*---------------------------------------------------------------------------
    Read the end-of-central-directory record and do any necessary machine-
    type conversions (byte ordering, structure padding compensation).  This
    is the first use of readbuf(), so make sure things are appropriately ini-
    tialized...
  ---------------------------------------------------------------------------*/

    lseek(zipfd, end_cent_offset, SEEK_SET);
    incnt = 0;

#ifndef NOTINT16
    if (readbuf(zipfd, (char *) &ecrec, ECREC_SIZE) <= 0)
        return (51);            /* 51:  unexpected EOF */

#else                           /* NOTINT16:  read data into character array,
                                 * then copy to struct */
    if (readbuf(zipfd, byterec, ECREC_SIZE) <= 0)
        return (51);

    ecrec.number_this_disk =
        makeword(&byterec[NUMBER_THIS_DISK]);
    ecrec.num_disk_with_start_central_dir =
        makeword(&byterec[NUM_DISK_WITH_START_CENTRAL_DIR]);
    ecrec.num_entries_centrl_dir_ths_disk =
        makeword(&byterec[NUM_ENTRIES_CENTRL_DIR_THS_DISK]);
    ecrec.total_entries_central_dir =
        makeword(&byterec[TOTAL_ENTRIES_CENTRAL_DIR]);
    ecrec.size_central_directory =
        makelong(&byterec[SIZE_CENTRAL_DIRECTORY]);
    ecrec.offset_start_central_directory =
        makelong(&byterec[OFFSET_START_CENTRAL_DIRECTORY]);
    ecrec.zipfile_comment_length =
        makeword(&byterec[ZIPFILE_COMMENT_LENGTH]);
#endif                          /* NOTINT16 */

/*---------------------------------------------------------------------------
    Get the zipfile comment, if any, and print it out.  (Comment may be up
    to 64KB long.  May the fleas of a thousand camels infest the armpits of
    anyone who actually takes advantage of this fact.)
  ---------------------------------------------------------------------------*/

    if (ecrec.zipfile_comment_length && !quietflg) {
        printf("[%s] comment:  ", zipfn);
        if (do_string(ecrec.zipfile_comment_length, DISPLAY)) {
            fprintf(stderr, "\nwarning:  zipfile comment truncated\n");
            return (1);         /* 1:  warning error */
        }
        printf("\n");
    }
    return (0);                 /* 0:  no error */

}       /* end function process_end_central_dir() */





/***************************/
/*  Function list_files() */
/***************************/

int list_files()
{                               /* return PK-type error code */
    char **fnamev;
    int do_this_file = FALSE, ratio, error, error_in_archive = 0;
    int which_hdr = (vflag > 1);
    UWORD j, yr, mo, dy, hh, mm, members = 0;
    ULONG tot_csize = 0L, tot_ucsize = 0L;
    static char *method[NUM_METHODS + 1] =
    {"Stored", "Shrunk", "Reduce1",
     "Reduce2", "Reduce3", "Reduce4", "Implode", "Unknown"};
    static char *Headers[][2] =
    {
        {" Length    Date    Time   Name",
         " ------    ----    ----   ----"},
        {" Length  Method   Size  Ratio   Date    Time   CRC-32    Name",
       " ------  ------   ----  -----   ----    ----   ------    ----"}};



/*---------------------------------------------------------------------------
    Unlike extract_or_test_files(), this routine confines itself to the cen-
    tral directory.  Thus its structure is somewhat simpler, since we can do
    just a single loop through the entire directory, listing files as we go.

    So to start off, set file pointer to start of central directory, make
    sure readbuf() is reset, and print the heading line.  (Don't actually
    need cur_zipfile_fileptr in this routine, but may as well set it anyway.)
    Then start main loop through central directory.  The results will look
    vaguely like the following:

     Length  Method   Size  Ratio   Date    Time   CRC-32    Name
     ------  ------   ----  -----   ----    ----   ------    ----
      44004  Implode  13041  71%  11-02-89  19:34  8b4207f7  Makefile.UNIX
       3438  Shrunk    2209  36%  09-15-90  14:07  a2394fd8  dos-file.ext
  ---------------------------------------------------------------------------*/

    cur_zipfile_fileptr =
        lseek(zipfd, (longint) ULONG_(ecrec.offset_start_central_directory),
                SEEK_SET);
    incnt = 0;                  /* readbuf() has just been reset */

    if (quietflg < 2)
        printf("%s\n%s\n", Headers[which_hdr][0], Headers[which_hdr][1]);

    for (j = 0; j < ecrec.total_entries_central_dir; ++j) {

        if (readbuf(zipfd, sig, 4) <= 0)
            return (51);        /* 51:  unexpected EOF */
        if (strncmp(sig, CENTRAL_HDR_SIG, 4)) { /* just to make sure */
            fprintf(stderr, "\nerror:  expected central file header signature \
not found (file #%u).\n", j);
            fprintf(stderr, errmsg);
/* "        (please report to info-zip@wsmr-simtel20.army.mil)\n" */
            return (3);         /* 3:  error in zipfile */
        }
        if ((error = process_central_file_header()) != 0)  /* (sets lcflag) */
            return (error);     /* only 51 (EOF) defined */

        /*---------------------------------------------------------------------
         We could DISPLAY the filename instead of storing (and possibly trun-
         cating, in the case of a very long name) and printing it, but that
         has the disadvantage of not allowing case conversion--and it's nice
         to be able to see in the listing precisely how you have to type each
         filename in order for unzip to consider it a match.  Speaking of
         which, if member names were specified on the command line, check in
         with match() to see if the current file is one of them, and make a
         note of it if it is.
        --------------------------------------------------------------------*/

        /*   v----- (uses lcflag) */
        if ((error = do_string(crec.filename_length, FILENAME)) != 0) {
            error_in_archive = error;   /* might be just warning */
            if (error > 1)      /* fatal:  can't continue */
                return (error);
        }
        if ((error = do_string(crec.extra_field_length, SKIP)) != 0) {
            error_in_archive = error;   /* might be just warning */
            if (error > 1)      /* fatal */
                return (error);
        }
        if (!process_all_files) {       /* check if specified on command line */
            do_this_file = FALSE;
            fnamev = fnv;       /* don't destroy permanent filename ptr */
            for (--fnamev; *++fnamev;)
                if (match(filename, *fnamev)) {
                    do_this_file = TRUE;
                    break;      /* found match, so stop looping */
                }
        }
        /*---------------------------------------------------------------------
         If current file was specified on command line, or if no names were
         specified, do the listing for this file.  Otherwise, get rid of the
         file comment and go back for the next file.
         -------------------------------------------------------------------*/

        if (process_all_files || do_this_file) {

            yr = ((crec.last_mod_file_date >> 9) & 0x7f) + 80;
            mo = (crec.last_mod_file_date >> 5) & 0x0f;
            dy = crec.last_mod_file_date & 0x1f;
            hh = (crec.last_mod_file_time >> 11) & 0x1f;
            mm = (crec.last_mod_file_time >> 5) & 0x3f;

            csize = (longint) ULONG_(crec.compressed_size);
            ucsize = (longint) ULONG_(crec.uncompressed_size);

            ratio = (ucsize == 0) ?     /* can .zip contain 0-size file? */
                0 : (int) ((1000L * (ucsize - csize)) / ucsize) + 5;

            switch (which_hdr) {
            case 0:             /* short form */
                printf("%7ld  %02u-%02u-%02u  %02u:%02u  %s\n",
                       ucsize, mo, dy, yr, hh, mm, filename);
                break;
            case 1:             /* verbose */
                printf(
          "%7ld  %-7s%7ld %3d%%  %02u-%02u-%02u  %02u:%02u  %08lx  %s\n",
          ucsize, method[methnum], csize, ratio / 10, mo, dy, yr, hh, mm,
          ULONG_(crec.crc32), filename);
            }

            error = do_string(crec.file_comment_length, (QCOND2 ? DISPLAY : SKIP));
            if (error) {
                error_in_archive = error;       /* might be just warning */
                if (error > 1)  /* fatal */
                    return (error);
            }
            tot_ucsize += (ULONG) ucsize;
            tot_csize += (ULONG) csize;
            ++members;

        } else          /* not listing this file */
          if ((error = do_string(crec.file_comment_length, SKIP)) != 0) {
            error_in_archive = error;   /* might be just warning */
            if (error > 1)      /* fatal */
                return (error);
        }
    }                   /* end for-loop (j: files in central directory) */

/*---------------------------------------------------------------------------
    Print footer line and totals (compressed size, uncompressed size, number
    of members in zipfile).
  ---------------------------------------------------------------------------*/

    ratio = (tot_ucsize == 0) ?
        0 : (int) ((1000L * (tot_ucsize - tot_csize)) / tot_ucsize) + 5;

    if (quietflg < 2) {
        switch (which_hdr) {
        case 0:         /* short */
            printf("%s\n%7lu                   %-7u\n",
                   " ------                   -------",
                   tot_ucsize, members);
            break;
        case 1:         /* verbose */
    printf("%s\n%7lu         %7lu %3d%%                             %-7u\n",
           " ------          ------  ---                             -------",
           tot_ucsize, tot_csize, ratio / 10, members);
        }
    }
/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

    readbuf(zipfd, sig, 4);
    if (strncmp(sig, END_CENTRAL_SIG, 4)) {     /* just to make sure again */
        fprintf(stderr, "\nwarning:  didn't find end-of-central-dir signature \
at end of central dir.\n");
        fprintf(stderr, errmsg);
/* "        (please report to info-zip@wsmr-simtel20.army.mil)\n" */
        error_in_archive = 1;   /* 1:  warning error */
    }
    return (error_in_archive);

}       /* end function list_files() */





/**************************************/
/*  Function extract_or_test_files() */
/**************************************/

int extract_or_test_files()
{                               /* return PK-type error code */
    char **fnamev;
    int lcflags[DIR_BLKSIZ], error, error_in_archive = 0;
    UWORD i, j, members_remaining;
    longint cur_central_dir_offset, dif, offsets[DIR_BLKSIZ];



/*---------------------------------------------------------------------------
    The basic idea of this function is as follows.  Since the central di-
    rectory lies at the end of the zipfile and the member files lie at the
    beginning or middle or wherever, it is not very desirable to simply
    read a central directory entry, jump to the member and extract it, and
    then jump back to the central directory.  In the case of a large zipfile
    this would lead to a whole lot of disk-grinding, especially if each mem-
    ber file is small.  Instead, we read from the central directory the per-
    tinent information for a block of files, then go extract/test the whole
    block.  Thus this routine contains two small(er) loops within a very
    large outer loop:  the first of the small ones reads a block of files
    from the central directory; the second extracts or tests each file; and
    the outer one loops over blocks.  There's some file-pointer positioning
    stuff in between, but that's about it.  Btw, it's because of this jump-
    ing around that we can afford to be lenient if an error occurs in one of
    the member files:  we should still be able to go find the other members,
    since we know the offset of each from the beginning of the zipfile.

    So to start off, set file pointer to start of central directory and make
    sure readbuf() is reset.
  ---------------------------------------------------------------------------*/

    cur_zipfile_fileptr =
        lseek(zipfd, (longint) ULONG_(ecrec.offset_start_central_directory), SEEK_SET);
    incnt = 0;                  /* readbuf() has just been reset */

/*---------------------------------------------------------------------------
    Begin main loop over blocks of member files.  We know the entire central
    directory is on this disk:  we would not have any of this information un-
    less the end-of-central-directory record was on this disk, and we would
    not have gotten to this routine unless this is also the disk on which
    the central directory starts.  In practice, this had better be the ONLY
    disk in the archive, but maybe someday we'll add multi-disk support.
  ---------------------------------------------------------------------------*/

    members_remaining = ecrec.total_entries_central_dir;
    while (members_remaining) {
        j = 0;

        /*-------------------------------------------------------------------
         Loop through files in central directory, storing offsets and case-
         conversion flags until block size is reached.
         -------------------------------------------------------------------*/

        while (members_remaining && (j < DIR_BLKSIZ)) {
            --members_remaining;

            if (readbuf(zipfd, sig, 4) <= 0) {
                error_in_archive = 51;  /* 51:  unexpected EOF */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if (strncmp(sig, CENTRAL_HDR_SIG, 4)) {     /* just to make sure */
                fprintf(stderr, "\nerror:  expected central file header \
signature not found (file #%u).\n", j);
                fprintf(stderr, errmsg);
/* "        (please report to info-zip@wsmr-simtel20.army.mil)\n" */
                error_in_archive = 3;   /* 3:  error in zipfile */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            /*  v------ (sets lcflag) */
            if ((error = process_central_file_header()) != 0) {
                error_in_archive = error;       /* only 51 (EOF) defined */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if ((error = do_string(crec.filename_length, FILENAME)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {/* fatal:  no more left to do */
                    fprintf(stderr, "\n%s:  bad filename length (central)\n",
                            filename);
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = do_string(crec.extra_field_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {/* fatal */
                    fprintf(stderr, "\n%s:  bad extra field length (central)\n",
                            filename);
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = do_string(crec.file_comment_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {/* fatal */
                    fprintf(stderr, "\n%s:  bad file comment length (central)\n",
                            filename);
                    members_remaining = 0;
                    break;
                }
            }
            if (process_all_files) {
                lcflags[j] = lcflag;
                offsets[j++] = (longint) ULONG_(crec.relative_offset_local_header);
            } else {
                fnamev = fnv;   /* don't destroy permanent filename pointer */
                for (--fnamev; *++fnamev;)
                    if (match(filename, *fnamev)) {
                        if (crec.version_needed_to_extract[0] > UNZIP_VERSION) {
                            fprintf(stderr, "\n%s:  requires compatibility \
with version %u.%u to extract\n       (this handles %u.%u)--skipping.\n",
                        filename, crec.version_needed_to_extract[0] / 10,
                        crec.version_needed_to_extract[0] % 10,
                        UNZIP_VERSION / 10, UNZIP_VERSION % 10);
                            error_in_archive = 1;       /* 1:  warning error */
                        } else {
                            lcflags[j] = lcflag;
            offsets[j++] = (longint) ULONG_(crec.relative_offset_local_header);
                        }
                        break;  /* found match for filename, so stop looping */

                    }   /* end if (match), for-loop (fnamev) */
            }           /* end if (process_all_files) */
        }               /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cur_central_dir_offset = cur_zipfile_fileptr - incnt;

        /*--------------------------------------------------------------------
          Loop through files in block, extracting or testing each one.
          -------------------------------------------------------------------*/

        for (i = 0; i < j; ++i) {
            /* if the target position is either after the file pointer
             * (have not yet read far enough) or before the position
             * from which the data presently in the buffer was read,
             * skip to the target position and reset readbuf().
             */
            if ((offsets[i] > cur_zipfile_fileptr) ||
                (offsets[i] < cur_zipfile_bufstart)) {

                cur_zipfile_fileptr = lseek(zipfd, offsets[i], SEEK_SET);
                incnt = 0;
                if (cur_zipfile_fileptr < 0) {
                    fprintf(stderr, "\n%s:  bad zipfile offset (lseek)\n",
                            filename);
                    error_in_archive = 3;       /* 3:  error in zipfile */
                    continue;   /* but can still do next one */
                }
            } else if ((dif = cur_zipfile_fileptr - offsets[i]) != incnt) {
                if (dif > incnt) {
                    fprintf(stderr, "programmer's logic is screwed up:\n");
                    fprintf(stderr, "  cur_zipfile_fileptr = %ld [%.8lXh]\n",
                            cur_zipfile_fileptr, cur_zipfile_fileptr);
                    fprintf(stderr, "  cur_zipfile_bufstart = %ld [%.8lXh]\n",
                            cur_zipfile_bufstart, cur_zipfile_bufstart);
                    fprintf(stderr, "  offsets[%d] = %ld [%.8lXh]\n", i,
                            offsets[i], offsets[i]);
                    fprintf(stderr, "  dif = %ld [%.8lXh]\n", dif, dif);
                    fprintf(stderr, "  incnt = %d [%.4Xh]\n", incnt, incnt);
                    return (99);/* 99:  fucked up */
                }
                inptr += incnt - (int) dif;
                incnt = (int) dif;
            }
            lcflag = lcflags[i];

/*          should be in proper position now, so check for sig */
            if (readbuf(zipfd, sig, 4) <= 0) {
                fprintf(stderr, "\n%s:  bad zipfile offset (EOF)\n", filename);
                error_in_archive = 3;   /* 3:  error in zipfile */
                continue;       /* but can still do next one */
            }
            if (strncmp(sig, LOCAL_HDR_SIG, 4)) {
                fprintf(stderr, "\n%s:  bad zipfile offset (can't find local \
header sig)\n",
                        filename);
                error_in_archive = 3;
                continue;
            }
            if ((error = process_local_file_header()) != 0) {
                fprintf(stderr, "\n%s:  bad local header\n", filename);
                error_in_archive = error;       /* only 51 (EOF) defined */
                continue;       /* can still do next one */
            }
            if ((error = do_string(lrec.filename_length, FILENAME)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {
                    fprintf(stderr, "\n%s:  bad filename length (local)\n",
                            filename);
                    continue;   /* go on to next one */
                }
            }
            if ((error = do_string(lrec.extra_field_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {
                    fprintf(stderr, "\n%s:  bad extra field length (local)\n",
                            filename);
                    continue;   /* go on to next one */
                }
            }
            if ((error = extract_or_test_member()) != 0)
                error_in_archive = error;       /* ...and keep going */

        }                       /* end for-loop (i:  files in current block) */

        /*-------------------------------------------------------------------
         Jump back to where we were in the central directory, then go and do
         the next batch of files.
          -------------------------------------------------------------------*/

        cur_zipfile_fileptr = lseek(zipfd, cur_central_dir_offset, SEEK_SET);
        incnt = 0;

    }           /* end while-loop (blocks of files in central directory) */

/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

    readbuf(zipfd, sig, 4);
    if (strncmp(sig, END_CENTRAL_SIG, 4)) {     /* just to make sure again */
        fprintf(stderr, "\nwarning:  didn't find end-of-central-dir signature \
at end of central dir.\n");
        fprintf(stderr, errmsg);
/* "        (please report to info-zip@wsmr-simtel20.army.mil)\n" */
        if (!error_in_archive)  /* don't overwrite stronger error */
            error_in_archive = 1;       /* 1:  warning error */
    }
    if (tflag && (quietflg < 2)) {
        if (error_in_archive)
            printf("At least one error was detected in the archive.\n");
        else if (process_all_files)
            printf("No errors detected.\n");
        else
            printf("No errors detected in the tested files.\n");
    }
    return (error_in_archive);

}       /* end function extract_or_test_files() */





/***************************************/
/*  Function extract_or_test_member() */
/***************************************/

int extract_or_test_member()
{                               /* return PK-type error code */
    UWORD b;                    /* for test reasons */



/*---------------------------------------------------------------------------
    Initialize variables, buffers, etc.
  ---------------------------------------------------------------------------*/

    bits_left = 0;
    bitbuf = 0;
    outpos = 0L;
    outcnt = 0;
    outptr = outbuf;
    zipeof = 0;
    crc32val = 0xFFFFFFFFL;

    memset(outbuf, 0, OUTBUFSIZ);
    if (aflag)                  /* if we have a scratchpad, */
        memset(outout, 0, OUTBUFSIZ);   /*  clear it out */

    if (tflag) {
        if (!quietflg) {
            fprintf(stdout, "  Testing: %-12s ", filename);
            fflush(stdout);
        }
    } else {
        if (cflag)              /* output to stdout (copy of it) */
            outfd = dup(1);
        else {
            if (!mapped_name()) /* member name conversion failed */
                return (2);     /* 2:  (weak) error in zipfile */
            if (create_output_file())   /* output to file:  read/write perms */
                return (50);    /* 50:  disk full */
        }
    }                           /* endif (!tflag) */

/*---------------------------------------------------------------------------
    Unpack the file.
  ---------------------------------------------------------------------------*/

    switch (lrec.compression_method) {

    case STORED:
        if (!tflag && QCOND) {
            fprintf(stdout, " Extracting: %-12s ", filename);
            if (cflag)
                fprintf(stdout, "\n");
            fflush(stdout);
        }
        while (ReadByte(&b))
            OUTB(b);
        break;

    case SHRUNK:
        if (!tflag && QCOND) {
            fprintf(stdout, "UnShrinking: %-12s ", filename);
            if (cflag)
                fprintf(stdout, "\n");
            fflush(stdout);
        }
        unShrink();
        break;

    case REDUCED1:
    case REDUCED2:
    case REDUCED3:
    case REDUCED4:
        if (!tflag && QCOND) {
            fprintf(stdout, "  Expanding: %-12s ", filename);
            if (cflag)
                fprintf(stdout, "\n");
            fflush(stdout);
        }
        unReduce();
        break;

    case IMPLODED:
        if (!tflag && QCOND) {
            fprintf(stdout, "  Exploding: %-12s ", filename);
            if (cflag)
                fprintf(stdout, "\n");
            fflush(stdout);
        }
        unImplode();
        break;

    default:
        fprintf(stderr, "%s:  unknown compression method\n", filename);
        /* close and delete file before return?? */
        return (1);             /* 1:  warning error */
    }

/*---------------------------------------------------------------------------
    Write the last partial buffer, if any; set the file date and time; and
    close the file (not necessarily in that order).  Then make sure CRC came
    out OK and print result.  [Note:  crc32val must be logical-ANDed with
    32 bits of 1's, or else machines whose longs are bigger than 32 bits will
    report bad CRCs (because of the upper bits being filled with 1's instead
    of 0's).  v3.94]
  ---------------------------------------------------------------------------*/

    if (FlushOutput())
        return (50);            /* 50:  disk full */

    if (!tflag)
#if defined(VMS) || defined(MTS)/* VMS already set file time; MTS can't */
        close(outfd);
#else
        set_file_time_and_close();
#endif

    if ((crc32val = ((~crc32val) & 0xFFFFFFFFL)) != ULONG_(lrec.crc32)) {
        /* if quietflg is set we haven't output the filename yet, do it */
        if (quietflg)
            printf("%-12s: ", filename);
        fprintf(stdout, " Bad CRC %08lx  (should be %08lx)\n", crc32val,
                ULONG_(lrec.crc32));
        return (1);             /* 1:  warning error */
    } else if (tflag) {
        if (!quietflg)
            fprintf(stdout, " OK\n");
    } else {
        if (QCOND)
            fprintf(stdout, "\n");
    }

    return (0);                 /* 0:  no error */

}       /* end function extract_or_test_member() */





/********************************************/
/*  Function process_central_file_header() */
/********************************************/

int process_central_file_header()
{                               /* return PK-type error code */
#ifdef NOTINT16
    central_directory_byte_header byterec;
#endif



/*---------------------------------------------------------------------------
    Read the next central directory entry and do any necessary machine-type
    conversions (byte ordering, structure padding compensation--in the latter
    case, copy the data from the array into which it was read (byterec) to
    the usable struct (crec)).
  ---------------------------------------------------------------------------*/

#ifndef NOTINT16
    if (readbuf(zipfd, (char *) &crec, CREC_SIZE) <= 0)
        return (51);

#else                           /* NOTINT16 */
    if (readbuf(zipfd, byterec, CREC_SIZE) <= 0)
        return (51);            /* 51:  unexpected EOF */

    crec.version_made_by[0] = byterec[C_VERSION_MADE_BY_0];
    crec.version_made_by[1] = byterec[C_VERSION_MADE_BY_1];
    crec.version_needed_to_extract[0] = byterec[C_VERSION_NEEDED_TO_EXTRACT_0];
    crec.version_needed_to_extract[1] = byterec[C_VERSION_NEEDED_TO_EXTRACT_1];

    crec.general_purpose_bit_flag =
        makeword(&byterec[C_GENERAL_PURPOSE_BIT_FLAG]);
    crec.compression_method =
        makeword(&byterec[C_COMPRESSION_METHOD]);
    crec.last_mod_file_time =
        makeword(&byterec[C_LAST_MOD_FILE_TIME]);
    crec.last_mod_file_date =
        makeword(&byterec[C_LAST_MOD_FILE_DATE]);
    crec.crc32 =
        makelong(&byterec[C_CRC32]);
    crec.compressed_size =
        makelong(&byterec[C_COMPRESSED_SIZE]);
    crec.uncompressed_size =
        makelong(&byterec[C_UNCOMPRESSED_SIZE]);
    crec.filename_length =
        makeword(&byterec[C_FILENAME_LENGTH]);
    crec.extra_field_length =
        makeword(&byterec[C_EXTRA_FIELD_LENGTH]);
    crec.file_comment_length =
        makeword(&byterec[C_FILE_COMMENT_LENGTH]);
    crec.disk_number_start =
        makeword(&byterec[C_DISK_NUMBER_START]);
    crec.internal_file_attributes =
        makeword(&byterec[C_INTERNAL_FILE_ATTRIBUTES]);
    crec.external_file_attributes =
        makelong(&byterec[C_EXTERNAL_FILE_ATTRIBUTES]); /* LONG, not word! */
    crec.relative_offset_local_header =
        makelong(&byterec[C_RELATIVE_OFFSET_LOCAL_HEADER]);
#endif                          /* NOTINT16 */

    hostnum = min(crec.version_made_by[1], NUM_HOSTS);
/*  extnum = min( crec.version_needed_to_extract[1], NUM_HOSTS ); */
    methnum = min(crec.compression_method, NUM_METHODS);

/*---------------------------------------------------------------------------
    Set flag for lowercase conversion of filename, depending on which OS the
    file is coming from.  This section could be ifdef'd if some people have
    come to love DOS uppercase filenames under Unix...but really, guys, get
    a life. :)  NOTE THAT ALL SYSTEM NAMES NOW HAVE TRAILING UNDERSCORES!!!
    This is to prevent interference with compiler command-line defines such
    as -DUNIX, for example, which are then used in "#ifdef UNIX" constructs.
  ---------------------------------------------------------------------------*/

    lcflag = FALSE;
    if (!Uflag)                 /* as long as user hasn't specified
                                 * case-preservation */
        switch (hostnum) {
        case DOS_OS2_FAT_:
        case VMS_:
        case VM_CMS_:           /* all caps? */
        case CPM_:              /* like DOS, right? */
/*        case ATARI_:          ??? */
/*        case Z_SYSTEM_:       ??? */
/*        case TOPS20_:         (if we had such a thing...) */

            lcflag = TRUE;      /* convert filename to lowercase */
            break;

        default:                /* AMIGA_, UNIX_, (ATARI_), OS2_HPFS_, */
            break;              /*   MAC_, (Z_SYSTEM_):  no conversion */
        }

    return (0);

}       /* end function process_central_file_header() */





/******************************************/
/*  Function process_local_file_header() */
/******************************************/

int process_local_file_header()
{                               /* return PK-type error code */
#ifdef NOTINT16
    local_byte_header byterec;
#endif



/*---------------------------------------------------------------------------
    Read the next local file header and do any necessary machine-type con-
    versions (byte ordering, structure padding compensation--in the latter
    case, copy the data from the array into which it was read (byterec) to
    the usable struct (lrec)).
  ---------------------------------------------------------------------------*/

#ifndef NOTINT16
    if (readbuf(zipfd, (char *) &lrec, LREC_SIZE) <= 0)
        return (51);

#else                           /* NOTINT16 */
    if (readbuf(zipfd, byterec, LREC_SIZE) <= 0)
        return (51);            /* 51:  unexpected EOF */

    lrec.version_needed_to_extract[0] = byterec[L_VERSION_NEEDED_TO_EXTRACT_0];
    lrec.version_needed_to_extract[1] = byterec[L_VERSION_NEEDED_TO_EXTRACT_1];

    lrec.general_purpose_bit_flag = makeword(&byterec[L_GENERAL_PURPOSE_BIT_FLAG]);
    lrec.compression_method = makeword(&byterec[L_COMPRESSION_METHOD]);
    lrec.last_mod_file_time = makeword(&byterec[L_LAST_MOD_FILE_TIME]);
    lrec.last_mod_file_date = makeword(&byterec[L_LAST_MOD_FILE_DATE]);
    lrec.crc32 = makelong(&byterec[L_CRC32]);
    lrec.compressed_size = makelong(&byterec[L_COMPRESSED_SIZE]);
    lrec.uncompressed_size = makelong(&byterec[L_UNCOMPRESSED_SIZE]);
    lrec.filename_length = makeword(&byterec[L_FILENAME_LENGTH]);
    lrec.extra_field_length = makeword(&byterec[L_EXTRA_FIELD_LENGTH]);
#endif                          /* NOTINT16 */

    csize = (longint) ULONG_(lrec.compressed_size);
    ucsize = (longint) ULONG_(lrec.uncompressed_size);

    return (0);                 /* 0:  no error */

}       /* end function process_local_file_header() */
