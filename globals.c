/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2003-May-08 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  globals.c

  Routines to allocate and initialize globals, with or without threads.

  Contents:  registerGlobalPointer()
             deregisterGlobalPointer()
             getGlobalPointer()
             globalsCtor()

  ---------------------------------------------------------------------------*/

#include "unzip.h"

/* initialization of sigs is completed at runtime so unzip(sfx) executable
 * won't look like a zipfile
 */
char central_hdr_sig[4] = {0, 0, 0x01, 0x02};
char local_hdr_sig[4] = {0, 0, 0x03, 0x04};
char end_central_sig[4] = {0, 0, 0x05, 0x06};
char end_central64_sig[4] = {0, 0, 0x06, 0x06};
char end_centloc64_sig[4] = {0, 0, 0x06, 0x07};
/* extern char extd_local_sig[4] = {0, 0, 0x07, 0x08};  NOT USED YET */

const char *fnames[2] = {"*", NULL}; /* default filenames vector */

Uz_Globs G;

Uz_Globs *globalsCtor()
{

    /* for REENTRANT version, G is defined as (*pG) */

    memzero(&G, sizeof(Uz_Globs));

    uO.lflag = (-1);
    G.wildzipfn = "";
    G.pfnames = (char **) fnames;
    G.pxnames = (char **) &fnames[1];
    G.pInfo = G.info;
    G.sol = TRUE; /* at start of line */

    G.message = UzpMessagePrnt;
    G.input = UzpInput; /* not used by anyone at the moment... */
    G.mpause = UzpMorePause;
    G.decr_passwd = UzpPassword;

    G.echofd = -1;

    return &G;
}
