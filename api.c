/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  api.c

  This module supplies an UnZip engine for use directly from C/C++
  programs.  The functions are:

    ZCONST UzpVer *UzpVersion(void);
    unsigned UzpVersion2(UzpVer2 *version)
    int UzpMain(int argc, char *argv[]);
    int UzpAltMain(int argc, char *argv[], UzpInit *init);
    int UzpValidate(char *archive, int AllCodes);
    void UzpFreeMemBuffer(UzpBuffer *retstr);
    int UzpUnzipToMemory(char *zip, char *file, UzpOpts *optflgs,
                         UzpCB *UsrFuncts, UzpBuffer *retstr);

  non-WINDLL only (a special WINDLL variant is defined in windll/windll.c):
    int UzpGrep(char *archive, char *file, char *pattern, int cmd, int SkipBin,
                UzpCB *UsrFuncts);

  OS/2 only (for now):
    int UzpFileTree(char *name, cbList(callBack), char *cpInclude[],
          char *cpExclude[]);

  You must define `DLL' in order to include the API extensions.

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"
#ifdef WINDLL
#  ifdef POCKET_UNZIP
#    include "wince/intrface.h"
#  else
#    include "windll/windll.h"
#  endif
#endif
#include "unzvers.h"
#include <setjmp.h>

