/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  ttyio.c

  This file contains routines for doing console input/output, including code
  for non-echoing input.  It is used by the encryption/decryption code but
  does not contain any restricted code itself.  This file is shared between
  Info-ZIP's Zip and UnZip.

  Contains:  echo()         (VMS only)
             Echon()        (Unix only)
             Echoff()       (Unix only)
             screensize()   (Unix only)
             zgetch()       (Unix, VMS, and non-Unix/VMS versions)
             getp()         ("PC," Unix/Atari/Be, VMS/VMCMS/MVS)

  ---------------------------------------------------------------------------*/

#define __TTYIO_C       /* identifies this source module */

#include "zip.h"
#include "crypt.h"

/* Non-echo console/keyboard input is needed for (en/de)cryption's password
 * entry, and for UnZip(SFX)'s MORE and Pause features.
 * (The corresponding #endif is found at the end of this module.)
 */

#include "ttyio.h"

#ifndef PUTC
#  define PUTC putc
#endif

#ifdef ZIP
#  ifdef GLOBAL          /* used in Amiga system headers, maybe others too */
#    undef GLOBAL
#  endif
#  define GLOBAL(g) g
#else
#  define GLOBAL(g) G.g
#endif

#  ifndef HAVE_TERMIOS_H
#    define HAVE_TERMIOS_H     /* POSIX termios.h */
#  endif

#ifdef UNZIP            /* Zip handles this with the unix/configure script */
#endif /* UNZIP */

#ifdef HAVE_TERMIOS_H
#endif

   /* include system support for switching of console echo */
#    ifdef HAVE_TERMIOS_H
#      include <termios.h>
#      define sgttyb termios
#      define sg_flags c_lflag
#      define GTTY(f, s) tcgetattr(f, (zvoid *) s)
#      define STTY(f, s) tcsetattr(f, TCSAFLUSH, (zvoid *) s)
#    else /* !HAVE_TERMIOS_H */
#      ifdef USE_SYSV_TERMIO           /* Amdahl, Cray, all SysV? */
#        ifdef NEED_PTEM
#          include <sys/stream.h>
#          include <sys/ptem.h>
#        endif
#        define sgttyb termio
#        define sg_flags c_lflag
#        define GTTY(f,s) ioctl(f,TCGETA,(zvoid *)s)
#        define STTY(f,s) ioctl(f,TCSETAW,(zvoid *)s)
#      else /* !USE_SYSV_TERMIO */
#          if (!defined(MINIX) && !defined(GOT_IOCTL_H))
#            include <sys/ioctl.h>
#          endif
#          include <sgtty.h>
#          define GTTY gtty
#          define STTY stty
#          ifdef UNZIP
             /*
              * XXX : Are these declarations needed at all ????
              */
             /*
              * GRR: let's find out...   Hmmm, appears not...
             int gtty OF((int, struct sgttyb *));
             int stty OF((int, struct sgttyb *));
              */
#          endif
#      endif /* ?USE_SYSV_TERMIO */
#    endif /* ?HAVE_TERMIOS_H */
#      ifndef UNZIP
#        include <fcntl.h>
#      endif



/* For VM/CMS and MVS, non-echo terminal input is not (yet?) supported. */

#ifdef ZIP                      /* moved to globals.h for UnZip */
   static int echofd=(-1);      /* file descriptor whose echo is off */
#endif

/*
 * Turn echo off for file descriptor f.  Assumes that f is a tty device.
 */
void Echoff(__G__ f)
    __GDEF
    int f;                    /* file descriptor for which to turn echo off */
{
    struct sgttyb sg;         /* tty device structure */

    GLOBAL(echofd) = f;
    GTTY(f, &sg);             /* get settings */
    sg.sg_flags &= ~ECHO;     /* turn echo off */
    STTY(f, &sg);
}

/*
 * Turn echo back on for file descriptor echofd.
 */
void Echon(__G)
    __GDEF
{
    struct sgttyb sg;         /* tty device structure */

    if (GLOBAL(echofd) != -1) {
        GTTY(GLOBAL(echofd), &sg);    /* get settings */
        sg.sg_flags |= ECHO;  /* turn echo on */
        STTY(GLOBAL(echofd), &sg);
        GLOBAL(echofd) = -1;
    }
}


#if (defined(UNZIP) && !defined(FUNZIP))


/*
 * Get a character from the given file descriptor without echo or newline.
 */
int zgetch(__G__ f)
    __GDEF
    int f;                      /* file descriptor from which to read */
{
    char oldmin, oldtim;
    char c;
    struct sgttyb sg;           /* tty device structure */

    GTTY(f, &sg);               /* get settings */
    oldmin = sg.c_cc[VMIN];     /* save old values */
    oldtim = sg.c_cc[VTIME];
    sg.c_cc[VMIN] = 1;          /* need only one char to return read() */
    sg.c_cc[VTIME] = 0;         /* no timeout */
    sg.sg_flags &= ~ICANON;     /* canonical mode off */
    sg.sg_flags &= ~ECHO;       /* turn echo off, too */
    STTY(f, &sg);               /* set cbreak mode */
    GLOBAL(echofd) = f;         /* in case ^C hit (not perfect: still CBREAK) */

    read(f, &c, 1);             /* read our character */

    sg.c_cc[VMIN] = oldmin;     /* restore old values */
    sg.c_cc[VTIME] = oldtim;
    sg.sg_flags |= ICANON;      /* canonical mode on */
    sg.sg_flags |= ECHO;        /* turn echo on */
    STTY(f, &sg);               /* restore canonical mode */
    GLOBAL(echofd) = -1;

    return (int)(uch)c;
}


#endif /* UNZIP && !FUNZIP */


/*
 * Simple compile-time check for source compatibility between
 * zcrypt and ttyio:
 */
#if (!defined(CR_MAJORVER) || (CR_MAJORVER < 2) || (CR_MINORVER < 7))
   error:  This Info-ZIP tool requires zcrypt 2.7 or later.
#endif

/*
 * Get a password of length n-1 or less into *p using the prompt *m.
 * The entered password is not echoed.
 */


#ifndef _PATH_TTY
#    define _PATH_TTY "/dev/tty"
#endif

char *getp(__G__ m, p, n)
    __GDEF
    ZCONST char *m;             /* prompt for password */
    char *p;                    /* return value: line input */
    int n;                      /* bytes available in p[] */
{
    char c;                     /* one-byte buffer for read() to use */
    int i;                      /* number of characters input */
    char *w;                    /* warning on retry */
    int f;                      /* file descriptor for tty device */

    /* turn off echo on tty */

    if ((f = open(_PATH_TTY, 0)) == -1)
        return NULL;
    /* get password */
    w = "";
    do {
        fputs(w, stderr);       /* warning if back again */
        fputs(m, stderr);       /* prompt */
        fflush(stderr);
        i = 0;
        echoff(f);
        do {                    /* read line, keeping n */
            read(f, &c, 1);
            if (i < n)
                p[i++] = c;
        } while (c != '\n');
        echon();
        PUTC('\n', stderr);  fflush(stderr);
        w = "(line too long--try again)\n";
    } while (p[i-1] != '\n');
    p[i-1] = 0;                 /* terminate at newline */

    close(f);

    return p;                   /* return pointer to password */

} /* end function getp() */



