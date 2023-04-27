/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/*---------------------------------------------------------------------------

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

#include <termios.h>

/*
 * Turn echo off for file descriptor f.  Assumes that f is a tty device.
 */
void echoff(f) int f; /* file descriptor for which to turn echo off */
{
    struct termios sg; /* tty device structure */

    G.echofd = f;
    tcgetattr(f, &sg);   /* get settings */
    sg.c_lflag &= ~ECHO; /* turn echo off */
    tcsetattr(f, TCSAFLUSH, &sg);
}

/*
 * Turn echo back on for file descriptor echofd.
 */
void echon()
{
    struct termios sg; /* tty device structure */

    if (G.echofd != -1) {
        tcgetattr(G.echofd, &sg); /* get settings */
        sg.c_lflag |= ECHO;       /* turn echo on */
        tcsetattr(G.echofd, TCSAFLUSH, &sg);
        G.echofd = -1;
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
    struct termios sg; /* tty device structure */

    tcgetattr(f, &sg);      /* get settings */
    oldmin = sg.c_cc[VMIN]; /* save old values */
    oldtim = sg.c_cc[VTIME];
    sg.c_cc[VMIN] = 1;            /* need only one char to return read() */
    sg.c_cc[VTIME] = 0;           /* no timeout */
    sg.c_lflag &= ~ICANON;        /* canonical mode off */
    sg.c_lflag &= ~ECHO;          /* turn echo off, too */
    tcsetattr(f, TCSAFLUSH, &sg); /* set cbreak mode */
    G.echofd = f; /* in case ^C hit (not perfect: still CBREAK) */

    read(f, &c, 1); /* read our character */

    sg.c_cc[VMIN] = oldmin; /* restore old values */
    sg.c_cc[VTIME] = oldtim;
    sg.c_lflag |= ICANON;         /* canonical mode on */
    sg.c_lflag |= ECHO;           /* turn echo on */
    tcsetattr(f, TCSAFLUSH, &sg); /* restore canonical mode */
    G.echofd = -1;

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
        putc('\n', stderr);
        fflush(stderr);
        w = "(line too long--try again)\n";
    } while (p[i - 1] != '\n');
    p[i - 1] = 0; /* terminate at newline */

    close(f);

    return p; /* return pointer to password */

} /* end function getp() */
