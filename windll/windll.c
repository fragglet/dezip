/*
 Copyright (C) 1996 Mike White
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/
/* Windows Info-ZIP Unzip DLL module
 *
 * Author: Mike White
 *
 * Original: 1996
 *
 * This module has the entry points for "unzipping" a zip file.
 */

/*---------------------------------------------------------------------------

  This file is the WINDLL replacement for the generic ``main program source
  file'' unzip.c.

  See the general comments in the header part of unzip.c.

  UnZip - a zipfile extraction utility.

  Version:  unzip530.{tar.Z | zip | zoo} for Unix, VMS, OS/2, MS-DOS, Amiga,
              Atari, Windows 3.x/95/NT, Macintosh, Human68K, Acorn RISC OS,
              VM/CMS, MVS, AOS/VS and TOPS-20.  Decryption requires sources
              in zcrypt27.zip.  See the accompanying "Where" file in the main
              source distribution for ftp, uucp, BBS and mail-server sites.

  Copyrights:  see accompanying file "COPYING" in UnZip source distribution.
               (This software is free but NOT IN THE PUBLIC DOMAIN.  There
               are some restrictions on commercial use.)

  ---------------------------------------------------------------------------*/

#include <windows.h>
#ifdef __RSXNT__
#  include "win32/rsxntwin.h"
#endif
#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include "version.h"
#include "windll.h"
#include "structs.h"

HANDLE hwildZipFN;

HANDLE hInst;               /* current instance */

LPDCL lpDCL;
HANDLE hDCL;
LPUSERFUNCTIONS lpUserFunctions;

/* For displaying status messages and error messages */
int DllMessagePrint(zvoid *pG, uch *buf, ulg size, int flag);

/* For displaying files extracted to the display window */
int DllDisplayPrint(zvoid *pG, uch *buf, ulg size, int flag);
DLLPRNT *lpPrint;

/* Dummy sound function for those applications that don't use sound */
void WINAPI DummySound(void);

/*  DLL Entry Point */

#ifdef __BORLANDC__
#pragma warn -par
#endif
#if defined WIN32
BOOL WINAPI DllEntryPoint( HINSTANCE hInstance,
                           DWORD fdwRreason,
                           LPVOID plvReserved)
#else
int FAR PASCAL LibMain( HINSTANCE hInstance,
                        WORD wDataSegment,
                        WORD wHeapSize,
                        LPSTR lpszCmdLine )
#endif
{
#ifndef WIN32
/* The startup code for the DLL initializes the local heap(if there is one)
 * with a call to LocalInit which locks the data segment.
 */

if ( wHeapSize != 0 )
   {
   UnlockData( 0 );
   }
#endif
hInst = hInstance;
return 1;   /* Indicate that the DLL was initialized successfully. */
}

int FAR PASCAL WEP ( int bSystemExit )
{
return 1;
}
#ifdef __BORLANDC__
#pragma warn .par
#endif

/* DLL calls */

jmp_buf dll_error_return;

int WINAPI windll_unzip(int argc, char **FNV, DCL far*C,
   USERFUNCTIONS far *lpUserFunc)
{
int retcode;
CONSTRUCTGLOBALS();

if (!Unz_Init((zvoid *)&G, lpUserFunc))
   {
   DESTROYGLOBALS();
   return PK_BADERR;
   }

if (C->lpszZipFN == NULL) /* Something has screwed up, we don't have a filename */
   {
   DESTROYGLOBALS();
   return PK_NOZIP;
   }

retcode = setjmp(dll_error_return);
if (retcode)
   {
   DESTROYGLOBALS();
   return PK_BADERR;
   }


lpDCL = C;

Unz_SetOpts((zvoid *)&G, C);

/* Here is the actual call to "unzip" the files (or whatever else you
 * are doing.)
 */
retcode = Unz_Unzip((zvoid *)&G, argc, FNV);

DESTROYGLOBALS();
return retcode;
}


/* Added type casts to prevent potential "type mismatch" error messages. */
#ifdef REENTRANT
#  undef G
#  undef __G
#  undef __G__
#  define G                   (*(struct Globals *)pG)
#  define __G                 (struct Globals *)pG
#  define __G__               (struct Globals *)pG,
#endif

zvoid * WINAPI Unz_CreateGlobals(void)
{
    CONSTRUCTGLOBALS();
    return (zvoid *)&G;
}


BOOL WINAPI Unz_Init(pG, lpUserFunc)
zvoid *pG;
USERFUNCTIONS far * lpUserFunc;
{
lpUserFunctions = lpUserFunc;
lpPrint = lpUserFunc->print;
G.message = DllMessagePrint;
G.sound = lpUserFunc->sound;
if (G.sound == NULL)
   G.sound = DummySound;
G.replace = lpUserFunc->replace;

if (!lpPrint ||
    !G.sound ||
    !G.replace)
    return FALSE;

return TRUE;
}

int WINAPI Unz_Unzip(pG, argc, FNV)
zvoid *pG;
int argc;
char **FNV;
{
int retcode;

G.filespecs = argc;

if (argc > 0) {
    G.pfnames = FNV;
    G.process_all_files = FALSE;
#ifndef CRTL_CP_IS_ISO
    for (; *FNV != NULL; FNV++)
        {
        _ISO_INTERN(*FNV);
        }
#endif
    }
else
    G.process_all_files = TRUE;       /* for speed */

/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

retcode = process_zipfiles(__G);
FreeDllMem(__G);
return retcode;
}


void WINAPI Unz_ReleaseGlobals(zvoid *pUzpGlobals)
{
    register struct Globals *pG = (struct Globals *)pUzpGlobals;
    DESTROYGLOBALS()
}


int win_fprintf(FILE *file, unsigned int size, char far *buffer)
{
if ((file != stderr) && (file != stdout))
   {
   return write(fileno(file),(char far *)(buffer),size);
   }
return lpPrint(buffer, size);
}

/**********************************
 * Function DllMessagePrint()     *
 *                                *
 * Send messages to status window *
 **********************************/
#ifdef __BORLANDC__
#pragma argsused
#endif
int DllMessagePrint(pG, buf, size, flag)
    zvoid *pG;      /* globals struct:  always passed */
    uch *buf;       /* preformatted string to be printed */
    ulg size;       /* length of string (may include nulls) */
    int flag;       /* flag bits */
{
return lpPrint((char far *)buf, size);
}

/********************************
 * Function DllDisplayPrint()   *
 *                              *
 * Send files to display window *
 ********************************/
#ifdef __BORLANDC__
#pragma argsused
#endif
int DllDisplayPrint(pG, buf, size, flag)
    zvoid *pG;      /* globals struct:  always passed */
    uch *buf;       /* preformatted string to be printed */
    ulg size;       /* length of string (may include nulls) */
    int flag;       /* flag bits */
{
return lpPrint((char far *)buf, size);
}


/**********************************
 * Function UzpPassword()         *
 *                                *
 * Prompt for decryption password *
 **********************************/
#ifdef __BORLANDC__
#pragma argsused
#endif
int UzpPassword(pG, rcnt, pwbuf, size, zfn, efn)
    zvoid *pG;          /* globals struct: always passed */
    int *rcnt;          /* retry counter */
    char *pwbuf;        /* buffer for password */
    int size;           /* size of password buffer */
    ZCONST char *zfn;   /* name of zip archiv */
    ZCONST char *efn;   /* name of archiv entry being processed */
{
#if CRYPT
    char *m;

    if (*rcnt == 0) {
        *rcnt = 2;
        m = "Enter password for: ";
    } else {
        (*rcnt)--;
        m = "Password incorrect--reenter: ";
    }

    return (*lpUserFunctions->password)(pwbuf, size, m, efn);
#else /* !CRYPT */
    return IZ_PW_ERROR; /* internal error, function should never get called */
#endif /* ?CRYPT */
} /* end function UzpPassword() */


int WINAPI unzipVB(int argc, char **FNV, DCL far *C,
   VBUSERFUNCTIONS far *lpUF)
{
int retcode;
HANDLE hUF;
LPUSERFUNCTIONS lpUserFunc;
void * lpSound;
void * lpPassword;
void * lpMessage;
void * lpReplace;
void * lpPrintVB;
CONSTRUCTGLOBALS();

hUF = GlobalAlloc( GPTR, (DWORD)sizeof(VBUSERFUNCTIONS));
if (!hUF)
   {
   DESTROYGLOBALS();
   return PK_MEM;
   }
lpUserFunc = (LPUSERFUNCTIONS)GlobalLock(hUF);
if (!lpUserFunc)
   {
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_MEM;
   }
lpPrintVB = GetProcAddress(lpUF->hInstance,
   lpUF->print);
lpUserFunc->print = lpPrintVB;
if (!lpUserFunc->print)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }
lpSound = GetProcAddress(lpUF->hInstance,lpUF->sound);
lpUserFunc->sound = lpSound;
if (!lpUserFunc->sound)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }
#ifdef __GNUC__
(int far WINAPI (*))lpReplace =
        (int far WINAPI (*))GetProcAddress(lpUF->hInstance,lpUF->replace);
#else
(int far *WINAPI)lpReplace =
        (int far *WINAPI)GetProcAddress(lpUF->hInstance,lpUF->replace);
#endif
lpUserFunc->replace = lpReplace;
if (!lpUserFunc->replace)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }
#ifdef __GNUC__
(int WINAPI (*)())lpPassword = (int WINAPI (*)())GetProcAddress(lpUF->hInstance,
   lpUF->password);
#else
(int * WINAPI)lpPassword = (int * WINAPI)GetProcAddress(lpUF->hInstance,
   lpUF->password);
#endif
lpUserFunc->password = lpPassword;
if (!lpUserFunc->password)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }
lpMessage = GetProcAddress(lpUF->hInstance,
   lpUF->SendApplicationMessage);
lpUserFunc->SendApplicationMessage = lpMessage;
if (!lpUserFunc->SendApplicationMessage)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }
lpUserFunc->cchComment = lpUF->cchComment;
lpUserFunc->TotalSizeComp = lpUF->TotalSizeComp;
lpUserFunc->TotalSize = lpUF->TotalSize;
lpUserFunc->CompFactor = lpUF->CompFactor;
lpUserFunc->NumMembers = lpUF->NumMembers;

if (!Unz_Init((zvoid *)pG, lpUserFunc))
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }

if (C->lpszZipFN == NULL)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_NOZIP;
   }

retcode = setjmp(dll_error_return);
if (retcode)
   {
   GlobalUnlock(hUF);
   GlobalFree(hUF);
   DESTROYGLOBALS();
   return PK_BADERR;
   }


lpDCL = C;

Unz_SetOpts((zvoid *)pG, C);

retcode = Unz_Unzip((zvoid *)pG, argc, FNV);

GlobalUnlock(hUF);
GlobalFree(hUF);
DESTROYGLOBALS();
return retcode;
}

/* Dummy sound function for those applications that don't use sound */
void WINAPI DummySound(void)
{
}

