//******************************************************************************
//
// File:        WINCE.CPP
//
// Description: This file implements all the Win32 APIs and C runtime functions
//              that the Info-ZIP code calls, but are not implemented natively
//              on Windows CE.
//
// Copyright:   All the source files for Pocket UnZip, except for components
//              written by the Info-ZIP group, are copyrighted 1997 by Steve P.
//              Miller.  The product "Pocket UnZip" itself is property of the
//              author and cannot be altered in any way without written consent
//              from Steve P. Miller.
//
// Disclaimer:  All project files are provided "as is" with no guarantee of
//              their correctness.  The authors are not liable for any outcome
//              that is the result of using this source.  The source for Pocket
//              UnZip has been placed in the public domain to help provide an
//              understanding of its implementation.  You are hereby granted
//              full permission to use this source in any way you wish, except
//              to alter Pocket UnZip itself.  For comments, suggestions, and
//              bug reports, please write to stevemil@pobox.com.
//
// Functions:   DebugOut
//              chmod
//              close
//              isatty
//              lseek
//              open
//              read
//              setmode
//              unlink
//              fflush
//              fgets
//              fileno
//              fopen
//              fprintf
//              fclose
//              putc
//              sprintf
//              _stricmp
//              _strupr
//              strrchr
//              localtime
//              isupper
//              stat
//
//
// Date      Name          History
// --------  ------------  -----------------------------------------------------
// 02/01/97  Steve Miller  Created (Version 1.0 using Info-ZIP UnZip 5.30)
//
//******************************************************************************


extern "C" {
#include "punzip.h"
}
#include <tchar.h> // Must be outside of extern "C" block


//******************************************************************************
//***** For all platforms - Our debug output function
//******************************************************************************

#ifdef DEBUG // RETAIL version is __inline and does not generate any code.

void DebugOut(LPCTSTR szFormat, ...) {
   TCHAR szBuffer[512] = TEXT("PUNZIP: ");

   va_list pArgs; 
   va_start(pArgs, szFormat);
   _vsntprintf(szBuffer + 8, countof(szBuffer) - 10, szFormat, pArgs);
   va_end(pArgs);

   TCHAR *psz = szBuffer;
   while (psz = _tcschr(psz, TEXT('\n'))) {
      *psz = TEXT('|');
   }
   psz = szBuffer;
   while (psz = _tcschr(psz, TEXT('\r'))) {
      *psz = TEXT('|');
   }

   _tcscat(szBuffer, TEXT("\r\n"));

   OutputDebugString(szBuffer);
}

#endif // DEBUG


//******************************************************************************
//***** Windows CE Native
//******************************************************************************

#if defined(_WIN32_WCE)

//******************************************************************************
//***** IO.H functions
//******************************************************************************

//-- Called from fileio.c
int __cdecl chmod(const char *filename, int pmode) {
   // Called before unlink() to delete read-only files.

   DWORD dwAttribs = (pmode & _S_IWRITE) ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_READONLY;

   TCHAR szPath[_MAX_PATH];
   mbstowcs(szPath, filename, countof(szPath));
   return (SetFileAttributes(szPath, dwAttribs) ? 0 : -1);
}

//******************************************************************************
//-- Called from process.c
int __cdecl close(int handle) {
   return (CloseHandle((HANDLE)handle) ? 0 : -1);
}

//******************************************************************************
//-- Called from fileio.c
int __cdecl isatty(int handle) {
   // returns TRUE if handle is a terminal, console, printer, or serial port
   // called with 1 (stdout) and 2 (stderr)
   return 0;
}

//******************************************************************************
//-- Called from extract.c, fileio.c, process.c
long __cdecl lseek(int handle, long offset, int origin) {
   // SEEK_SET, SEEK_CUR, SEEK_END are equal to FILE_BEGIN, FILE_CURRENT, FILE_END   
   return SetFilePointer((HANDLE)handle, offset, NULL, origin);
}
                 
//******************************************************************************
//-- Called from fileio.c
int __cdecl open(const char *filename, int oflag, ...) {

   // The Info-Zip code currently only opens existing ZIP files for read using open().

   TCHAR szPath[_MAX_PATH];
   mbstowcs(szPath, filename, countof(szPath));
   HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   return ((hFile == INVALID_HANDLE_VALUE) ? -1 : (int)hFile);
}

//******************************************************************************
//-- Called from extract.c, fileio.c, process.c
int __cdecl read(int handle, void *buffer, unsigned int count) {
   DWORD dwRead = 0;
   return (ReadFile((HANDLE)handle, buffer, count, &dwRead, NULL) ? dwRead : -1);
}

//******************************************************************************
//-- Called from extract.c
int __cdecl setmode(int handle, int mode) {
   //TEXT/BINARY translation - currently always called with O_BINARY.
   return O_BINARY;
}

//******************************************************************************
//-- Called from fileio.c
int __cdecl unlink(const char *filename) {

   // Called to delete files before an extract overwrite.

   TCHAR szPath[_MAX_PATH];
   mbstowcs(szPath, filename, countof(szPath));
   return (DeleteFile(szPath) ? 0: -1);
}

//******************************************************************************
//***** STDIO.H functions
//******************************************************************************

//-- Called from fileio.c
int __cdecl fflush(FILE *stream) {
   return (FlushFileBuffers((HANDLE)stream) ? 0 : EOF);
}

//******************************************************************************
//-- Called from extract.c
char * __cdecl fgets(char *string, int n, FILE *stream) {
   // stream always equals "stdin" and fgets() should never be called.
   DebugOut(TEXT("WARNING: fgets(0x%08X, %d, %08X) called."), string, n, stream);
   return NULL;
}

//******************************************************************************
//-- Called from extract.c
int __cdecl fileno(FILE *stream) {
   return (int)stream;
}

//******************************************************************************
//-- Called from fileio.c
FILE * __cdecl fopen(const char *filename, const char *mode) {

   // fopen() is used to create all extracted files.

   DWORD dwAccess = 0;
   DWORD dwCreate = 0;
   BOOL  fAppend  = FALSE;

   if (strstr(mode, "r+")) {
      dwAccess = GENERIC_READ | GENERIC_WRITE;
      dwCreate = OPEN_EXISTING;
   } else if (strstr(mode, "w+")) {
      dwAccess = GENERIC_READ | GENERIC_WRITE;
      dwCreate = CREATE_ALWAYS;
   } else if (strstr(mode, "a+")) {
      dwAccess = GENERIC_READ | GENERIC_WRITE;
      dwCreate = OPEN_ALWAYS;
      fAppend = TRUE;
   } else if (strstr(mode, "r")) {
      dwAccess = GENERIC_READ;
      dwCreate = OPEN_EXISTING;
   } else if (strstr(mode, "w")) {
      dwAccess = GENERIC_WRITE;
      dwCreate = CREATE_ALWAYS;
   } else if (strstr(mode, "a")) {
      dwAccess = GENERIC_WRITE;
      dwCreate = OPEN_ALWAYS;
      fAppend  = TRUE;
   }

   TCHAR szPath[_MAX_PATH];
   mbstowcs(szPath, filename, countof(szPath));
   HANDLE hFile = CreateFile(szPath, dwAccess, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, dwCreate, FILE_ATTRIBUTE_NORMAL, NULL);

   if (hFile == INVALID_HANDLE_VALUE) {
      return NULL;
   }

   if (fAppend) {
      SetFilePointer(hFile, 0, NULL, FILE_END);
   }

   return (FILE*)hFile;
}

//******************************************************************************
//-- Called from unshrink.c
int __cdecl fprintf(FILE *stream, const char *format, ...) {
   
   // All standard output/error in Info-ZIP is handled through fprintf()
   if ((stream == stdout) || (stream == stderr)) {
      return 1;
   }

   // "stream" always equals "stderr" or "stdout" - log error if we see otherwise.
   DebugOut(TEXT("WARNING: fprintf(0x%08X, \"%S\", ...) called."), stream, format);
   return 0;
}

//******************************************************************************
//-- Called from fileio.c
int __cdecl fclose(FILE *stream) {
   return (CloseHandle((HANDLE)stream) ? 0 : EOF);
}

//******************************************************************************
//-- Called from fileio.c
int __cdecl putc(int c, FILE *stream) {
   DebugOut(TEXT("WARNING: putc(%d, 0x%08X) called."), c, stream);
   return 0;
}

//******************************************************************************
//-- Called from intrface.c, extract.c, fileio.c, list.c, process.c
int __cdecl sprintf(char *buffer, const char *format, ...) {

   WCHAR wszBuffer[512], wszFormat[512];

   mbstowcs(wszFormat, format, countof(wszFormat));
   BOOL fPercent = FALSE;
   for (WCHAR *pwsz = wszFormat; *pwsz; pwsz++) {
      if (*pwsz == L'%') {
         fPercent = !fPercent;
      } else if (fPercent && (((*pwsz >= L'a') && (*pwsz <= L'z')) || 
                              ((*pwsz >= L'A') && (*pwsz <= L'Z')))) 
      {
         if (*pwsz == L's') {
            *pwsz = L'S';
         } else if (*pwsz == L'S') {
            *pwsz = L's';
         }
         fPercent = FALSE;
      }
   }

   va_list pArgs; 
   va_start(pArgs, format);
   _vsntprintf(wszBuffer, countof(wszBuffer), wszFormat, pArgs);
   va_end(pArgs);

   wcstombs(buffer, wszBuffer, countof(wszBuffer));

   return 0;
}

//******************************************************************************
//***** STRING.H functions
//******************************************************************************

//-- Called from winmain.c
int __cdecl _stricmp(const char *string1, const char *string2) {
   while (*string1 && ((*string1 | 0x20) == (*string2 | 0x20))) {
      string1++;
      string2++;
   }
   return (*string1 - *string2);
}

//******************************************************************************
//-- Called from winmain.c
char* __cdecl _strupr(char *string) {
   while (*string) {
      if ((*string >= 'a') && (*string <= 'z')) {
         *string -= 'a' - 'A';
      }
      string++;
   }
   return string;
}

//******************************************************************************
//-- Called from _interface.c and winmain.c
char* __cdecl strrchr(const char *string, int c) {

   // Walk to end of string.
   for (char *p = (char*)string; *p; p++) {
   }

   // Walk backwards looking for character.
   for (p--; p >= string; p--) {
      if ((int)*p == c) {
         return p;
      }
   }

   return NULL;
}


//******************************************************************************
//***** TIME.H functions
//******************************************************************************

//-- Called from list.c
struct tm * __cdecl localtime(const time_t *timer) {

   // Return value for localtime().  Source currently never references
   // more than one "tm" at a time, so the single return structure is ok.
   static struct tm g_tm; 

   // time_t   is a 32-bit value for the seconds since January 1, 1970
   // FILETIME is a 64-bit value for the number of 100-nanosecond intervals since January 1, 1601

   // Compute the FILETIME for the given time_t.
   DWORDLONG dwl = ((DWORDLONG)116444736000000000 + ((DWORDLONG)*timer * (DWORDLONG)10000000));
   FILETIME ft = *(FILETIME*)&dwl;

   // Compute the FILETIME to a local FILETIME.
   FILETIME ftLocal;
   ZeroMemory(&ftLocal, sizeof(ftLocal));
   FileTimeToLocalFileTime(&ft, &ftLocal);

   // Convert the FILETIME to a SYSTEMTIME.
   SYSTEMTIME st;
   ZeroMemory(&st, sizeof(st));
   FileTimeToSystemTime(&ftLocal, &st);

   // Convert the SYSTEMTIME to a "tm".
   ZeroMemory(&g_tm, sizeof(g_tm));
   g_tm.tm_sec  = (int)st.wSecond;
   g_tm.tm_min  = (int)st.wMinute;
   g_tm.tm_hour = (int)st.wHour;
   g_tm.tm_mday = (int)st.wDay;
   g_tm.tm_mon  = (int)st.wMonth - 1;
   g_tm.tm_year = (int)st.wYear - 1900;

   return &g_tm;
}

//******************************************************************************
//***** CTYPE.H functions
//******************************************************************************

//-- Called from fileio.c
int __cdecl isupper(int c) {
   return ((c >= 'A') && (c <= 'Z'));
}

//******************************************************************************
//***** STAT.H functions
//******************************************************************************

//-- Called fileio.c, process.c, intrface.c
int __cdecl stat(const char *path, struct stat *buffer) {

   // stat() is called on both the ZIP files and extracred files.

   // Clear our stat buffer to be safe.
   ZeroMemory(buffer, sizeof(struct stat));

   // Find the file/direcotry and fill in a WIN32_FIND_DATA structure.
   WIN32_FIND_DATA w32fd;
   ZeroMemory(&w32fd, sizeof(w32fd));

   TCHAR szPath[_MAX_PATH];
   mbstowcs(szPath, path, countof(szPath));
   HANDLE hFind = FindFirstFile(szPath, &w32fd);

   // Bail out now if we could not find the file/directory.
   if (hFind == INVALID_HANDLE_VALUE) {
      return -1;
   }

   // Close the find.
   FindClose(hFind);

   // Mode flags that are currently used: S_IWRITE, S_IFMT, S_IFDIR, S_IEXEC
   if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      buffer->st_mode = _S_IFDIR | _S_IREAD | _S_IEXEC;
   } else {
      buffer->st_mode = _S_IFREG | _S_IREAD;
   }
   if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
      buffer->st_mode |= _S_IWRITE;
   }

   // Store the file size.
   buffer->st_size  = (_off_t)w32fd.nFileSizeLow;

   // Convert the modified FILETIME to a time_t and store it.
   DWORDLONG dwl = *(DWORDLONG*)&w32fd.ftLastWriteTime;
   buffer->st_mtime = (time_t)((dwl - (DWORDLONG)116444736000000000) / (DWORDLONG)10000000);

   return 0;
}

#endif // _WIN32_WCE
