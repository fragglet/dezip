#ifndef __windll_h   /* prevent multiple inclusions */
#define __windll_h

#include <windows.h>
#include <assert.h>    /* required for all Windows applications */
#include <setjmp.h>
#include <commdlg.h>
#ifndef __RSXNT__
#  include <dlgs.h>
#endif
#define UNZIP_INTERNAL
#include "unzip.h"
#include "structs.h"

#ifndef MSWIN
#  define MSWIN
#endif

/* Allow compilation under Borland C++ also */
#ifndef __based
#  define __based(A)
#endif

#ifndef PATH_MAX
#  define PATH_MAX 128            /* max total file or directory name path */
#endif

/* These two are dependent on the zip directory listing format string.
 * They help find the filename in the listbox entry.
 */
#define SHORT_FORM_FNAME_INX     27
#define LONG_FORM_FNAME_INX      58

#define IDM_REPLACE_NO     100
#define IDM_REPLACE_TEXT   101
#define IDM_REPLACE_YES    102
#define IDM_REPLACE_ALL    103
#define IDM_REPLACE_NONE   104
#define IDM_REPLACE_RENAME 105
#define IDM_REPLACE_HELP   106

extern jmp_buf dll_error_return;

extern HANDLE hInst;        /* current instance */

extern LPDCL lpDCL;
extern LPUSERFUNCTIONS lpUserFunctions;

#ifdef UNZIP_INTERNAL
void FreeDllMem(__GPRO);
int win_fprintf(FILE *file, unsigned int, char far *);
#endif

zvoid * WINAPI Unz_CreateGlobals(void);
BOOL    WINAPI Unz_Init(zvoid *, USERFUNCTIONS far *);
BOOL    WINAPI Unz_SetOpts(zvoid *, LPDCL);
int     WINAPI Unz_Unzip(zvoid *, int, char **);
void    WINAPI Unz_ReleaseGlobals(zvoid *pUzpGlobals);
extern  WINAPI windll_unzip(int, char **, DCL far *, USERFUNCTIONS far *);

#endif /* __windll_h */
