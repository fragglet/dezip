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

     under MS-DOS (TurboC):  make -fMAKEFILE.DOS  for command line compiles
       (or use the integrated environment and the included files TCCONFIG.TC
        and UNZIP.PRJ.  Tweak for your environment.)

     under MS-DOS (MSC):  make MAKEFILE.DOS
       (or use Makefile if you have MSC 6.0:  "nmake msc_dos")

     under OS/2 (MSC):  make MAKEFILE.DOS   (edit appropriately)
       (or use Makefile if you have MSC 6.0:  "nmake msc_os2")

     under Atari OS:  Any Day Now.

     under VMS:  DEFINE LNK$LIBRARY SYS$LIBRARY:VAXCRTL.OLB   (see VMSNOTES)
                 CC UNZIP,FILE_IO,MAPNAME,MATCH,...,UNSHRINK
                 LINK UNZIP,FILE_IO,MAPNAME,MATCH,...,UNSHRINK
                 UNZIP :== $DISKNAME:[DIRECTORY]UNZIP.EXE

     under Macintosh OS:   Double click on unzip.mac.  Press <Command>-M

  ---------------------------------------------------------------------------

  Version:  unzip41.arc/unzip41.tar.Z for Unix, VMS, OS/2, MS-DOS & Mac
  Source:   wsmr-simtel20.army.mil (192.88.110.20) in pd1:[misc.unix]
            wuarchive.wustl.edu (128.252.135.4) in /mirrors/misc/unix
  Source:   wsmr-simtel20.army.mil (26.2.0.74) in pd1:[misc.unix]

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
     * If you have to add a new one for YOUR system, please forward the new
     * Makefile context diff to kirsch%maxemail@uunet.uu.net for distribution.
     * Be SURE to give FULL details on your system (hardware, OS, versions,
     * processor, whatever) that made it unique.
     *
     * REVISION HISTORY : See History.41 (or whatever current version is)

  To join Info-ZIP, send a message to Info-ZIP-Request@WSMR-Simtel20.Army.Mil

  ---------------------------------------------------------------------------*/





#include "unzip.h"              /* includes, defines, and macros */

#define VERSION  "v4.1 of 5-13-91"
#define PAKFIX   /* temporary solution to PAK-created zipfiles */





/**********************/
/*  Global Variables */
/**********************/

int tflag;              /* -t: test */
int vflag;              /* -v: view directory (only used in unzip.c) */
int cflag;              /* -c: output to stdout */
int aflag;              /* -a: do ascii to ebcdic translation OR CR-LF */
                        /*     to CR or LF conversion of extracted files */
int dflag;              /* -d: create containing directories */
int Uflag;              /* -U: leave filenames in upper or mixed case */
int V_flag;             /* -V: don't strip VMS version numbers */
int quietflg;           /* -q: produce a lot less output */
int do_all;             /* -o: OK to overwrite files without prompting */
int zflag;              /* -z: Display only the archive comment */

int lcflag;             /* convert filename to lowercase */
unsigned f_attr;        /* file attributes (permissions) */
longint csize;          /* used by list_files(), ReadByte(): must be signed */
longint ucsize;         /* used by list_files(), unReduce(), unImplode() */

/*---------------------------------------------------------------------------
    unShrink/unReduce/unImplode working storage:
  ---------------------------------------------------------------------------*/

/* prefix_of (for unShrink) is biggest storage area, esp. on Crays...space */
/*  is shared by lit_nodes (unImplode) and followers (unReduce) */

short prefix_of[HSIZE + 1];     /* (8193 * sizeof(short)) */
#ifdef MACOS
byte *suffix_of;
byte *stack;
#else
byte suffix_of[HSIZE + 1];      /* also s-f length_nodes (smaller) */
byte stack[HSIZE + 1];          /* also s-f distance_nodes (smaller) */
#endif

ULONG crc32val;

UWORD mask_bits[] =
{0, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff};

/*---------------------------------------------------------------------------
    Input file variables:
  ---------------------------------------------------------------------------*/

byte *inbuf, *inptr;    /* input buffer (any size is legal) and pointer */
int incnt;

UWORD bitbuf;
int bits_left;
boolean zipeof;

int zipfd;                      /* zipfile file handle */
char zipfn[FILNAMSIZ];

local_file_header lrec;
struct stat statbuf;            /* used by main(), mapped_name() */

longint cur_zipfile_bufstart;   /* extract_or_test_files, readbuf, ReadByte */

/*---------------------------------------------------------------------------
    Output stream variables:
  ---------------------------------------------------------------------------*/

byte *outbuf;                   /* buffer for rle look-back */
byte *outptr;
byte *outout;                   /* scratch pad for ASCII-native trans */
longint outpos;                 /* absolute position in outfile */
int outcnt;                     /* current position in outbuf */

int outfd;
char filename[FILNAMSIZ];

/*---------------------------------------------------------------------------
    unzip.c static global variables (visible only within this file):
  ---------------------------------------------------------------------------*/

static char *fnames[2] =
{"*", NULL};                    /* default filenames vector */
static char **fnv = &fnames[0];
static char sig[5];
static byte *hold;
static int process_all_files;
static longint ziplen;
static UWORD hostnum;
static UWORD methnum;
/* static UWORD extnum; */
static central_directory_file_header crec;
static end_central_dir_record ecrec;

/*---------------------------------------------------------------------------
    unzip.c repeated error messages (we use all of these at least twice ==>
    worth it to centralize to keep executable small):
  ---------------------------------------------------------------------------*/

static char *CryptMsg =
  "%s:  encrypted (can't do yet)--skipping.\n";
static char *FilNamMsg =
  "\n%s:  bad filename length (%s)\n";
static char *ExtFieldMsg =
  "\n%s:  bad extra field length (%s)\n";
static char *OffsetMsg =
  "\n%s:  bad zipfile offset (%s)\n";
static char *EndSigMsg =
  "\nwarning:  didn't find end-of-central-dir signature at end of central dir.\n";
static char *CentSigMsg =
  "\nerror:  expected central file header signature not found (file #%u).\n";
static char *ReportMsg =
  "        (please report to info-zip@wsmr-simtel20.army.mil)\n";

/* (original) Bill Davidsen version */
#define QCOND    (!quietflg)    /* -xq[q] kill "extracting: ..." msgs */
#define QCOND2   (which_hdr)    /* file comments with -v, -vq, -vqq */




/*****************/
/*  Main program */
/*****************/

main(argc, argv)        /* return PK-type error code (except under VMS) */
int argc;
char *argv[];
{
    char *s;
    int c, print_usage=TRUE;



#ifdef MACOS
#ifdef THINK_C
    #include <console.h>
    #include <StdFilePkg.h>
    typedef struct sf_node {        /* node in a true shannon-fano tree */
        UWORD left;                 /* 0 means leaf node */
        UWORD right;                /*   or value if leaf node */
    } sf_node;
    static char *argstr[30], args[30*64];
 
    extern sf_node *lit_nodes, *length_nodes, *distance_nodes;
 
    Point   p;
    SFTypeList  sfT;
    int a;
    EventRecord theEvent;
    short   eMask;
    SFReply  fileRep;

    suffix_of = (byte *)calloc(HSIZE+1, sizeof(byte));
    stack = (byte *)calloc(HSIZE+1, sizeof(byte));
    length_nodes = (sf_node *) suffix_of;  /* 2*LENVALS nodes */
    distance_nodes = (sf_node *) stack;    /* 2*DISTVALS nodes */
   
    for (a=0; a<30; a+=1)
    {
        argstr[a] = &args[a*64];
    }
start:
    tflag = vflag=cflag=aflag=dflag=Uflag=quietflg=lcflag=zflag = 0;
    argc = ccommand(&argv);
    SetPt(&p, 40,40);

    SFGetFile(p, "\pSpecify ZIP file:", 0L, -1, sfT, 0L, &fileRep);
    if (!fileRep.good)
        exit(1);
    macfstest(fileRep.vRefNum, true);
    SetMacVol(NULL, fileRep.vRefNum);
    for (a=1; a<argc; a+=1)
    {
        if (argv[a][0] == '-')
        {
            BlockMove(argv[a], argstr[a], (strlen(argv[a])>63) ? 64 : strlen(argv[a])+1);
        }
        else
            break;
    }
    PtoCstr((char *)fileRep.fName);
    strcpy(argstr[a], fileRep.fName);
    for (;a<argc; a+=1)
    {
        BlockMove(argv[a], argstr[a+1], (strlen(argv[a])>63) ? 64 : strlen(argv[a])+1);
    }
    argc+=1;
    argv = argstr;

#endif /* THINK_C */
#endif /* MACOS */

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

#ifndef KNOW_IT_WORKS  /* define this to save space, if things already work */
#ifndef DOS_OS2        /* already works (no RISCy OS/2's yet...) */
#ifndef NOTINT16       /* whole point is to see if this NEEDS defining */
    {
        int error=0;
        long testsig;
        static char *mach_type[3] = {"big-endian",
          "structure-padding", "big-endian and structure-padding"};
        strcpy((char *)&testsig,"012");
        if (testsig != 0x00323130)
            error = 1;
        if (sizeof(central_directory_file_header) != CREC_SIZE)
            error += 2;
        if (error--)
            fprintf(stderr, "It appears that your machine is %s.  If errors\n\
occur, please try recompiling with \"NOTINT16\" defined (read the\n\
Makefile, or try \"make hp\").\n\n", mach_type[error]);
    }
#endif /* !NOTINT16 */
#endif /* !DOS_OS2 */
#endif /* !KNOW_IT_WORKS */

/*---------------------------------------------------------------------------
    Rip through any command-line options lurking about...
  ---------------------------------------------------------------------------*/

    while (--argc > 0 && (*++argv)[0] == '-') {
        s = argv[0] + 1;
        while ((c = *s++) != 0) {    /* "!= 0":  prevent Turbo C warning */
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
            case ('V'): /* Version flag:  retain VMS/DEC-20 file versions */
                ++V_flag;
                break;
            case ('d'): /* create parent dirs flag */
#ifdef MACOS
                if (hfsflag == true)
#endif
                ++dflag;
                break;
            case ('o'): /* OK to overwrite files without prompting */
                ++do_all;
                break;
            case ('z'): /* Display only the archive commentp */
                ++zflag;
                break;
            default:
                if (print_usage) {
                    usage();
                    print_usage = FALSE;
                }
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
    if (quietflg && zflag)
        quietflg = 0;
    if (argc-- == 0) {
        if (print_usage)
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
        ziplen = statbuf.st_size;

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
        outout = (byte *) (malloc(OUTBUFSIZ));
    else                        /*  allocate it... */
        outout = outbuf;        /*  else just point to outbuf */

    if ((inbuf == NULL) || (outbuf == NULL) || (outout == NULL)) {
        fprintf(stderr, "error:  can't allocate unzip buffers\n");
        RETURN(4);              /* 4-8:  insufficient memory */
    }
    hold = &inbuf[INBUFSIZ];    /* to check for boundary-spanning signatures */

#ifdef THINK_C
    if (!process_zipfile())
        goto start;
#else
    RETURN(process_zipfile());  /* keep passing errors back... */
#endif

}       /* end main() */





/*********************/
/*  Function usage() */
/*********************/

void usage()
{
#ifdef NATIVE
    char *astring = "-a     convert ASCII to native character set";
#else
#ifdef MACOS
    char *astring = "-a     convert to Mac textfile format (CR LF => CR)";
#else
#ifdef DOS_OS2
    char *astring = "-a     convert to DOS & OS/2 textfile format (LF => CR LF)";
#else
    char *astring = "-a     convert to Unix/VMS textfile format (CR LF => LF)";
#endif /* ?DOS_OS2 */
#endif /* ?MACOS */
#endif /* ?NATIVE */


    fprintf(stderr, "UnZip:  Zipfile Extract %s;  (C) 1989 Samuel H. Smith\n",
                 VERSION);
    fprintf(stderr, "Courtesy of:  S.H.Smith  and  The Tool Shop BBS,  \
(602) 279-2673.\n\n");
    fprintf(stderr, "Versions 3.0 and later brought to you by the fine folks \
at Info-ZIP\n");
    fprintf(stderr, "(Info-ZIP@WSMR-Simtel20.Army.Mil)\n\n");
    fprintf(stderr, "Usage:  unzip [ -xecptlvz[qadoUV] ] file[.zip] [filespec...]\n\
  -x,-e  extract files in archive (default--i.e., this flag is optional)\n\
  -c     extract files to stdout (\"CRT\")\n\
  -p     extract files to stdout and no informational messages (for pipes)\n\
  -t     test files\n\
  -l     list files (short format)\n\
  -v     verbose listing of files\n");
    fprintf(stderr, "\
  -z     display only the archive comment\n\
  -q     perform operations quietly (up to two q's allowed)\n\
  %s\n\
  -d     include directory structure when extracting/listing\n\
  -o     OK to overwrite files without prompting\n\
  -U     don't map filenames to lowercase for selected (uppercase) OS's\n\
  -V     retain file version numbers\n", astring);

#ifdef VMS
    fprintf(stderr, "Remember that non-lowercase filespecs must be quoted in \
VMS (e.g., \"Makefile\").\n");
#endif

}       /* end function usage() */





/*******************************/
/*  Function process_zipfile() */
/*******************************/

int process_zipfile()
/* return PK-type error code */
{
    int error=0, error_in_archive;



/*---------------------------------------------------------------------------
    Open the zipfile for reading and in BINARY mode to prevent CR/LF trans-
    lation, which would corrupt the bitstreams.  Then find and process the
    central directory; list, extract or test member files as instructed; and
    close the zipfile.
  ---------------------------------------------------------------------------*/

#ifdef VMS
    change_zipfile_attributes(0);
#endif
    if (open_input_file())      /* this should never happen, given the */
        return (9);             /*   stat() test in main(), but... */

    if (find_end_central_dir()) /* not found; nothing to do */
        return (2);             /* 2:  error in zipfile */

#ifdef TEST
    printf("\n  found end-of-central-dir signature at offset %ld (%.8lXh)\n",
      cur_zipfile_bufstart+(inptr-inbuf), cur_zipfile_bufstart+(inptr-inbuf) );
    printf("    from beginning of file; offset %d (%.4Xh) within block\n",
      inptr-inbuf, inptr-inbuf);
#endif

    if ((error_in_archive = process_end_central_dir()) > 1)
        return (error_in_archive);

    if (zflag)
        return (0);

#ifndef PAKFIX
    if (ecrec.number_this_disk == 0) {
#else /* PAKFIX */
    if (ecrec.number_this_disk == 0  ||  (error = (ecrec.number_this_disk == 1
        && ecrec.num_disk_with_start_central_dir == 1))) {

        if (error) {
            fprintf(stderr,
     "\n     Warning:  zipfile claims to be disk 2 of a two-part archive;\n\
     attempting to process anyway.  If no further errors occur, this\n\
     archive was probably created by PAK v2.5 or earlier.  This bug\n\
     has been reported to NoGate and should be fixed by mid-April 1991.\n\n");
            error_in_archive = 1;  /* 1:  warning */
        }
#endif /* ?PAKFIX */
        if (vflag)
            error = list_files();       /* LIST 'EM */
        else
            error = extract_or_test_files();    /* EXTRACT OR TEST 'EM */
        if (error > error_in_archive)   /* don't overwrite stronger error */
            error_in_archive = error;   /*  with (for example) a warning */
    } else {
        fprintf(stderr, "\nerror:  zipfile is part of multi-disk archive \
(sorry, no can do).\n");
        fprintf(stderr, ReportMsg);   /* report to info-zip */
        error_in_archive = 11;  /* 11:  no files found */
    }

    close(zipfd);
#ifdef VMS
    change_zipfile_attributes(1);
#endif
    return (error_in_archive);

}       /* end function process_zipfile() */





/************************************/
/*  Function find_end_central_dir() */
/************************************/

int find_end_central_dir()
/* return 0 if found, 1 otherwise */
{
    int i, numblks;
    longint tail_len;



/*---------------------------------------------------------------------------
    Treat case of short zipfile separately.
  ---------------------------------------------------------------------------*/

    if (ziplen <= INBUFSIZ) {
        lseek(zipfd, 0L, SEEK_SET);
        if ((incnt = read(zipfd,inbuf,(unsigned int)ziplen)) == ziplen)

            /* 'P' must be at least 22 bytes from end of zipfile */
            for ( inptr = inbuf+ziplen-22  ;  inptr >= inbuf  ;  --inptr )
                if ( (ascii_to_native(*inptr) == 'P')  &&
                      !strncmp((char *)inptr, END_CENTRAL_SIG, 4) ) {
                    incnt -= inptr - inbuf;
                    return(0);  /* found it! */
                }               /* ...otherwise fall through & fail */

/*---------------------------------------------------------------------------
    Zipfile is longer than INBUFSIZ:  may need to loop.  Start with short
    block at end of zipfile (if not TOO short).
  ---------------------------------------------------------------------------*/

    } else {
        if ((tail_len = ziplen % INBUFSIZ) > ECREC_SIZE) {
            cur_zipfile_bufstart = lseek(zipfd, ziplen-tail_len, SEEK_SET);
            if ((incnt = read(zipfd,inbuf,(unsigned int)tail_len)) != tail_len)
                goto fail;      /* shut up, it's expedient. */

            /* 'P' must be at least 22 bytes from end of zipfile */
            for ( inptr = inbuf+tail_len-22  ;  inptr >= inbuf  ;  --inptr )
                if ( (ascii_to_native(*inptr) == 'P')  &&
                      !strncmp((char *)inptr, END_CENTRAL_SIG, 4) ) {
                    incnt -= inptr - inbuf;
                    return(0);  /* found it! */
                }               /* ...otherwise search next block */
            strncpy((char *)hold, (char *)inbuf, 3);    /* sig may span block
                                                           boundary */

        } else {
            cur_zipfile_bufstart = ziplen - tail_len;
        }

        /*
         * Loop through blocks of zipfile data, starting at the end and going
         * toward the beginning.  Need only check last 65557 bytes of zipfile:
         * comment may be up to 65535 bytes long, end-of-central-directory rec-
         * ord is 18 bytes (shouldn't hardcode this number, but what the hell:
         * already did so above (22=18+4)), and sig itself is 4 bytes.
         */

        /*          ==amt to search==   ==done==   ==rounding==     =blksiz= */
        numblks = ( min(ziplen,65557) - tail_len + (INBUFSIZ-1) ) / INBUFSIZ;

        for ( i = 1  ;  i <= numblks  ;  ++i ) {
            cur_zipfile_bufstart -= INBUFSIZ;
            lseek(zipfd, cur_zipfile_bufstart, SEEK_SET);
            if ((incnt = read(zipfd,inbuf,INBUFSIZ)) != INBUFSIZ)
                break;          /* fall through and fail */

            for ( inptr = inbuf+INBUFSIZ-1  ;  inptr >= inbuf  ;  --inptr )
                if ( (ascii_to_native(*inptr) == 'P')  &&
                      !strncmp((char *)inptr, END_CENTRAL_SIG, 4) ) {
                    incnt -= inptr - inbuf;
                    return(0);  /* found it! */
                }
            strncpy((char *)hold, (char *)inbuf, 3);    /* sig may span block
                                                           boundary */
        }

    } /* end if (ziplen > INBUFSIZ) */

/*---------------------------------------------------------------------------
    Searched through whole region where signature should be without finding
    it.  Print informational message and die a horrible death.
  ---------------------------------------------------------------------------*/

fail:

    fprintf(stderr, "\nFile:  %s\n\n\
     End-of-central-directory signature not found.  Either this file is not\n\
     a zipfile, or it constitutes one disk of a multi-part archive.  In the\n\
     latter case the central directory and zipfile comment will be found on\n\
     the last disk(s) of this archive.\n", zipfn);
    return(1);

}       /* end function find_end_central_dir() */





/***************************************/
/*  Function process_end_central_dir() */
/***************************************/

int process_end_central_dir()
/* return PK-type error code */
{
#ifdef NOTINT16
    end_central_byte_record byterec;
#endif
    int error=0;



/*---------------------------------------------------------------------------
    Read the end-of-central-directory record and do any necessary machine-
    type conversions (byte ordering, structure padding compensation).
  ---------------------------------------------------------------------------*/

#ifndef NOTINT16
    if (readbuf((char *) &ecrec, ECREC_SIZE+4) <= 0)
        return (51);            /* 51:  unexpected EOF */

#else  /* NOTINT16:  read data into character array, then copy to struct */
    if (readbuf((char *) byterec, ECREC_SIZE+4) <= 0)
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
#endif  /* NOTINT16 */

/*---------------------------------------------------------------------------
    Get the zipfile comment, if any, and print it out.  (Comment may be up
    to 64KB long.  May the fleas of a thousand camels infest the armpits of
    anyone who actually takes advantage of this fact.)  Then position the
    file pointer to the beginning of the central directory and fill buffer.
  ---------------------------------------------------------------------------*/

    if (ecrec.zipfile_comment_length && !quietflg) {
        if (!zflag)
          printf("[%s] comment:  ", zipfn);
        if (do_string(ecrec.zipfile_comment_length,DISPLAY)) {
            fprintf(stderr, "\nwarning:  zipfile comment truncated\n");
            error = 1;          /* 1:  warning error */
        }
        if (!zflag)
          printf("\n\n");
    }
    LSEEK( ULONG_(ecrec.offset_start_central_directory) )

    return (error);

}       /* end function process_end_central_dir() */





/**************************/
/*  Function list_files() */
/**************************/

int list_files()
/* return PK-type error code */
{
    char **fnamev;
    int do_this_file = FALSE, ratio, error, error_in_archive = 0;
    int which_hdr = (vflag > 1);
    UWORD j, yr, mo, dy, hh, mm, members = 0;
    ULONG tot_csize = 0L, tot_ucsize = 0L;
    static char *method[NUM_METHODS + 1] =
    {"Stored", "Shrunk", "Reduce1", "Reduce2",
     "Reduce3", "Reduce4", "Implode", "Unknown"};
    static char *Headers[][2] =
    {
        {" Length    Date    Time    Name",
         " ------    ----    ----    ----"},
        {" Length  Method   Size  Ratio   Date    Time   CRC-32     Name",
         " ------  ------   ----  -----   ----    ----   ------     ----"}};



/*---------------------------------------------------------------------------
    Unlike extract_or_test_files(), this routine confines itself to the cen-
    tral directory.  Thus its structure is somewhat simpler, since we can do
    just a single loop through the entire directory, listing files as we go.

    So to start off, print the heading line and then begin main loop through
    the central directory.  The results will look vaguely like the following:

  Length  Method   Size  Ratio   Date    Time   CRC-32     Name ("^" ==> case
  ------  ------   ----  -----   ----    ----   ------     ----   conversion)
   44004  Implode  13041  71%  11-02-89  19:34  8b4207f7   Makefile.UNIX
    3438  Shrunk    2209  36%  09-15-90  14:07  a2394fd8  ^dos-file.ext
  ---------------------------------------------------------------------------*/

    if (quietflg < 2)
        if (Uflag)
            printf("%s\n%s\n", Headers[which_hdr][0], Headers[which_hdr][1]);
        else
            printf("%s (\"^\" ==> case\n%s   conversion)\n", 
              Headers[which_hdr][0], Headers[which_hdr][1]);

    for (j = 0; j < ecrec.total_entries_central_dir; ++j) {

        if (readbuf(sig, 4) <= 0)
            return (51);        /* 51:  unexpected EOF */
        if (strncmp(sig, CENTRAL_HDR_SIG, 4)) {  /* just to make sure */
            fprintf(stderr, CentSigMsg, j);  /* sig not found */
            fprintf(stderr, ReportMsg);   /* report to info-zip */
            return (3);         /* 3:  error in zipfile */
        }
        if ((error = process_central_file_header()) != 0)  /* (sets lcflag) */
            return (error);     /* only 51 (EOF) defined */

        /*
         * We could DISPLAY the filename instead of storing (and possibly trun-
         * cating, in the case of a very long name) and printing it, but that
         * has the disadvantage of not allowing case conversion--and it's nice
         * to be able to see in the listing precisely how you have to type each
         * filename in order for unzip to consider it a match.  Speaking of
         * which, if member names were specified on the command line, check in
         * with match() to see if the current file is one of them, and make a
         * note of it if it is.
         */

        if ((error = do_string(crec.filename_length, FILENAME)) != 0) {
            error_in_archive = error;  /* (uses lcflag)---^   */
            if (error > 1)      /* fatal:  can't continue */
                return (error);
        }
        if ((error = do_string(crec.extra_field_length, SKIP)) != 0) {
            error_in_archive = error;   /* might be just warning */
            if (error > 1)      /* fatal */
                return (error);
        }
        if (!process_all_files) {   /* check if specified on command line */
            do_this_file = FALSE;
            fnamev = fnv;       /* don't destroy permanent filename ptr */
            for (--fnamev; *++fnamev;)
                if (match(filename, *fnamev)) {
                    do_this_file = TRUE;
                    break;      /* found match, so stop looping */
                }
        }
        /*
         * If current file was specified on command line, or if no names were
         * specified, do the listing for this file.  Otherwise, get rid of the
         * file comment and go back for the next file.
         */

        if (process_all_files || do_this_file) {

            yr = (((crec.last_mod_file_date >> 9) & 0x7f) + 80) % 100;
            mo = (crec.last_mod_file_date >> 5) & 0x0f;
            dy = crec.last_mod_file_date & 0x1f;
            hh = (crec.last_mod_file_time >> 11) & 0x1f;
            mm = (crec.last_mod_file_time >> 5) & 0x3f;

            csize = (longint) ULONG_(crec.compressed_size);
            ucsize = (longint) ULONG_(crec.uncompressed_size);
            if (crec.general_purpose_bit_flag & 1)
                csize -= 12;    /* if encrypted, don't count encrypt hdr */

            ratio = (ucsize == 0) ? 0 :   /* .zip can have 0-length members */
                ((ucsize > 2000000) ?     /* risk signed overflow if mult. */
                (int) ((ucsize-csize) / (ucsize/1000L)) + 5 :   /* big */
                (int) ((1000L*(ucsize-csize)) / ucsize) + 5);   /* small */

            switch (which_hdr) {
            case 0:             /* short form */
                printf("%7ld  %02u-%02u-%02u  %02u:%02u  %c%s\n",
                       ucsize, mo, dy, yr, hh, mm, (lcflag?'^':' '), filename);
                break;
            case 1:             /* verbose */
                printf(
                  "%7ld  %-7s%7ld %3d%%  %02u-%02u-%02u  %02u:%02u  %08lx  %c%s\n",
                  ucsize, method[methnum], csize, ratio/10, mo, dy, yr, hh, mm,
                  ULONG_(crec.crc32), (lcflag?'^':' '), filename);
            }

            error = do_string(crec.file_comment_length, (QCOND2 ? DISPLAY : SKIP));
            if (error) {
                error_in_archive = error;  /* might be just warning */
                if (error > 1)  /* fatal */
                    return (error);
            }
            tot_ucsize += (ULONG) ucsize;
            tot_csize += (ULONG) csize;
            ++members;

        } else {        /* not listing this file */
            if ((error = do_string(crec.file_comment_length, SKIP)) != 0) {
              error_in_archive = error;   /* might be warning */
              if (error > 1)      /* fatal */
                  return (error);
            }
        }
    }                   /* end for-loop (j: files in central directory) */

/*---------------------------------------------------------------------------
    Print footer line and totals (compressed size, uncompressed size, number
    of members in zipfile).
  ---------------------------------------------------------------------------*/

    ratio = (tot_ucsize == 0) ? 
        0 : ((tot_ucsize > 4000000) ?    /* risk unsigned overflow if mult. */
        (int) ((tot_ucsize - tot_csize) / (tot_ucsize/1000L)) + 5 :
        (int) ((tot_ucsize - tot_csize) * 1000L / tot_ucsize) + 5);

    if (quietflg < 2) {
        switch (which_hdr) {
        case 0:         /* short */
            printf("%s\n%7lu                    %-7u\n",
                   " ------                    -------",
                   tot_ucsize, members);
            break;
        case 1:         /* verbose */
            printf(
              "%s\n%7lu         %7lu %3d%%                              %-7u\n",
              " ------          ------  ---                              -------",
              tot_ucsize, tot_csize, ratio / 10, members);
        }
    }
/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

    readbuf(sig, 4);
    if (strncmp(sig, END_CENTRAL_SIG, 4)) {     /* just to make sure again */
        fprintf(stderr, EndSigMsg);  /* didn't find end-of-central-dir sig */
        fprintf(stderr, ReportMsg);  /* report to info-zip */
        error_in_archive = 1;        /* 1:  warning error */
    }
    return (error_in_archive);

}       /* end function list_files() */





/*************************************/
/*  Function extract_or_test_files() */
/*************************************/

int extract_or_test_files()
/* return PK-type error code */
{
    char **fnamev;
    byte *cd_inptr;
    int cd_incnt, error, error_in_archive = 0;
    UWORD i, j, members_remaining;
    longint cd_bufstart, bufstart, inbuf_offset;
    struct min_info {
        unsigned f_attr;
        longint offset;
        int lcflag;
    } info[DIR_BLKSIZ];



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

        /*
         * Loop through files in central directory, storing offsets, file
         * attributes, and case-conversion flags until block size is reached.
         */

        while (members_remaining && (j < DIR_BLKSIZ)) {
            --members_remaining;

            if (readbuf(sig, 4) <= 0) {
                error_in_archive = 51;  /* 51:  unexpected EOF */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if (strncmp(sig, CENTRAL_HDR_SIG, 4)) {  /* just to make sure */
                fprintf(stderr, CentSigMsg, j);  /* sig not found */
                fprintf(stderr, ReportMsg);   /* report to info-zip */
                error_in_archive = 3;   /* 3:  error in zipfile */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            /* (sets lcflag)-------v  */
            if ((error = process_central_file_header()) != 0) {
                error_in_archive = error;   /* only 51 (EOF) defined */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if ((error = do_string(crec.filename_length, FILENAME)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {  /* fatal:  no more left to do */
                    fprintf(stderr, FilNamMsg, filename, "central");
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = do_string(crec.extra_field_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {  /* fatal */
                    fprintf(stderr, ExtFieldMsg, filename, "central");
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = do_string(crec.file_comment_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {  /* fatal */
                    fprintf(stderr, "\n%s:  bad file comment length\n",
                            filename);
                    members_remaining = 0;
                    break;
                }
            }
            if (process_all_files) {
                if (crec.general_purpose_bit_flag & 1) {
                    fprintf(stderr, CryptMsg, filename);  /* encrypted: skip */
                    error_in_archive = 1;       /* 1:  warning error */
                } else {
                    ULONG tmp = ULONG_(crec.external_file_attributes);

                    switch (hostnum) {
                        case UNIX_:
                          info[j].f_attr = tmp >> 16;
                          break;
                        case DOS_OS2_FAT_:
                          tmp = (!(tmp & 1)) << 1;   /* read-only bit */
                          info[j].f_attr = 0444 | tmp<<6 | tmp<<3 | tmp;
#ifdef UNIX
                          umask((int)(tmp = umask(0)));
                          info[j].f_attr &= ~tmp;
#endif
                          break;
                        case MAC_:
                          tmp &= 1;   /* read-only bit */
                          info[j].f_attr = tmp;
                          break;
                        default:
                          info[j].f_attr = 0666;
                          break;
                    }
                    info[j].lcflag = lcflag;
                    info[j++].offset = (longint)
                        ULONG_(crec.relative_offset_local_header);
                }
            } else {
                fnamev = fnv;   /* don't destroy permanent filename pointer */
                for (--fnamev; *++fnamev;)
                    if (match(filename, *fnamev)) {
                        if (crec.version_needed_to_extract[0] > UNZIP_VERSION) {
                            fprintf(stderr, "%s:  requires compatibility with\
 version %u.%u to extract\n       (this handles %u.%u)--skipping.\n", filename,
                                crec.version_needed_to_extract[0] / 10,
                                crec.version_needed_to_extract[0] % 10,
                                UNZIP_VERSION / 10, UNZIP_VERSION % 10);
                            error_in_archive = 1;       /* 1:  warning error */
                        } else if (crec.general_purpose_bit_flag & 1) {
                            fprintf(stderr, CryptMsg, filename);  /* encrypt */
                            error_in_archive = 1;       /* 1:  warning error */
                        } else {
                            ULONG tmp = ULONG_(crec.external_file_attributes);

                            switch (hostnum) {
                                case UNIX_:
                                  info[j].f_attr = tmp >> 16;
                                  break;
                                case DOS_OS2_FAT_:
                                  tmp = (!(tmp & 1)) << 1;  /* read-only bit */
                                  info[j].f_attr = 0444 | tmp<<6 | tmp<<3 | tmp;
#ifdef UNIX
                                  umask((int)(tmp = umask(0)));
                                  info[j].f_attr &= ~tmp;
#endif
                                  break;
                                case MAC_:
                                  tmp &= 1;   /* read-only bit */
                                  info[j].f_attr = tmp;
                                  break;
                                default:
                                  info[j].f_attr = 0666;
                                  break;
                            }
                            info[j].lcflag = lcflag;
                            info[j++].offset = (longint)
                                ULONG_(crec.relative_offset_local_header);
                        }
                        break;  /* found match for filename, so stop looping */

                    }   /* end if (match), for-loop (fnamev) */
            }           /* end if (process_all_files) */
        }               /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cd_bufstart = cur_zipfile_bufstart;
        cd_inptr = inptr;
        cd_incnt = incnt;

        /*
         * Loop through files in block, extracting or testing each one.
         */

        for (i = 0; i < j; ++i) {
            /*
             * if the target position is not within the current input buffer
             * (either haven't yet read far enough, or (maybe) skipping back-
             * ward) skip to the target position and reset readbuf().
             */

            /* LSEEK(info[i].offset):  */
            inbuf_offset = info[i].offset % INBUFSIZ;
            bufstart = info[i].offset - inbuf_offset;

            if (bufstart != cur_zipfile_bufstart) {
                cur_zipfile_bufstart = lseek(zipfd, bufstart, SEEK_SET);
                if ((incnt = read(zipfd,inbuf,INBUFSIZ)) <= 0) {
                    fprintf(stderr, OffsetMsg, filename, "lseek");
                    error_in_archive = 3;   /* 3:  error in zipfile, but */
                    continue;               /*  can still do next file   */
                }
                inptr = inbuf + inbuf_offset;
                incnt -= inbuf_offset;
            } else {
                incnt += (inptr-inbuf) - inbuf_offset;
                inptr = inbuf + inbuf_offset;
            }
            lcflag = info[i].lcflag;
            f_attr = info[i].f_attr;

            /* should be in proper position now, so check for sig */
            if (readbuf(sig, 4) <= 0) {
                fprintf(stderr, OffsetMsg, filename, "EOF");  /* bad offset */
                error_in_archive = 3;   /* 3:  error in zipfile */
                continue;       /* but can still do next one */
            }
            if (strncmp(sig, LOCAL_HDR_SIG, 4)) {
                fprintf(stderr, OffsetMsg, filename,
                        "can't find local header sig");   /* bad offset */
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
                    fprintf(stderr, FilNamMsg, filename, "local");
                    continue;   /* go on to next one */
                }
            }
            if ((error = do_string(lrec.extra_field_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) {
                    fprintf(stderr, ExtFieldMsg, filename, "local");
                    continue;   /* go on */
                }
            }
            if ((error = extract_or_test_member()) != 0)
                if (error > error_in_archive)
                    error_in_archive = error;       /* ...and keep going */

        }                       /* end for-loop (i:  files in current block) */

        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */

        cur_zipfile_bufstart = lseek(zipfd, cd_bufstart, SEEK_SET);
        read(zipfd, inbuf, INBUFSIZ);   /* were already there ==> no error */
        inptr = cd_inptr;
        incnt = cd_incnt;

#ifdef TEST
        printf("\ncd_bufstart = %ld (%.8lXh)\n", cd_bufstart, cd_bufstart);
        printf("cur_zipfile_bufstart = %ld (%.8lXh)\n", cur_zipfile_bufstart,
          cur_zipfile_bufstart);
        printf("inptr-inbuf = %d\n", inptr-inbuf);
        printf("incnt = %d\n\n", incnt);
#endif

    }           /* end while-loop (blocks of files in central directory) */

/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

    readbuf(sig, 4);
    if (strncmp(sig, END_CENTRAL_SIG, 4)) {     /* just to make sure again */
        fprintf(stderr, EndSigMsg);  /* didn't find end-of-central-dir sig */
        fprintf(stderr, ReportMsg);  /* report to info-zip */
        if (!error_in_archive)       /* don't overwrite stronger error */
            error_in_archive = 1;    /* 1:  warning error */
    }
    if (tflag && (quietflg == 1)) {
        if (error_in_archive)
            printf("At least one error was detected in the archive.\n");
        else if (process_all_files)
            printf("No errors detected.\n");
        else
            printf("No errors detected in the tested files.\n");
    }
    return (error_in_archive);

}       /* end function extract_or_test_files() */





/**************************************/
/*  Function extract_or_test_member() */
/**************************************/

int extract_or_test_member()
/* return PK-type error code */
{
    int error = 0;
    UWORD b;



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
    if (aflag)                  /* if we have a scratchpad, clear it out */
        memset(outout, 0, OUTBUFSIZ);

    if (tflag) {
        if (!quietflg) {
            fprintf(stdout, "  Testing: %-12s ", filename);
            fflush(stdout);
        }
    } else {
        if (cflag)              /* output to stdout (copy of it) */
#ifdef MACOS
            outfd = 1;
#else
            outfd = dup(1);
#endif
        else {
            if ((error = mapped_name()) > 1)  /* member name conversion error */
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
    of 0's).]
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

    return (error);

}       /* end function extract_or_test_member() */





/*******************************************/
/*  Function process_central_file_header() */
/*******************************************/

int process_central_file_header()
/* return PK-type error code */
{
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
    if (readbuf((char *) &crec, CREC_SIZE) <= 0)
        return (51);

#else  /* NOTINT16 */
    if (readbuf((char *) byterec, CREC_SIZE) <= 0)
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
#endif  /* NOTINT16 */

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
/*      case ATARI_:            ??? */
/*      case Z_SYSTEM_:         ??? */
/*      case TOPS20_:           (if we had such a thing...) */

            lcflag = TRUE;      /* convert filename to lowercase */
            break;

        default:                /* AMIGA_, UNIX_, (ATARI_), OS2_HPFS_, */
            break;              /*   MAC_, (Z_SYSTEM_):  no conversion */
        }

    return (0);

}       /* end function process_central_file_header() */





/*****************************************/
/*  Function process_local_file_header() */
/*****************************************/

int process_local_file_header()
/* return PK-type error code */
{
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
    if (readbuf((char *) &lrec, LREC_SIZE) <= 0)
        return (51);

#else /* NOTINT16 */
    if (readbuf((char *) byterec, LREC_SIZE) <= 0)
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
#endif  /* NOTINT16 */

    csize = (longint) ULONG_(lrec.compressed_size);
    ucsize = (longint) ULONG_(lrec.uncompressed_size);

    return (0);                 /* 0:  no error */

}       /* end function process_local_file_header() */
