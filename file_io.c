/* v3.05 File related functions for unzip.c */

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
    static int do_all = 0;
    char answerbuf[10];
    UWORD holder;

    if (cflag) {        /* output to stdout (a copy of it, really) */
        outfd = dup(1);
        return 0;
        }
    CR_flag = 0;	/* Hack to get CR at end of buffer working. */

    /* 
     * check if the file exists, unless do_all 
     * ask before overwrite code by Bill Davidsen (davidsen@crdos1.crd.ge.com)
     */
    if (!do_all) {
        outfd = open(filename, 0);
	if (outfd >= 0) {
	    /* first close it, before you forget! */
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
	        while(ReadByte(&holder));
		return 1; /* it's done! */
	    }
	}
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

/* ------------------------------------------------------------- */

int dos2unix (buf, len)
unsigned char *buf;
int len;
{
    int new_len;
    int i;
    unsigned char *walker;

    new_len = len;
    walker = outout;
    if (CR_flag && *buf != LF)
        *walker++ = ascii_to_native(CR);
    CR_flag = buf[len - 1] == CR;
    for (i = 0; i < len; i += 1) {
        *walker++ = *buf;
        if (*buf++ == CR && *buf == LF) {
            new_len--;
            walker[-1] = ascii_to_native(*buf++);
            i++;
        }
    }
    /*
     * If the last character is a CR, then "ignore it" for now...
     */
    if (walker[-1] == CR)
        new_len--;
    return new_len;
}

void WriteBuffer(fd, buf, len)
int fd;
unsigned char *buf;
int len;
{
     if (aflag)
         len = dos2unix (buf, len);
     if (write (fd, outout, len) != len) {
         fprintf (stderr, "Fatal write error.\n");
         exit (1);
     }
}


/* ------------------------------------------------------------- */

void FlushOutput()
 /* flush contents of output buffer */
{
    if (outcnt) {
        UpdateCRC(outbuf, outcnt);

        if (!tflag)
            WriteBuffer(outfd, outbuf, outcnt);

        outpos += outcnt;
        outcnt = 0;
        outptr = outbuf;
    }
}

