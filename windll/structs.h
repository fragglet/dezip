#ifndef _STRUCTS_H
#define _STRUCTS_H

#ifndef Far
#  define Far far
#endif

/* Porting definations between Win 3.1x and Win32 */
#ifdef WIN32
#  define far
#  define _far
#  define __far
#  define near
#  define _near
#  define __near
#  ifndef FAR
#    define FAR
#  endif
#endif

#ifndef PATH_MAX
#  define PATH_MAX 128            /* max total file or directory name path */
#endif

#ifndef DEFINED_ONCE
#define DEFINED_ONCE
#ifndef __unzip_h
   typedef unsigned short ush;
#endif
typedef int (WINAPI DLLPRNT) (char * far, unsigned long);
typedef int (WINAPI DLLPASSWORD) (char *, int, const char *, const char *);
#endif
typedef void (WINAPI DLLSND) (void);
typedef int (WINAPI DLLREPLACE)(char *);
typedef void (WINAPI DLLMESSAGE)(unsigned long,unsigned long,
   ush, ush, ush, ush, ush, ush, char, char *, char *, unsigned long);

typedef struct {
DLLPRNT *print;
DLLSND *sound;
DLLREPLACE *replace;
DLLPASSWORD *password;
DLLMESSAGE *SendApplicationMessage;
WORD cchComment;
unsigned long TotalSizeComp;
unsigned long TotalSize;
int CompFactor;
unsigned int NumMembers;
} USERFUNCTIONS, far * LPUSERFUNCTIONS;

typedef struct {
int ExtractOnlyNewer;
int Overwrite;
int SpaceToUnderscore;
int PromptToOverwrite;
int ZipInfoVerbose;
int fQuiet;
int ncflag;
int ntflag;
int nvflag;
int nUflag;
int nzflag;
int ndflag;
int noflag;
int naflag;
int nZIflag;
int fPrivilege;
LPSTR lpszZipFN;
} DCL, _far *LPDCL;

typedef struct {
HINSTANCE hInstance;
char print[80];
char sound[80];
char replace[80];
char password[80];
char SendApplicationMessage[80];
WORD cchComment;
unsigned long TotalSizeComp;
unsigned long TotalSize;
int CompFactor;
unsigned int NumMembers;
} VBUSERFUNCTIONS, far * LPVBUSERFUNCTIONS;

#endif /* _STRUCTS_H */
