/*
 Copyright (C) 1996 Mike White
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/
/* MS Windows Setup  and Take-Down functions bracket calls to
 * process_zipfiles().
 * These functions allocate and free the necessary buffers, set and clear
 * any global variables so that  process_zipfiles()  can be called multiple
 * times in the same session of WiZ.
 *
 * Based on Robert Heath's code in a module I no longer remember
 * the name of.
 */

#include <stdio.h>
#include <windows.h>
#ifdef __RSXNT__
#  include "win32/rsxntwin.h"
#endif
#include "version.h"
#define UNZIP_INTERNAL
#include "unzip.h"
#include "windll.h"
#include "consts.h"

/* Added typecasts to prevent potential "type mismatch" error messages. */
#ifdef REENTRANT
#  undef G
#  undef __G
#  undef __G__
#  define G                   (*(struct Globals *)pG)
#  define __G                 (struct Globals *)pG
#  define __G__               (struct Globals *)pG,
#endif

extern HANDLE hwildZipFN;
/*
    ncflag    = write to stdout if true
    ntflag    = test zip file
    nvflag    = verbose listing
    nUflag    = "update" (extract only newer/new files
    nzflag    = display zip file comment
    ndflag    = all args are files/dir to be extracted
    noflag    = overwrite all files
    naflag    = do ASCII-EBCDIC and/or end of line translation
    nZIflag   = get Zip Info if true
    fPrivilege = restore ACL's if 1, use privileges if 2
    lpszZipFN = zip file name
*/

BOOL WINAPI Unz_SetOpts(pG, C)
zvoid * pG;
LPDCL C;
{
    G.qflag=C->fQuiet;  /* Quiet flag */
    G.pfnames = &fnames[0];       /* assign default file name vector */

    G.jflag = !C->ndflag;
    G.cflag = C->ncflag;
    G.overwrite_all = C->noflag;
    G.tflag = C->ntflag ;
    G.vflag = C->nvflag;
    G.zflag = C->nzflag;
    G.uflag = C->nUflag;
    G.aflag = C->naflag;
    G.uflag = C->ExtractOnlyNewer;
#ifdef WIN32
    G.X_flag = C->fPrivilege;
#endif
    G.overwrite_all = C->Overwrite;
    G.overwrite_none = !G.overwrite_all;
    G.sflag = C->SpaceToUnderscore; /* Translate spaces to underscores? */
    if (C->nZIflag)
      {
      G.zipinfo_mode = TRUE;
      G.hflag = TRUE;
      G.lflag = 10;
      G.qflag = 2;
      }
    else
      {
      G.zipinfo_mode = FALSE;
      }

    G.extract_flag = (!G.zipinfo_mode &&
                      !G.cflag && !G.tflag && !G.vflag && !G.zflag
#ifdef TIMESTAMP
                      && !G.T_flag
#endif
                     );

    G.xfilespecs = 0;

/* G.wildzipfn needs to be initialized so that do_wild does not wind
   up clearing out the zip file name when it returns in process.c
*/
    if ((hwildZipFN = GlobalAlloc(GMEM_MOVEABLE, FILNAMSIZ))== (HGLOBAL)NULL)
        return FALSE;

    G.wildzipfn = GlobalLock(hwildZipFN);
    lstrcpy(G.wildzipfn, C->lpszZipFN);
    _ISO_INTERN(G.wildzipfn);

    return TRUE;    /* set up was OK */
}

void FreeDllMem(__GPRO)
{
    if (G.wildzipfn) {
        GlobalUnlock(hwildZipFN);
        G.wildzipfn = NULL;
    }
    if (hwildZipFN)
        hwildZipFN = GlobalFree(hwildZipFN);

    G.zipinfo_mode = FALSE;
}
