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
 * REVISION HISTORY : See history.307 (or whatever current version is)
 *
 */

#include "unzip.h"	/* v3.05 a BUNCH of ifdefs, etc.
			 * split out to reduce file size.
			 * David Kirschbaum
			 */

char *fnames[2] = { /* default filenames vector */
    "*",
    NULL
};
char **fnv = &fnames[0];

int tflag;      /* -t: test */
int vflag;      /* -v: view directory */
int cflag;      /* -c: output to stdout (JC) */
int aflag;      /* -a: do ascii to ebcdic translation 2.0f */
                /*     OR <cr><nl> to <nl> conversion  */
int mflag;	/* -m: map member filenames to lower case v2.0j */
int CR_flag = 0; /* When last char of buffer == CR */

int members;
longint csize;
longint ucsize;
longint tot_csize;
longint tot_ucsize;


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

/* Code now begins ( .. once more into the Valley of Death.. ) */


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
/* ----------------------------------------------------------- */

#include "file_io.c"		/* v3.05 file-related vars, functions */

#include "unreduce.c"		/* v3.05 */

#include "unshrink.c"		/* v3.05 */

#include "unimplod.c"		/* v3.05 */


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
            return; /* was exit(1); */
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
    FlushOutput ();

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
/* v3.04 Patch to enable processing of self-extracting ".EXE"
 * (and other) files that might have weird junk before the first
 * actual file member.
 * I don't THINK anyone'll have problems with this .. but just in case,
 * you can disable the entire mess by enabling the "NOSKIP" ifdef.
 * (up near code top).
 * Thanks to Warner Losh for this patch.
 */
#ifndef NOSKIP
void skip_to_signature()
{
    static char pk[] = "PK";
    int i, nread;
    unsigned char ch;
    extern int errno;
	
    errno = 0;			/* Be sure we start with 0 */
    do {
        /*
         * Search for "PK"
         */
        i = 0;
        while ((nread = read (zipfd, &ch, 1)) && i < 2) {
            if (ch == pk[i]) {
                i++;
            }
            else {
                if (ch == pk[0])
                    i = 1;
                else
                    i = 0;
            }
        }
	if (errno || nread==0) {	/* read err or EOF */
	    fprintf(stderr, "Unable to find a valid header signature. Aborting.\n");
	    exit (2);
	}
    } while (ch > 20);

    /*
     * We have now read 3 characters too many, so we backup.
     */
    lseek (zipfd, -3L, SEEK_CUR);
}
#endif		/* NOSKIP */


void process_zipfile()
{
    /*
     * open the zipfile for reading and in BINARY mode to prevent cr/lf
     * translation, which would corrupt the bitstreams
     */

    if (open_input_file())
        exit(1);

#ifndef NOSKIP		/* v3.03 */
    skip_to_signature();	/* read up to first "PK%" v3.03 */
#endif
    process_headers();

    close(zipfd);
}

/* ---------------------------------------------------------- */

void usage()        /* v2.0j */
{

#ifdef EBCDIC                              /* A.B. 03/20/90   */
  char *astring = "-a  ascii to ebcdic conversion";
#else
  char *astring = "-a  convert to unix textfile format (CR LF => LF)";	/* v3.04 */
#endif

fprintf(stderr, "\n%s\nCourtesy of:  S.H.Smith  and  The Tool Shop BBS,  (602) 279-2673.\n\n",VERSION);
fprintf(stderr, "Usage:  unzip [-tcamv] file[.zip] [filespec...]\n\
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
    struct stat statbuf;		/* v3.03 */
    
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

#ifdef REALOLDSTUFF		/* v3.02 */
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
#else				/* v3.02 */
      /*
       * OK, first check to see if the name is as given.  If it is
       * found as given, then we don't need to bother adding the
       * ZSUFX.  If it isn't found, then add the ZSUFX.  We don't
       * check to see if this results in a good name, but that check
       * will be done later.
       */
    if(stat (zipfn, &statbuf))		/* v3.02 */
        strcat (zipfn, ZSUFX);		/* v3.02 */

#endif	/* not REALOLDSTUF */
      
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

