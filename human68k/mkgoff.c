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
#include <stddef.h>

/* Keep this in sync with the definition of redirSlide in unzpriv.h: */
#ifdef DLL
#  define REDIRSLIDE redirect_pointer
#else
#  define REDIRSLIDE area.Slide
#endif

#define OFFS(m) offsetof (struct Globals, m)

int main(int argc, char **argv)
{
    Uz_Globs *pG = (void *) 0L;

    printf("bb              EQU     %lu\n", OFFS (bb));
    printf("bk              EQU     %lu\n", OFFS (bk));
    printf("wp              EQU     %lu\n", OFFS (wp));
#ifdef FUNZIP
    printf("in              EQU     %lu\n", OFFS (in));
#else
    printf("incnt           EQU     %lu\n", OFFS (incnt));
    printf("inptr           EQU     %lu\n", OFFS (inptr));
    printf("csize           EQU     %lu\n", OFFS (csize));
    printf("mem_mode        EQU     %lu\n", OFFS (mem_mode));
#endif
    printf("slide           EQU     %lu\n", OFFS (REDIRSLIDE));
    printf("SIZEOF_slide    EQU     %lu\n", sizeof(pG->REDIRSLIDE));
    printf("CRYPT           EQU     %d\n",  CRYPT);

    return 0;
}
