/*
  Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* Write out a fragment of assembly source giving offsets in globals.h: */

#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include <stdio.h>

/* Keep this in sync with the definition of redirSlide in unzpriv.h: */
#ifdef DLL
#  define pG_redirSlide pG->redirect_pointer
#else
#  define pG_redirSlide pG->area.Slide
#endif

int main(int argc, char **argv)
{
    Uz_Globs *pG = (void *) 0L;

    printf("bb              EQU     %lu\n", &pG->bb);
    printf("bk              EQU     %lu\n", &pG->bk);
    printf("wp              EQU     %lu\n", &pG->wp);
#ifdef FUNZIP
    printf("in              EQU     %lu\n", &pG->in);
#else
    printf("incnt           EQU     %lu\n", &pG->incnt);
    printf("inptr           EQU     %lu\n", &pG->inptr);
    printf("csize           EQU     %lu\n", &pG->csize);
    printf("mem_mode        EQU     %lu\n", &pG->mem_mode);
#endif
    printf("slide           EQU     %lu\n", &pG_redirSlide);
    printf("SIZEOF_slide    EQU     %lu\n", sizeof(pG_redirSlide));
    printf("CRYPT           EQU     %d\n",  CRYPT);
    return 0;
}
