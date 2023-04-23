/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  ttyio.c

  This file contains routines for doing console input/output, including code
  for non-echoing input.  It is used by the encryption/decryption code but
  does not contain any restricted code itself.  This file is shared between
  Info-ZIP's Zip and UnZip.

  ---------------------------------------------------------------------------*/

#include "unzip.h"
#include "crypt.h"

/* Non-echo console/keyboard input is needed for (en/de)cryption's password
 * entry.
 */

#include "ttyio.h"

#ifndef PUTC
#define PUTC putc
#endif

#define GLOBAL(g) G.g

/* include system support for switching of console echo */
#include <termios.h>
#define sgttyb     termios
#define sg_flags   c_lflag
#define GTTY(f, s) tcgetattr(f, (void *) s)
#define STTY(f, s) tcsetattr(f, TCSAFLUSH, (void *) s)

/*
 * Turn echo off for file descriptor f.  Assumes that f is a tty device.
 */
void Echoff(f) int f; /* file descriptor for which to turn echo off */
{
    struct sgttyb sg; /* tty device structure */

    GLOBAL(echofd) = f;
    GTTY(f, &sg);         /* get settings */
    sg.sg_flags &= ~ECHO; /* turn echo off */
    STTY(f, &sg);
}

/*
 * Turn echo back on for file descriptor echofd.
 */
void Echon()
{
    struct sgttyb sg; /* tty device structure */

    if (GLOBAL(echofd) != -1) {
        GTTY(GLOBAL(echofd), &sg); /* get settings */
        sg.sg_flags |= ECHO;       /* turn echo on */
        STTY(GLOBAL(echofd), &sg);
        GLOBAL(echofd) = -1;
    }
}

/*
 * Get a character from the given file descriptor without echo or newline.
 */
int zgetch(f)
int f; /* file descriptor from which to read */
{
    char oldmin, oldtim;
    char c;
    struct sgttyb sg; /* tty device structure */

    GTTY(f, &sg);           /* get settings */
    oldmin = sg.c_cc[VMIN]; /* save old values */
    oldtim = sg.c_cc[VTIME];
    sg.c_cc[VMIN] = 1;      /* need only one char to return read() */
    sg.c_cc[VTIME] = 0;     /* no timeout */
    sg.sg_flags &= ~ICANON; /* canonical mode off */
    sg.sg_flags &= ~ECHO;   /* turn echo off, too */
    STTY(f, &sg);           /* set cbreak mode */
    GLOBAL(echofd) = f;     /* in case ^C hit (not perfect: still CBREAK) */

    read(f, &c, 1); /* read our character */

    sg.c_cc[VMIN] = oldmin; /* restore old values */
    sg.c_cc[VTIME] = oldtim;
    sg.sg_flags |= ICANON; /* canonical mode on */
    sg.sg_flags |= ECHO;   /* turn echo on */
    STTY(f, &sg);          /* restore canonical mode */
    GLOBAL(echofd) = -1;

    return (int) (uch) c;
}

/*
 * Get a password of length n-1 or less into *p using the prompt *m.
 * The entered password is not echoed.
 */

#ifndef _PATH_TTY
#define _PATH_TTY "/dev/tty"
#endif

char *getp(m, p, n) const char *m; /* prompt for password */
char *p;                           /* return value: line input */
int n;                             /* bytes available in p[] */
{
    char c;  /* one-byte buffer for read() to use */
    int i;   /* number of characters input */
    char *w; /* warning on retry */
    int f;   /* file descriptor for tty device */

    /* turn off echo on tty */

    if ((f = open(_PATH_TTY, 0)) == -1)
        return NULL;
    /* get password */
    w = "";
    do {
        fputs(w, stderr); /* warning if back again */
        fputs(m, stderr); /* prompt */
        fflush(stderr);
        i = 0;
        echoff(f);
        do { /* read line, keeping n */
            read(f, &c, 1);
            if (i < n)
                p[i++] = c;
        } while (c != '\n');
        echon();
        PUTC('\n', stderr);
        fflush(stderr);
        w = "(line too long--try again)\n";
    } while (p[i - 1] != '\n');
    p[i - 1] = 0; /* terminate at newline */

    close(f);

    return p; /* return pointer to password */

} /* end function getp() */
