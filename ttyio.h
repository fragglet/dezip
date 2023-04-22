/*
  Copyright (c) 1990-2004 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
   ttyio.h
 */

#ifndef __ttyio_h /* don't include more than once */
#define __ttyio_h

#ifndef __crypt_h
#include "crypt.h" /* ensure that encryption header file has been seen */
#endif

/* Function prototypes */

/* For all other systems, ttyio.c supplies the two functions Echoff() and
 * Echon() for suppressing and (re)enabling console input echo.
 */
#ifndef echoff
#define echoff(f) Echoff(f)
#define echon()   Echon()
void Echoff(int f);
void Echon(void);
#endif

/* this stuff is used by MORE and also now by the ctrl-S code; fileio.c only */
/* default for all systems where no getch()-like function is available */
int zgetch(int f);

char *getp(const char *m, char *p, int n);

#endif /* !__ttyio_h */
