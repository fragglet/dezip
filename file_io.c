/*---------------------------------------------------------------------------

  file_io.c

  This file contains routines for doing direct input/output, file-related
  sorts of things.

  ---------------------------------------------------------------------------*/


#include "unzip.h"


/************************************/
/*  File_IO Includes, Defines, etc. */
/************************************/

#ifdef VMS
#include <rms.h>                /* RMS prototypes, error codes, etc. */
#include <ssdef.h>              /* system services error codes */
#include <descrip.h>            /* "descriptor" format stuff */
#endif

static int WriteBuffer __((int fd, unsigned char *buf, int len));
static int dos2unix __((unsigned char *buf, int len));

int CR_flag = 0;        /* when last char of buffer == CR (for dos2unix()) */





/*******************************/
/*  Function open_input_file() */
/*******************************/

int open_input_file()
{                               /* return non-0 if open failed */
    /*
     *  open the zipfile for reading and in BINARY mode to prevent cr/lf
     *  translation, which would corrupt the bitstreams
     */

#ifndef UNIX
    zipfd = open(zipfn, O_RDONLY | O_BINARY);
#else
    zipfd = open(zipfn, O_RDONLY);
#endif
    if (zipfd < 1) {
        fprintf(stderr, "error:  can't open zipfile [ %s ]\n", zipfn);
        return (1);
    }
    return 0;
}





/************************/
/*  Function readbuf()  */
/************************/

int readbuf(buf, size)
char *buf;
register unsigned size;
{                               /* return number of bytes read into buf */
    register int count;
    int n;

    n = size;
    while (size) {
        if (incnt == 0) {
            if ((incnt = read(zipfd, inbuf, INBUFSIZ)) <= 0)
                return (n-size);
            /* buffer ALWAYS starts on a block boundary:  */
            cur_zipfile_bufstart += INBUFSIZ;
            inptr = inbuf;
        }
        count = min(size, incnt);
        memcpy(buf, inptr, count);
        buf += count;
        inptr += count;
        incnt -= count;
        size -= count;
    }
    return (n);
}





#ifdef VMS

/**********************************/
/*  Function create_output_file() */
/**********************************/

int create_output_file()
{                               /* return non-0 if sys$create failed */
    /*
     * VMS VERSION (generic version is below)
     *
     * Create the output file and set its date/time using VMS Record Management
     * Services From Hell.  Then reopen for appending with normal Unix/C-type
     * I/O functions.  This is the EASY way to set the file date/time under VMS.
     */
    int ierr, yr, mo, dy, hh, mm, ss;
    char timbuf[24];            /* length = first entry in "stupid" + 1 */
    struct FAB fileblk;
    struct XABDAT dattim;
    static char *month[] =
    {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
/*  fixed-length string descriptor (why not just a pointer to timbuf? sigh.) */
    struct dsc$descriptor stupid =
    {23, DSC$K_DTYPE_T, DSC$K_CLASS_S, timbuf};



/*---------------------------------------------------------------------------
    First initialize the necessary RMS and date/time variables.  "FAB" stands
    for "file attribute block," "XAB" for "extended attribute block."  Files
    under VMS are usually in "variable-length records" format with "carriage-
    return carriage control" (at least for text files).  Unfortunately, some-
    where along the line extra "carriage returns" (i.e., line feed characters)
    get stuck in files which are opened in the variable format.  This may be
    a VMS problem, an RMS problem, or a Unix/C I/O problem, but every 8192
    characters of text file is followed by a spurious LF, and more often than
    that for binary files.  So we use the stream-LF format instead (which is
    what the Unix/C I/O routines do by default).  EDT complains about such
    files but goes ahead and edits them; TPU (Adam, Eve) and vi don't seem
    to care at all.
  ---------------------------------------------------------------------------*/

    yr = ((lrec.last_mod_file_date >> 9) & 0x7f) + 1980; /* dissect date */
    mo = ((lrec.last_mod_file_date >> 5) & 0x0f) - 1;
    dy = (lrec.last_mod_file_date & 0x1f);
    hh = (lrec.last_mod_file_time >> 11) & 0x1f;        /* dissect time */
    mm = (lrec.last_mod_file_time >> 5) & 0x3f;
    ss = (lrec.last_mod_file_time & 0x1f) * 2;

    fileblk = cc$rms_fab;               /* fill FAB with default values */
    fileblk.fab$l_fna = filename;       /* l_fna, b_fns are the only re- */
    fileblk.fab$b_fns = strlen(filename); /*  quired user-supplied fields */
    fileblk.fab$b_rfm = FAB$C_STMLF;    /* stream-LF record format */
    fileblk.fab$b_rat = FAB$M_CR;       /* carriage-return carriage ctrl */
    /*                      ^^^^ *NOT* V_CR!!!     */
    fileblk.fab$l_xab = &dattim;        /* chain XAB to FAB */
    dattim = cc$rms_xabdat;             /* fill XAB with default values */

    CR_flag = 0;                /* Hack to get CR at end of buffer working
                                   (dos2unix) */

/*---------------------------------------------------------------------------
    Next convert date into an ASCII string, then use a VMS service to con-
    vert the string into internal time format.  Obviously this is a bit of a
    kludge, but I have absolutely NO intention of figuring out how to convert
    MS-DOS time into tenths of microseconds elapsed since freaking 17 Novem-
    ber 1858!!  Particularly since DEC doesn't even have a native 64-bit data
    type.  Bleah.
  ---------------------------------------------------------------------------*/

    sprintf(timbuf, "%02d-%3s-%04d %02d:%02d:%02d.00", dy, month[mo], yr,
            hh, mm, ss);

/*  "xab$q_cdt" is the XAB field which holds the file's creation date/time */
    sys$bintim(&stupid, &dattim.xab$q_cdt);

/*---------------------------------------------------------------------------
    Next create the file under RMS.  If sys$create chokes with an error of
    RMS$_SYN (syntax error), it's probably because a Unix-style directory was
    specified, so try to create the file again using the regular creat() func-
    tion (date/time won't be set properly in this case, obviously).
  ---------------------------------------------------------------------------*/

    if ((ierr = sys$create(&fileblk)) != RMS$_NORMAL)
        if (ierr == RMS$_SYN) { /* try Unix/C create:  0 = default perms */
            outfd = creat(filename, 0, "rfm=stmlf", "rat=cr");
            if (outfd < 1) {
                fprintf(stderr, "Can't create output file:  %s\n", filename);
                return (1);
            } else {
                return (0);
            }
        } else {                /* probably access violation */
            fprintf(stderr, "Can't create output file:  %s\n", filename);
            return (1);
        }

/*---------------------------------------------------------------------------
    Finally, close the file under RMS and reopen with Unix/C open() function.
  ---------------------------------------------------------------------------*/

    sys$close(&fileblk);
    outfd = open(filename, O_RDWR);

    return (0);
}





#else                           /* !VMS */

/**********************************/
/*  Function create_output_file() */
/**********************************/

int create_output_file()
{                               /* return non-0 if creat failed */
    /*
     * GENERIC VERSION (special VMS version is above)
     *
     * Create the output file with default permissions.
     */
    extern int do_all;
    char answerbuf[10];
    UWORD holder;



    CR_flag = 0;                /* Hack to get CR at end of buffer working. */

    /*
     * check if the file exists, unless do_all
     */
    if (!do_all) {
        outfd = open(filename, 0);
        if (outfd >= 0) {       /* first close it, before you forget! */
            close(outfd);

            /* ask the user before blowing it away */
            fprintf(stderr, "replace %s, y-yes, n-no, a-all: ", filename);
            fgets(answerbuf, 9, stdin);

            switch (answerbuf[0]) {
            case 'y':
            case 'Y':
                break;
            case 'a':
            case 'A':
                do_all = 1;
                break;
            case 'n':
            case 'N':
            default:
                while (ReadByte(&holder));
                return 1;       /* it's done! */
            }
        }
    }
#ifndef UNIX
    outfd = creat(filename, (S_IWRITE | S_IREAD) & f_attr);
#else
    {
      int mask;
      mask = umask(0);
      outfd = creat(filename, 0777 & f_attr);
      umask(mask);
    }
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
#ifdef THINK_C
    /*
     * THINKC's stdio routines have the horrible habit of
     * making any file you open look like generic files
     * this code tells the OS that it's a text file
     */
    if (aflag) {
        fileParam pb;
        OSErr err;

        CtoPstr(filename);
        pb.ioNamePtr = filename;
        pb.ioVRefNum = 0;
        pb.ioFVersNum = 0;
        pb.ioFDirIndex = 0;
        err = PBGetFInfo(&pb,0);
        if (err == noErr) {
            pb.ioFlFndrInfo.fdCreator = '????';
            pb.ioFlFndrInfo.fdType = 'TEXT';
            err = PBSetFInfo(&pb, 0);
        }
        PtoCstr(filename);
    }
#endif
    if (!aflag) {
        close(outfd);
        outfd = open(filename, O_RDWR | O_BINARY);
    }
#endif
    return 0;
}

#endif                          /* !VMS */





/*****************************/
/*  Function FillBitBuffer() */
/*****************************/

int FillBitBuffer(bits)
register int bits;
{
    /*
     * Get the bits that are left and read the next UWORD.  This
     * function is only used by the READBIT macro (which is used
     * by all of the uncompression routines).
     */
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





/************************/
/*  Function ReadByte() */
/************************/

int ReadByte(x)
UWORD *x;
{
    /*
     * read a byte; return 8 if byte available, 0 if not
     */


    if (csize-- <= 0)
        return 0;

    if (incnt == 0) {
        if ((incnt = read(zipfd, inbuf, INBUFSIZ)) <= 0)
            return 0;
        /* buffer ALWAYS starts on a block boundary:  */
        cur_zipfile_bufstart += INBUFSIZ;
        inptr = inbuf;
    }
    *x = *inptr++;
    --incnt;
    return 8;
}



#ifdef FLUSH_AND_WRITE
/***************************/
/*  Function FlushOutput() */
/***************************/

int FlushOutput()
{                               /* return PK-type error code */
    /* flush contents of output buffer */
    /*
     * This combined version doesn't work, and I sure can't see why not...
     * probably something stupid, but how much can you screw up in 6 lines???
     * [optimization problem??]
     */
    int len;


    if (outcnt) {
        UpdateCRC(outbuf, outcnt);

        if (!tflag) {
            if (aflag)
                len = dos2unix(outbuf, outcnt);
            if (write(outfd, outout, len) != len) {
                fprintf(stderr, "Fatal write error.\n");
                return (50);    /* 50:  disk full */
            }
        }
        outpos += outcnt;
        outcnt = 0;
        outptr = outbuf;
    }
    return (0);                 /* 0:  no error */
}

#else                           /* separate flush and write routines */
/***************************/
/*  Function FlushOutput() */
/***************************/

int FlushOutput()
{                               /* return PK-type error code */
    /* flush contents of output buffer */
    if (outcnt) {
        UpdateCRC(outbuf, outcnt);

        if (!tflag && WriteBuffer(outfd, outbuf, outcnt))
            return (50);        /* 50:  disk full */

        outpos += outcnt;
        outcnt = 0;
        outptr = outbuf;
    }
    return (0);                 /* 0:  no error */
}

/***************************/
/*  Function WriteBuffer() */
/***************************/

static int WriteBuffer(fd, buf, len)    /* return 0 if successful, 1 if not */
int fd;
unsigned char *buf;
int len;
{
    if (aflag)
        len = dos2unix(buf, len);
    if (write(fd, outout, len) != len) {
#ifdef DOS_OS2
        if (!cflag) {           /* ^Z treated as EOF, removed with -c */
#endif
            fprintf(stderr, "Fatal write error.\n");
            return (1);         /* FAILED */
#ifdef DOS_OS2
        }
#endif
    }
    return (0);
}

#endif




/************************/
/*  Function dos2unix() */
/************************/

static int dos2unix(buf, len)
unsigned char *buf;
int len;
{
    int new_len;
    int i;
    unsigned char *walker;

    new_len = len;
    walker = outout;
#ifdef MACOS
    /*
     * Mac wants to strip LFs instead CRs from CRLF pairs
     */
    if (CR_flag && *buf == LF) {
        buf++;
        new_len--;
        len--;
        CR_flag = buf[len] == CR;
    }
    else
        CR_flag = buf[len - 1] == CR;
    for (i = 0; i < len; i += 1) {
        *walker++ = ascii_to_native(*buf);
        if (*buf == LF) walker[-1] = CR;
        if (*buf++ == CR && *buf == LF) {
            new_len--;
            buf++;
            i++;
        }
    }
#else
    if (CR_flag && *buf != LF)
        *walker++ = ascii_to_native(CR);
    CR_flag = buf[len - 1] == CR;
    for (i = 0; i < len; i += 1) {
        *walker++ = ascii_to_native(*buf);
        if (*buf++ == CR && *buf == LF) {
            new_len--;
            walker[-1] = ascii_to_native(*buf++);
            i++;
        }
    }
    /*
     * If the last character is a CR, then "ignore it" for now...
     */
    if (walker[-1] == ascii_to_native(CR))
        new_len--;
#endif
    return new_len;
}





#ifdef DOS_OS2

/***************************************/
/*  Function set_file_time_and_close() */
/***************************************/

void set_file_time_and_close()
 /*
  * MS-DOS AND OS/2 VERSION (Mac, Unix versions are below)
  *
  * Set the output file date/time stamp according to information from the
  * zipfile directory record for this member, then close the file.  This
  * is optional and can be deleted if your compiler does not easily support
  * setftime().
  */
{
/*---------------------------------------------------------------------------
    Allocate local variables needed by OS/2 and Turbo C.  [OK, OK, so it's
    a bogus comment...but this routine was getting way too cluttered and
    needed some visual separators.  Bleah.]
  ---------------------------------------------------------------------------*/

#ifdef OS2              /* (assuming only MSC or MSC-compatible compilers
                         * for this part) */

    union {
        FDATE fd;               /* system file date record */
        UWORD zdate;            /* date word */
    } ud;

    union {
        FTIME ft;               /* system file time record */
        UWORD ztime;            /* time word */
    } ut;

    FILESTATUS fs;

#else                           /* !OS2 */
#ifdef __TURBOC__

    union {
        struct ftime ft;        /* system file time record */
        struct {
            UWORD ztime;        /* date and time words */
            UWORD zdate;        /* .. same format as in .ZIP file */
        } zt;
    } td;

#endif                          /* __TURBOC__ */
#endif                          /* !OS2 */

    /*
     * Do not attempt to set the time stamp on standard output
     */
    if (cflag) {
        close(outfd);
        return;
    }


/*---------------------------------------------------------------------------
    Copy and/or convert time and date variables, if necessary; then set the
    file time/date.
  ---------------------------------------------------------------------------*/

#ifdef OS2

    DosQFileInfo(outfd, 1, &fs, sizeof(fs));
    ud.zdate = lrec.last_mod_file_date;
    fs.fdateLastWrite = ud.fd;
    ut.ztime = lrec.last_mod_file_time;
    fs.ftimeLastWrite = ut.ft;
    DosSetFileInfo(outfd, 1, &fs, sizeof(fs));

#else                           /* !OS2 */
#ifdef __TURBOC__

    td.zt.ztime = lrec.last_mod_file_time;
    td.zt.zdate = lrec.last_mod_file_date;
    setftime(outfd, &td.ft);

#else                           /* !__TURBOC__:  MSC MS-DOS */

    _dos_setftime(outfd, lrec.last_mod_file_date, lrec.last_mod_file_time);

#endif                          /* !__TURBOC__ */
#endif                          /* !OS2 */

/*---------------------------------------------------------------------------
    And finally we can close the file...at least everybody agrees on how to
    do *this*.  I think...
  ---------------------------------------------------------------------------*/

    close(outfd);
}





#else                           /* !DOS_OS2 ... */
#ifndef VMS                     /* && !VMS (already done) ... */
#ifndef MTS                     /* && !MTS (can't do):  Mac or UNIX */
#ifdef MACOS                    /* Mac first */

/***************************************/
/*  Function set_file_time_and_close() */
/***************************************/

void set_file_time_and_close()
 /*
  * MAC VERSION
  *
  * First close the output file, then set its date/time stamp according
  * to information from the zipfile directory record for this file.  [So
  * technically this should be called "close_file_and_set_time()", but
  * this way we can use the same prototype for either case, without extra
  * #ifdefs.  So there.]
  */
{
    long m_time;
    DateTimeRec dtr;
    ParamBlockRec pbr;
    OSErr err;

    if (outfd != 1)
    {
        close(outfd);

        /*
         * Macintosh bases all file modification times on the number of seconds
         * elapsed since Jan 1, 1904, 00:00:00.  Therefore, to maintain
         * compatibility with MS-DOS archives, which date from Jan 1, 1980,
         * with NO relation to GMT, the following conversions must be made:
         *      the Year (yr) must be incremented by 1980;
         *      and converted to seconds using the Mac routine Date2Secs(),
         *      almost similar in complexity to the Unix version :-)
         *                                     J. Lee
         */

        dtr.year = (((lrec.last_mod_file_date >> 9) & 0x7f) + 1980); /* dissect date */
        dtr.month = ((lrec.last_mod_file_date >> 5) & 0x0f);
        dtr.day = (lrec.last_mod_file_date & 0x1f);

        dtr.hour = ((lrec.last_mod_file_time >> 11) & 0x1f);      /* dissect time */
        dtr.minute = ((lrec.last_mod_file_time >> 5) & 0x3f);
        dtr.second = ((lrec.last_mod_file_time & 0x1f) * 2);
        Date2Secs(&dtr, &m_time);
        CtoPstr(filename);
        pbr.fileParam.ioNamePtr = filename;
        pbr.fileParam.ioVRefNum = pbr.fileParam.ioFVersNum = pbr.fileParam.ioFDirIndex = 0;
        err = PBGetFInfo(&pbr, 0L);
        pbr.fileParam.ioFlMdDat = pbr.fileParam.ioFlCrDat = m_time;
        if (err == noErr) {
            err = PBSetFInfo(&pbr, 0L);
        }
        if (err != noErr) {
            printf("Error, can't set the time for %s\n",filename);
        }

        /* set read-only perms if needed */
        if (err != noErr && f_attr != 0) {
            err = SetFLock(filename, 0);
        }
        PtoCstr(filename);
    }
}





#else                           /* !MACOS:  only one left is UNIX */

/***************************************/
/*  Function set_file_time_and_close() */
/***************************************/

void set_file_time_and_close()
 /*
  * UNIX VERSION (MS-DOS & OS/2, Mac versions are above)
  *
  * First close the output file, then set its date/time stamp according
  * to information from the zipfile directory record for this file.  [So
  * technically this should be called "close_file_and_set_time()", but
  * this way we can use the same prototype for either case, without extra
  * #ifdefs.  So there.]
  */
{
    long m_time;
    int yr, mo, dy, hh, mm, ss, leap, days = 0;
    struct utimbuf {
      time_t atime;             /* New access time */
      time_t mtime;             /* New modification time */
    } tp;
#ifdef BSD
    static struct timeb tbp;
#else
    extern long timezone;
#endif


    close(outfd);

    if (cflag)                  /* can't set time on stdout */
        return;

    /*
     * These date conversions look a little weird, so I'll explain.
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

    yr = (((lrec.last_mod_file_date >> 9) & 0x7f) + 10); /* dissect date */
    mo = ((lrec.last_mod_file_date >> 5) & 0x0f);
    dy = ((lrec.last_mod_file_date & 0x1f) - 1);

    hh = ((lrec.last_mod_file_time >> 11) & 0x1f);      /* dissect time */
    mm = ((lrec.last_mod_file_time >> 5) & 0x3f);
    ss = ((lrec.last_mod_file_time & 0x1f) * 2);

    /* leap = # of leap years from 1970 up to but not including
       the current year */

    leap = ((yr + 1969) / 4);   /* Leap year base factor */

    /* How many days from 1970 to this year? */
    days = (yr * 365) + (leap - 492);

    switch (mo) {               /* calculate expired days this year */
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
        days += 28;             /* account for leap years (2000 IS one) */
        if (((yr + 1970) % 4 == 0) && (yr + 1970) != 2100)  /* OK thru 2199 */
            ++days;
    case 2:
        days += 31;
    }

    /* convert date & time to seconds relative to 00:00:00, 01/01/1970 */
    m_time = ((days + dy) * 86400) + (hh * 3600) + (mm * 60) + ss;

#ifdef BSD
   ftime(&tbp);
   m_time += tbp.timezone * 60L;
#else                                   /* !BSD */
    tzset();                            /* Set `timezone'. */
    m_time += timezone;                 /* account for timezone differences */
#endif

    if (localtime(&m_time)->tm_isdst)
        m_time -= 60L * 60L;            /* Adjust for daylight savings time */

    /* set the time stamp on the file */
    
    tp.mtime = m_time;                  /* Set modification time */
    tp.atime = m_time;                  /* Set access time */

    if (utime(filename, &tp))
        fprintf(stderr, "Error, can't set the time for %s\n",filename);
}

#endif                          /* ?MACOS */
#endif                          /* ?MTS */
#endif                          /* ?VMS */
#endif                          /* ?DOS_OS2 */
