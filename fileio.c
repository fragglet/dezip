/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/*---------------------------------------------------------------------------

  This file contains routines for doing direct but relatively generic input/
  output, file-related sorts of things, plus some miscellaneous stuff.  Most
  of the stuff has to do with opening, closing, reading and/or writing files.

  ---------------------------------------------------------------------------*/

#include "unzip.h"
#include "crc32.h"
#include "crypt.h"
#include "ttyio.h"

/* setup of codepage conversion for decryption passwords */
#if (defined(CRYP_USES_ISO2OEM) && !defined(IZ_ISO2OEM_ARRAY))
#define IZ_ISO2OEM_ARRAY /* pull in iso2oem[] table */
#endif
#if (defined(CRYP_USES_OEM2ISO) && !defined(IZ_OEM2ISO_ARRAY))
#define IZ_OEM2ISO_ARRAY /* pull in oem2iso[] table */
#endif
#include "ebcdic.h" /* definition/initialization of ebcdic[] */

#define WriteError(buf, len, strm)                                   \
    ((size_t) write(fileno(strm), (char *) (buf), (size_t) (len)) != \
     (size_t) (len))

#define WriteTxtErr(buf, len, strm) WriteError(buf, len, strm)

static int disk_error(void);

static const char CannotOpenZipfile[] =
    "error:  cannot open zipfile [ %s ]\n        %s\n";

static const char CannotDeleteOldFile[] =
    "error:  cannot delete old %s\n        %s\n";
static const char CannotCreateFile[] = "error:  cannot create %s\n        %s\n";

static const char ReadError[] = "error:  zipfile read error\n";
static const char FilenameTooLongTrunc[] =
    "warning:  filename too long--truncating.\n";
static const char UFilenameCorrupt[] = "error: Unicode filename corrupt.\n";
static const char UFilenameTooLongTrunc[] =
    "warning:  Converted Unicode filename too long--truncating.\n";
static const char ExtraFieldTooLong[] =
    "warning:  extra field too long (%d).  Ignoring...\n";
static const char ExtraFieldCorrupt[] =
    "warning:  extra field (type: 0x%04x) corrupt.  Continuing...\n";

static const char DiskFullQuery[] =
    "%s:  write error (disk full?).  Continue? (y/n/^C) ";
static const char FileIsSymLink[] = "%s exists and is a symbolic link%s.\n";
static const char QuitPrompt[] =
    "--- Press `Q' to quit, or any other key to continue ---";
static const char HidePrompt[] = /* "\r                       \r"; */
    "\r                                                         \r";
static const char PasswPrompt[] = "[%s] %s password: ";
static const char PasswPrompt2[] = "Enter password: ";
static const char PasswRetry[] = "password incorrect--reenter: ";

int open_input_file() /* return 1 if open failed */
{
    /*
     *  open the zipfile for reading and in BINARY mode to prevent cr/lf
     *  translation, which would corrupt the bitstreams
     */

    G.zipfd = open(G.zipfn, O_RDONLY | O_BINARY);

    /* if (G.zipfd < 0) */ /* no good for Windows CE port */
    if (G.zipfd == -1) {
        Info(slide, 1,
             ((char *) slide, CannotOpenZipfile, G.zipfn, strerror(errno)));
        return 1;
    }
    return 0;
}

int open_outfile() /* return 1 if fail */
{
    mode_t umask_sav;
    if (stat(G.filename, &G.statbuf) == 0 ||
        lstat(G.filename, &G.statbuf) == 0) {
        Trace((stderr, "open_outfile:  stat(%s) returns 0:  file exists\n",
               FnFilter1(G.filename)));
        if (unlink(G.filename) != 0) {
            Info(slide, 1,
                 ((char *) slide, CannotDeleteOldFile, FnFilter1(G.filename),
                  strerror(errno)));
            return 1;
        }
        Trace(
            (stderr, "open_outfile:  %s now deleted\n", FnFilter1(G.filename)));
    }
#ifdef DEBUG
    Info(slide, 1,
         ((char *) slide, "open_outfile:  doing fopen(%s) for reading\n",
          FnFilter1(G.filename)));
    if ((G.outfile = fopen(G.filename, "rb")) == NULL)
        Info(slide, 1,
             ((char *) slide,
              "open_outfile:  fopen(%s) for reading failed:  does not exist\n",
              FnFilter1(G.filename)));
    else {
        Info(slide, 1,
             ((char *) slide,
              "open_outfile:  fopen(%s) for reading succeeded:  file exists\n",
              FnFilter1(G.filename)));
        fclose(G.outfile);
    }
#endif /* DEBUG */
    Trace((stderr, "open_outfile:  doing fopen(%s) for writing\n",
           FnFilter1(G.filename)));
    umask_sav = umask(0077);
    /* These features require the ability to re-read extracted data from
       the output files. Output files are created with Read&Write access.
     */
    G.outfile = fopen(G.filename, "w+b");
    umask(umask_sav);
    if (G.outfile == NULL) {
        Info(slide, 1,
             ((char *) slide, CannotCreateFile, FnFilter1(G.filename),
              strerror(errno)));
        return 1;
    }
    Trace((stderr, "open_outfile:  fopen(%s) for writing succeeded\n",
           FnFilter1(G.filename)));

    return 0;
}

/*
 * These functions allow NEXTBYTE to function without needing two bounds
 * checks.  Call defer_leftover_input() if you ever have filled G.inbuf
 * by some means other than readbyte(), and you then want to start using
 * NEXTBYTE.  When going back to processing bytes without NEXTBYTE, call
 * undefer_input().  For example, extract_or_test_member brackets its
 * central section that does the decompression with these two functions.
 * If you need to check the number of bytes remaining in the current
 * file while using NEXTBYTE, check (G.csize + G.incnt), not G.csize.
 */

void undefer_input()
{
    if (G.incnt > 0)
        G.csize += G.incnt;
    if (G.incnt_leftover > 0) {
        /* We know that "(G.csize < MAXINT)" so we can cast G.csize to int:
         * This condition was checked when G.incnt_leftover was set > 0 in
         * defer_leftover_input(), and it is NOT allowed to touch G.csize
         * before calling undefer_input() when (G.incnt_leftover > 0)
         * (single exception: see readbyte()'s  "G.csize <= 0" handling) !!
         */
        if (G.csize < 0L)
            G.csize = 0L;
        G.incnt = G.incnt_leftover + (int) G.csize;
        G.inptr = G.inptr_leftover - (int) G.csize;
        G.incnt_leftover = 0;
    } else if (G.incnt < 0)
        G.incnt = 0;
}

void defer_leftover_input()
{
    if ((off_t) G.incnt > G.csize) {
        /* (G.csize < MAXINT), we can safely cast it to int !! */
        if (G.csize < 0L)
            G.csize = 0L;
        G.inptr_leftover = G.inptr + (int) G.csize;
        G.incnt_leftover = G.incnt - (int) G.csize;
        G.incnt = (int) G.csize;
    } else
        G.incnt_leftover = 0;
    G.csize -= G.incnt;
}

/* return number of bytes read into buf */
unsigned readbuf(buf, size)
char *buf;
register unsigned size;
{
    register unsigned count;
    unsigned n;

    n = size;
    while (size) {
        if (G.incnt <= 0) {
            if ((G.incnt = read(G.zipfd, (char *) G.inbuf, INBUFSIZ)) == 0)
                return (n - size);
            else if (G.incnt < 0) {
                /* another hack, but no real harm copying same thing twice */
                (*G.message)((uint8_t *) ReadError, /* CANNOT use slide */
                             (uint32_t) strlen(ReadError), 1);
                return 0; /* discarding some data; better than lock-up */
            }
            /* buffer ALWAYS starts on a block boundary:  */
            G.cur_zipfile_bufstart += INBUFSIZ;
            G.inptr = G.inbuf;
        }
        count = MIN(size, (unsigned) G.incnt);
        memcpy(buf, G.inptr, count);
        buf += count;
        G.inptr += count;
        G.incnt -= count;
        size -= count;
    }
    return n;
}

/* refill inbuf and return a byte if available, else EOF */
int readbyte()
{
    if (G.mem_mode)
        return EOF;
    if (G.csize <= 0) {
        G.csize--; /* for tests done after exploding */
        G.incnt = 0;
        return EOF;
    }
    if (G.incnt <= 0) {
        if ((G.incnt = read(G.zipfd, (char *) G.inbuf, INBUFSIZ)) == 0) {
            return EOF;
        } else if (G.incnt < 0) { /* "fail" (abort, retry, ...) returns this */
            /* another hack, but no real harm copying same thing twice */
            (*G.message)((uint8_t *) ReadError, (uint32_t) strlen(ReadError),
                         1);
            echon();
            exit(PK_BADERR); /* totally bailing; better than lock-up */
        }
        G.cur_zipfile_bufstart += INBUFSIZ; /* always starts on block bndry */
        G.inptr = G.inbuf;
        defer_leftover_input(); /* decrements G.csize */
    }

    if (G.pInfo->encrypted) {
        uint8_t *p;
        int n;

        /* This was previously set to decrypt one byte beyond G.csize, when
         * incnt reached that far.  GRR said, "but it's required:  why?"  This
         * was a bug in fillinbuf() -- was it also a bug here?
         */
        for (n = G.incnt, p = G.inptr; n--; p++)
            zdecode(*p);
    }

    --G.incnt;
    return *G.inptr++;
}

/* like readbyte() except returns number of bytes in inbuf */
int fillinbuf()
{
    if (G.mem_mode ||
        (G.incnt = read(G.zipfd, (char *) G.inbuf, INBUFSIZ)) <= 0)
        return 0;
    G.cur_zipfile_bufstart += INBUFSIZ; /* always starts on a block boundary */
    G.inptr = G.inbuf;
    defer_leftover_input(); /* decrements G.csize */

    if (G.pInfo->encrypted) {
        uint8_t *p;
        int n;

        for (n = G.incnt, p = G.inptr; n--; p++)
            zdecode(*p);
    }

    return G.incnt;
}

int seek_zipf(abs_offset)
off_t abs_offset;
{
    /*
     *  Seek to the block boundary of the block which includes abs_offset,
     *  then read block into input buffer and set pointers appropriately.
     *  If block is already in the buffer, just set the pointers.  This function
     *  is used by do_seekable (process.c), extract_or_test_entrylist
     * (extract.c) and do_string (fileio.c).  Also, a slightly modified version
     * is embedded within extract_or_test_entrylist (extract.c).  readbyte() and
     * readbuf() (fileio.c) are compatible.  NOTE THAT abs_offset is intended to
     * be the "proper offset" (i.e., if there were no extra bytes prepended);
     *  cur_zipfile_bufstart contains the corrected offset.
     *
     *  Since seek_zipf() is never used during decompression, it is safe to
     *  use the slide[] buffer for the error message.
     *
     * returns PK error codes:
     *  PK_BADERR if effective offset in zipfile is negative
     *  PK_EOF if seeking past end of zipfile
     *  PK_OK when seek was successful
     */
    off_t request = abs_offset + G.extra_bytes;
    off_t inbuf_offset = request % INBUFSIZ;
    off_t bufstart = request - inbuf_offset;

    if (request < 0) {
        Info(slide, 1, ((char *) slide, SEEK_MSG, G.zipfn));
        return (PK_BADERR);
    } else if (bufstart != G.cur_zipfile_bufstart) {
        Trace((stderr, "fpos_zip: abs_offset = %s, G.extra_bytes = %s\n",
               format_off_t(abs_offset, NULL, NULL),
               format_off_t(G.extra_bytes, NULL, NULL)));
        G.cur_zipfile_bufstart = lseek(G.zipfd, bufstart, SEEK_SET);
        Trace((stderr,
               "       request = %s, (abs+extra) = %s, inbuf_offset = %s\n",
               format_off_t(request, NULL, NULL),
               format_off_t((abs_offset + G.extra_bytes), NULL, NULL),
               format_off_t(inbuf_offset, NULL, NULL)));
        Trace((stderr, "       bufstart = %s, cur_zipfile_bufstart = %s\n",
               format_off_t(bufstart, NULL, NULL),
               format_off_t(G.cur_zipfile_bufstart, NULL, NULL)));
        if ((G.incnt = read(G.zipfd, (char *) G.inbuf, INBUFSIZ)) <= 0)
            return (PK_EOF);
        G.incnt -= (int) inbuf_offset;
        G.inptr = G.inbuf + (int) inbuf_offset;
    } else {
        G.incnt += (G.inptr - G.inbuf) - (int) inbuf_offset;
        G.inptr = G.inbuf + (int) inbuf_offset;
    }
    return (PK_OK);
}

int flush(rawbuf, size, unshrink)
uint8_t *rawbuf;
uint32_t size;
int unshrink;
{
    register uint8_t *p;
    register uint8_t *q;
    uint8_t *transbuf;

    /*---------------------------------------------------------------------------
        Compute the CRC first; if testing or if disk is full, that's it.
      ---------------------------------------------------------------------------*/

    G.crc32val = crc32(G.crc32val, rawbuf, (size_t) size);

    if (G.UzO.tflag || size == 0L) /* testing or nothing to write:  all done */
        return PK_OK;

    if (G.disk_full)
        return PK_DISK; /* disk already full:  ignore rest of file */

    /*---------------------------------------------------------------------------
        Write the bytes rawbuf[0..size-1] to the output device, first converting
        end-of-lines and ASCII/EBCDIC as needed.  If SMALL_MEM or MED_MEM are
      NOT defined, outbuf is assumed to be at least as large as rawbuf and is
      not necessarily checked for overflow.
      ---------------------------------------------------------------------------*/

    if (!G.pInfo->textmode) { /* write raw binary data */
        /* GRR:  note that for standard MS-DOS compilers, size argument to
         * fwrite() can never be more than 65534, so WriteError macro will
         * have to be rewritten if size can ever be that large.  For now,
         * never more than 32K.  Also note that write() returns an int, which
         * doesn't necessarily limit size to 32767 bytes if write() is used
         * on 16-bit systems but does make it more of a pain; however, because
         * at least MSC 5.1 has a lousy implementation of fwrite() (as does
         * DEC Ultrix cc), write() is used anyway.
         */
        if (!G.UzO.cflag && WriteError(rawbuf, size, G.outfile))
            return disk_error();
        return PK_OK;
    }
    if (unshrink) {
        /* rawbuf = outbuf */
        transbuf = G.outbuf2;
    } else {
        /* rawbuf = slide */
        transbuf = G.outbuf;
    }
    if (G.newfile) {
        G.didCRlast = FALSE; /* no previous buffers written */
        G.newfile = FALSE;
    }

    /*-----------------------------------------------------------------------
        Algorithm:  CR/LF => native; lone CR => native; lone LF => native.
        This routine is only for non-raw-VMS, non-raw-VM/CMS files (i.e.,
        stream-oriented files, not record-oriented).
      -----------------------------------------------------------------------*/

    p = rawbuf;
    if (*p == LF && G.didCRlast)
        ++p;
    G.didCRlast = FALSE;
    for (q = transbuf; (size_t) (p - rawbuf) < (size_t) size; ++p) {
        if (*p == CR) { /* lone CR or CR/LF: treat as EOL  */
            *q++ = LF;
            if ((size_t) (p - rawbuf) == (size_t) size - 1)
                /* last char in buffer */
                G.didCRlast = TRUE;
            else if (p[1] == LF) /* get rid of accompanying LF */
                ++p;
        } else if (*p == LF) { /* lone LF */
            *q++ = LF;
        } else if (*p != CTRLZ) { /* lose all ^Z's */
            *q++ = *p;
        }
    }

    /*-----------------------------------------------------------------------
        Done translating:  write whatever we've got to file (or screen).
      -----------------------------------------------------------------------*/

    Trace((stderr, "p - rawbuf = %u   q-transbuf = %u   size = %u\n",
           (unsigned) (p - rawbuf), (unsigned) (q - transbuf), size));
    if (q > transbuf) {
        if (!G.UzO.cflag &&
            WriteError(transbuf, (size_t) (q - transbuf), G.outfile))
            return disk_error();
        else if (G.UzO.cflag &&
                 (*G.message)(transbuf, (uint32_t) (q - transbuf), 0))
            return PK_OK;
    }

    return PK_OK;
}

static int disk_error(void)
{
    /* OK to use slide[] here because this file is finished regardless */
    Info(slide, 0x21, ((char *) slide, DiskFullQuery, FnFilter1(G.filename)));

    fgets(G.answerbuf, sizeof(G.answerbuf), stdin);
    if (*G.answerbuf == 'y') /* stop writing to this file */
        G.disk_full = 1;     /*  (outfile bad?), but new OK */
    else
        G.disk_full = 2; /* no:  exit program */

    return PK_DISK;
}

int UzpMessagePrnt(buf, size, flag)
uint8_t *buf;  /* preformatted string to be printed */
uint32_t size; /* length of string (may include nulls) */
int flag;      /* flag bits */
{
    int error;
    uint8_t *q = buf, *endbuf = buf + (unsigned) size;
    FILE *outfp;

    /*---------------------------------------------------------------------------
        These tests are here to allow fine-tuning of UnZip's output messages,
        but none of them will do anything without setting the appropriate bit
        in the flag argument of every Info() statement which is to be turned
        *off*.  That is, all messages are currently turned on for all ports.
        To turn off *all* messages, use the UzpMessageNull() function instead
        of this one.
      ---------------------------------------------------------------------------*/

    if (MSG_STDERR(flag) && !G.UzO.tflag)
        outfp = (FILE *) stderr;
    else
        outfp = (FILE *) stdout;

    if (MSG_LNEWLN(flag) && !G.sol) {
        /* not at start of line:  want newline */
        putc('\n', outfp);
        fflush(outfp);
        if (MSG_STDERR(flag) && G.UzO.tflag && !isatty(1) && isatty(2)) {
            /* error output from testing redirected:  also send to stderr */
            putc('\n', stderr);
            fflush(stderr);
        }
        G.sol = TRUE;
    }

    /* put zipfile name, filename and/or error/warning keywords here */

    if (size) {
        if ((error = WriteTxtErr(q, size, outfp)) != 0)
            return error;
        fflush(outfp);
        if (MSG_STDERR(flag) && G.UzO.tflag && !isatty(1) && isatty(2)) {
            /* error output from testing redirected:  also send to stderr */
            if ((error = WriteTxtErr(q, size, stderr)) != 0)
                return error;
            fflush(stderr);
        }
        G.sol = (endbuf[-1] == '\n');
    }
    return 0;
}

void UzpMorePause(prompt, flag) const char *prompt; /* "--More--" prompt */
int flag; /* 0 = any char OK; 1 = accept only '\n', ' ', q */
{
    uint8_t c;

    /*---------------------------------------------------------------------------
        Print a prompt and wait for the user to press a key, then erase prompt
        if possible.
      ---------------------------------------------------------------------------*/

    if (!G.sol)
        fprintf(stderr, "\n");
    /* numlines may or may not be used: */
    fprintf(stderr, prompt, G.numlines);
    fflush(stderr);
    if (flag & 1) {
        do {
            c = (uint8_t) zgetch(0);
        } while (c != '\r' && c != '\n' && c != ' ' && c != 'q' && c != 'Q');
    } else
        c = (uint8_t) zgetch(0);

    /* newline was not echoed, so cover up prompt line */
    fprintf(stderr, HidePrompt);
    fflush(stderr);

    if ((ToLower(c) == 'q')) {
        exit(PK_COOL);
    }

    G.sol = TRUE;
}

int UzpPassword(rcnt, pwbuf, size, zfn, efn)
int *rcnt;       /* retry counter */
char *pwbuf;     /* buffer for password */
int size;        /* size of password buffer */
const char *zfn; /* name of zip archive */
const char *efn; /* name of archive entry being processed */
{
    int r = IZ_PW_ENTERED;
    char *m;
    char *prompt;
    char *zfnf;
    char *efnf;
    size_t zfnfl;
    int isOverflow;

    if (*rcnt == 0) { /* First call for current entry */
        *rcnt = 2;
        zfnf = FnFilter1(zfn);
        efnf = FnFilter2(efn);
        zfnfl = strlen(zfnf);
        isOverflow = TRUE;
        if (2 * FILNAMSIZ >= zfnfl && (2 * FILNAMSIZ - zfnfl) >= strlen(efnf)) {
            isOverflow = FALSE;
        }
        if ((isOverflow == FALSE) &&
            ((prompt = malloc(2 * FILNAMSIZ + 15)) != NULL)) {
            sprintf(prompt, PasswPrompt, FnFilter1(zfn), FnFilter2(efn));
            m = prompt;
        } else
            m = (char *) PasswPrompt2;
    } else { /* Retry call, previous password was wrong */
        (*rcnt)--;
        prompt = NULL;
        m = (char *) PasswRetry;
    }

    m = getp(m, pwbuf, size);
    free(prompt);
    if (m == NULL) {
        r = IZ_PW_ERROR;
    } else if (*pwbuf == '\0') {
        r = IZ_PW_CANCELALL;
    }
    return r;
}

time_t dos_to_unix_time(dosdatetime)
uint32_t dosdatetime;
{
    time_t m_time;

    const time_t now = time(NULL);
    struct tm *tm;
#define YRBASE 1900

    tm = localtime(&now);
    tm->tm_isdst = -1; /* let mktime determine if DST is in effect */

    /* dissect date */
    tm->tm_year = ((int) (dosdatetime >> 25) & 0x7f) + (1980 - YRBASE);
    tm->tm_mon = ((int) (dosdatetime >> 21) & 0x0f) - 1;
    tm->tm_mday = ((int) (dosdatetime >> 16) & 0x1f);

    /* dissect time */
    tm->tm_hour = (int) ((unsigned) dosdatetime >> 11) & 0x1f;
    tm->tm_min = (int) ((unsigned) dosdatetime >> 5) & 0x3f;
    tm->tm_sec = (int) ((unsigned) dosdatetime << 1) & 0x3e;

    m_time = mktime(tm);
    TTrace((stderr, "  final m_time  =       %u\n", (uint32_t) m_time));

    if ((dosdatetime >= DOSTIME_2038_01_18) && (m_time < (time_t) 0x70000000L))
        m_time = U_TIME_T_MAX; /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t) 0L)  /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX; /*  -> saturate at max signed time_t value */

    return m_time;
}

int check_for_newer(filename) /* return 1 if existing file is newer */
                              /*  or equal; 0 if older; -1 if doesn't */
char *filename;               /*  exist yet */
{
    time_t existing, archive;
    iztimes z_utime;

    Trace((stderr, "check_for_newer:  doing stat(%s)\n", FnFilter1(filename)));
    if (stat(filename, &G.statbuf)) {
        Trace((stderr,
               "check_for_newer:  stat(%s) returns %d:  file does not exist\n",
               FnFilter1(filename), stat(filename, &G.statbuf)));
        Trace((stderr, "check_for_newer:  doing lstat(%s)\n",
               FnFilter1(filename)));
        /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
        if (lstat(filename, &G.statbuf) != 0) {
            return DOES_NOT_EXIST;
        }
        Trace((stderr,
               "check_for_newer:  lstat(%s) returns 0:  symlink does exist\n",
               FnFilter1(filename)));
        if (QCOND2 && !IS_OVERWRT_ALL) {
            printf(FileIsSymLink, FnFilter1(filename), " with no real file");
        }
        return EXISTS_AND_OLDER; /* symlink dates are meaningless */
    }
    Trace((stderr, "check_for_newer:  stat(%s) returns 0:  file exists\n",
           FnFilter1(filename)));

    /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
    if (lstat(filename, &G.statbuf) == 0 && S_ISLNK(G.statbuf.st_mode)) {
        Trace((stderr, "check_for_newer:  %s is a symbolic link\n",
               FnFilter1(filename)));
        if (QCOND2 && !IS_OVERWRT_ALL) {
            printf(FileIsSymLink, FnFilter1(filename), "");
        }
        return EXISTS_AND_OLDER; /* symlink dates are meaningless */
    }

    /* The `Unix extra field mtime' should be used for comparison with the
     * time stamp of the existing file >>>ONLY<<< when the EF info is also
     * used to set the modification time of the extracted file.
     */
    if (G.extra_field &&
        (ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                          G.lrec.last_mod_dos_datetime, &z_utime, NULL) &
         EB_UT_FL_MTIME)) {
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
        existing = G.statbuf.st_mtime;
        archive = z_utime.mtime;
    } else {
        /* round up existing filetime to nearest 2 seconds for comparison,
         * but saturate in case of arithmetic overflow
         */
        existing = ((G.statbuf.st_mtime & 1) &&
                    (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime))
                       ? G.statbuf.st_mtime + 1
                       : G.statbuf.st_mtime;
        archive = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }

    TTrace((stderr, "check_for_newer:  existing %u, archive %u, e-a %d\n",
            (uint32_t) existing, (uint32_t) archive,
            (long) (existing - archive)));

    return (existing >= archive);
}

int do_string(length, option) /* return PK-type error code */
unsigned int length; /* without prototype, uint16_t converted to this */
int option;
{
    unsigned comment_bytes_left;
    unsigned int block_len;
    int error = PK_OK;

    /*---------------------------------------------------------------------------
        This function processes arbitrary-length (well, usually) strings.  Four
        major options are allowed:  SKIP, wherein the string is skipped (pretty
        logical, eh?); DISPLAY, wherein the string is printed to standard output
        after undergoing any necessary or unnecessary character conversions;
        DS_FN, wherein the string is put into the filename[] array after under-
        going appropriate conversions (including case-conversion, if that is
        indicated: see the global variable pInfo->lcflag); and EXTRA_FIELD,
        wherein the `string' is assumed to be an extra field and is copied to
        the (freshly malloced) buffer G.extra_field.  The third option should
        be OK since filename is dimensioned at 1025, but we check anyway.

        The string, by the way, is assumed to start at the current file-pointer
        position; its length is given by 'length'.  So start off by checking the
        length of the string:  if zero, we're already done.
      ---------------------------------------------------------------------------*/

    if (!length)
        return PK_COOL;

    switch (option) {

        /*
         * First normal case:  print string on standard output.  First set loop
         * variables, then loop through the comment in chunks of OUTBUFSIZ
         * bytes, converting formats and printing as we go.  The second half of
         * the loop conditional was added because the file might be truncated,
         * in which case comment_bytes_left will remain at some non-zero value
         * for all time.  outbuf and slide are used as scratch buffers because
         * they are available (we should be either before or in between any file
         * pro- cessing).
         */

    case DISPLAY:
    case DISPL_8:
        comment_bytes_left = length;
        block_len = OUTBUFSIZ; /* for the while statement, first time */
        while (comment_bytes_left > 0 && block_len > 0) {
            register uint8_t *p = G.outbuf;
            register uint8_t *q = G.outbuf;

            if ((block_len =
                     readbuf((char *) G.outbuf, MIN((unsigned) OUTBUFSIZ,
                                                    comment_bytes_left))) == 0)
                return PK_EOF;
            comment_bytes_left -= block_len;

            /* this is why we allocated an extra byte for outbuf:  terminate
             *  with zero (ASCIIZ) */
            G.outbuf[block_len] = '\0';

            /* remove all ASCII carriage returns from comment before printing
             * (since used before A_TO_N(), check for CR instead of '\r')
             */
            while (*p) {
                while (*p == CR)
                    ++p;
                *q++ = *p++;
            }
            /* could check whether (p - outbuf) == block_len here */
            *q = '\0';

            if (option == DISPL_8) {
                /* translate the text coded in the entry's host-dependent
                   "extended ASCII" charset into the compiler's (system's)
                   internal text code page */
                Ext_ASCII_TO_Native((char *) G.outbuf, G.pInfo->hostnum,
                                    G.pInfo->hostver, G.pInfo->HasUxAtt, FALSE);
            } else {
                A_TO_N(G.outbuf); /* translate string to native */
            }

            p = G.outbuf - 1;
            q = slide;
            while (*++p) {
                int pause = FALSE;

                if (*p == 0x1B) { /* ASCII escape char */
                    *q++ = '^';
                    *q++ = '[';
                } else if (*p == 0x13) { /* ASCII ^S (pause) */
                    pause = TRUE;
                    if (p[1] == LF) /* ASCII LF */
                        *q++ = *++p;
                    else if (p[1] == CR && p[2] == LF) { /* ASCII CR LF */
                        *q++ = *++p;
                        *q++ = *++p;
                    }
                } else
                    *q++ = *p;
                if ((unsigned) (q - slide) > WSIZE - 3 || pause) { /* flush */
                    (*G.message)(slide, (uint32_t) (q - slide), 0);
                    q = slide;
                    if (pause && G.extract_flag) /* don't pause for list/test */
                        (*G.mpause)(QuitPrompt, 0);
                }
            }
            (*G.message)(slide, (uint32_t) (q - slide), 0);
        }
        /* add '\n' if not at start of line */
        (*G.message)(slide, 0L, 0x20);
        break;

        /*
         * Second case:  read string into filename[] array.  The filename should
         * never ever be longer than FILNAMSIZ-1 (1024), but for now we'll
         * check, just to be sure.
         */

    case DS_FN:
    case DS_FN_L:
        /* get the whole filename as need it for Unicode checksum */
        if (G.fnfull_bufsize <= length) {
            size_t fnbufsiz = FILNAMSIZ;

            if (fnbufsiz <= length)
                fnbufsiz = length + 1;
            free(G.filename_full);
            G.filename_full = malloc(fnbufsiz);
            if (G.filename_full == NULL)
                return PK_MEM;
            G.fnfull_bufsize = fnbufsiz;
        }
        if (readbuf(G.filename_full, length) == 0)
            return PK_EOF;
        G.filename_full[length] = '\0'; /* terminate w/zero:  ASCIIZ */

        /* if needed, chop off end so standard filename is a valid length */
        if (length >= FILNAMSIZ) {
            Info(slide, 1, ((char *) slide, FilenameTooLongTrunc));
            error = PK_WARN;
            length = FILNAMSIZ - 1;
        }
        /* no excess size */
        block_len = 0;
        strncpy(G.filename, G.filename_full, length);
        G.filename[length] = '\0'; /* terminate w/zero:  ASCIIZ */

        /* translate the Zip entry filename coded in host-dependent "extended
           ASCII" into the compiler's (system's) internal text code page */
        Ext_ASCII_TO_Native(G.filename, G.pInfo->hostnum, G.pInfo->hostver,
                            G.pInfo->HasUxAtt, (option == DS_FN_L));

        if (G.pInfo->lcflag) /* replace with lowercase filename */
            STRLOWER(G.filename, G.filename);

        if (G.pInfo->vollabel && length > 8 && G.filename[8] == '.') {
            char *p = G.filename + 8;
            while (*p++)
                p[-1] = *p; /* disk label, and 8th char is dot:  remove dot */
        }

        if (!block_len) /* no overflow, we're done here */
            break;

        /*
         * We truncated the filename, so print what's left and then fall
         * through to the SKIP routine.
         */
        Info(slide, 1, ((char *) slide, "[ %s ]\n", FnFilter1(G.filename)));
        length = block_len; /* SKIP the excess bytes... */
        /*  FALL THROUGH...  */

        /*
         * Third case:  skip string, adjusting readbuf's internal variables
         * as necessary (and possibly skipping to and reading a new block of
         * data).
         */

    case SKIP:
        /* cur_zipfile_bufstart already takes account of extra_bytes, so don't
         * correct for it twice: */
        seek_zipf(G.cur_zipfile_bufstart - G.extra_bytes + (G.inptr - G.inbuf) +
                  length);
        break;

        /*
         * Fourth case:  assume we're at the start of an "extra field"; malloc
         * storage for it and read data into the allocated space.
         */

    case EXTRA_FIELD:
        free(G.extra_field);
        if ((G.extra_field = malloc(length)) == NULL) {
            Info(slide, 1, ((char *) slide, ExtraFieldTooLong, length));
            /* cur_zipfile_bufstart already takes account of extra_bytes,
             * so don't correct for it twice: */
            seek_zipf(G.cur_zipfile_bufstart - G.extra_bytes +
                      (G.inptr - G.inbuf) + length);
            break;
        } else if (readbuf((char *) G.extra_field, length) == 0) {
            return PK_EOF;
        }
        /* Looks like here is where extra fields are read */
        if (getZip64Data(G.extra_field, length) != PK_COOL) {
            Info(slide, 1, ((char *) slide, ExtraFieldCorrupt, EF_PKSZ64));
            error = PK_WARN;
        }
        G.unipath_filename = NULL;
        if (G.UzO.U_flag >= 2) {
            break;
        }
        /* check if GPB11 (General Purpuse Bit 11) is set indicating
           the standard path and comment are UTF-8 */
        if (G.pInfo->GPFIsUTF8) {
            /* if GPB11 set then filename_full is untruncated UTF-8 */
            G.unipath_filename = G.filename_full;
        } else {
            /* Get the Unicode fields if exist */
            getUnicodeData(G.extra_field, length);
            if (G.unipath_filename && strlen(G.unipath_filename) == 0) {
                /* the standard filename field is UTF-8 */
                free(G.unipath_filename);
                G.unipath_filename = G.filename_full;
            }
        }
        if (!G.unipath_filename) {
            break;
        }
        if (G.native_is_utf8 && (!G.unicode_escape_all)) {
            strncpy(G.filename, G.unipath_filename, FILNAMSIZ - 1);
            /* make sure filename is short enough */
            if (strlen(G.unipath_filename) >= FILNAMSIZ) {
                G.filename[FILNAMSIZ - 1] = '\0';
                Info(slide, 1, ((char *) slide, UFilenameTooLongTrunc));
                error = PK_WARN;
            }
        } else {
            char *fn;

            /* convert UTF-8 to local character set */
            fn = utf8_to_local_string(G.unipath_filename, G.unicode_escape_all);

            /* 2022-07-22 SMS, et al.  CVE-2022-0530
             * Detect conversion failure, emit message.
             * Continue with unconverted name.
             */
            if (fn == NULL) {
                Info(slide, 1, ((char *) slide, UFilenameCorrupt));
                error = PK_ERR;
            } else {
                /* make sure filename is short enough */
                if (strlen(fn) >= FILNAMSIZ) {
                    fn[FILNAMSIZ - 1] = '\0';
                    Info(slide, 1, ((char *) slide, UFilenameTooLongTrunc));
                    error = PK_WARN;
                }
                /* replace filename with converted UTF-8 */
                strcpy(G.filename, fn);
                free(fn);
            }
        }
        if (G.unipath_filename != G.filename_full)
            free(G.unipath_filename);
        G.unipath_filename = NULL;
        break;
    } /* end switch (option) */

    return error;
}

uint16_t makeint16(b) const uint8_t *b;
{
    /*
     * Convert Intel style 'short' integer to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (uint16_t) ((b[1] << 8) | b[0]);
}

uint32_t makeint32(sig) const uint8_t *sig;
{
    /*
     * Convert intel style 'long' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((uint32_t) sig[3]) << 24) + (((uint32_t) sig[2]) << 16) +
           (uint32_t) ((((unsigned) sig[1]) << 8) + ((unsigned) sig[0]));
}

uint64_t makeint64(sig) const uint8_t *sig;
{
#ifdef LARGE_FILE_SUPPORT
    /*
     * Convert intel style 'int64' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((uint64_t) sig[7]) << 56) + (((uint64_t) sig[6]) << 48) +
           (((uint64_t) sig[5]) << 40) + (((uint64_t) sig[4]) << 32) +
           (uint64_t) ((((uint32_t) sig[3]) << 24) +
                       (((uint32_t) sig[2]) << 16) +
                       (((unsigned) sig[1]) << 8) + (sig[0]));

#else /* !LARGE_FILE_SUPPORT */

    if ((sig[7] | sig[6] | sig[5] | sig[4]) != 0)
        return (uint64_t) 0xffffffffL;
    else
        return (uint64_t) ((((uint32_t) sig[3]) << 24) +
                           (((uint32_t) sig[2]) << 16) +
                           (((unsigned) sig[1]) << 8) + (sig[0]));

#endif /* ?LARGE_FILE_SUPPORT */
}

/* Format a off_t value in a cylindrical buffer set. */
char *format_off_t(val, pre, post)
off_t val;
const char *pre;
const char *post;
{
    /* Temporary format string storage. */
    char fmt[16];

    if (pre == NULL) {
        pre = "";
    }
    if (post == NULL) {
        post = "d";
    }
    /* Assemble the format string. */
    snprintf(fmt, sizeof(fmt), "%%%s%s%s", pre, OFF_T_FMT, post);

    /* Advance the cylinder. */
    G.fofft_index = (G.fofft_index + 1) % OFF_T_NUM;

    /* Write into the current chamber. */
    snprintf(G.fofft_buf[G.fofft_index], OFF_T_LEN, fmt, val);

    /* Return a pointer to this chamber. */
    return G.fofft_buf[G.fofft_index];
}

#ifdef NEED_STR2ISO
char *str2iso(dst, src)
char *dst;                /* destination buffer */
register const char *src; /* source string */
{
#ifdef INTERN_TO_ISO
    INTERN_TO_ISO(src, dst);
#else
    register uint8_t c;
    register char *dstp = dst;

    do {
        c = (uint8_t) *src++;
        *dstp++ = (char) ASCII2ISO(c);
    } while (c != '\0');
#endif

    return dst;
}
#endif /* NEED_STR2ISO */

#ifdef NEED_STR2OEM
char *str2oem(dst, src)
char *dst;                /* destination buffer */
register const char *src; /* source string */
{
#ifdef INTERN_TO_OEM
    INTERN_TO_OEM(src, dst);
#else
    register uint8_t c;
    register char *dstp = dst;

    do {
        c = (uint8_t) *src++;
        *dstp++ = (char) ASCII2OEM(c);
    } while (c != '\0');
#endif

    return dst;
}
#endif /* NEED_STR2OEM */

#ifdef _MBCS

/* DBCS support for Info-ZIP's zip  (mainly for japanese (-: )
 * by Yoshioka Tsuneo (QWF00133@nifty.ne.jp,tsuneo-y@is.aist-nara.ac.jp)
 * This code is public domain!   Date: 1998/12/20
 */

char *plastchar(ptr, len) const char *ptr;
size_t len;
{
    unsigned clen;
    const char *oldptr = ptr;
    while (*ptr != '\0' && len > 0) {
        oldptr = ptr;
        clen = CLEN(ptr);
        ptr += clen;
        len -= clen;
    }
    return (char *) oldptr;
}

#ifdef NEED_UZMBCLEN
size_t uzmbclen(ptr) const unsigned char *ptr;
{
    int mbl;

    mbl = mblen((const char *) ptr, MB_CUR_MAX);
    /* For use in code scanning through MBCS strings, we need a strictly
       positive "MB char bytes count".  For our scanning purpose, it is not
       not relevant whether the MB character is valid or not. And, the NUL
       char '\0' has a byte count of 1, but mblen() returns 0. So, we make
       sure that the uzmbclen() return value is not less than 1.
     */
    return (size_t) (mbl > 0 ? mbl : 1);
}
#endif /* NEED_UZMBCLEN */

#ifdef NEED_UZMBSCHR
unsigned char *uzmbschr(str, c) const unsigned char *str;
unsigned int c;
{
    while (*str != '\0') {
        if (*str == c) {
            return (unsigned char *) str;
        }
        INCSTR(str);
    }
    return NULL;
}
#endif /* NEED_UZMBSCHR */

#ifdef NEED_UZMBSRCHR
unsigned char *uzmbsrchr(str, c) const unsigned char *str;
unsigned int c;
{
    unsigned char *match = NULL;
    while (*str != '\0') {
        if (*str == c) {
            match = (unsigned char *) str;
        }
        INCSTR(str);
    }
    return match;
}
#endif /* NEED_UZMBSRCHR */
#endif /* _MBCS */
