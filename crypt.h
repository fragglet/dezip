/*
   crypt.h (full version) by Info-ZIP.   Last revised:  [see CR_VERSION_DATE]

   This header file is not copyrighted, and non-beta versions may be
   distributed without restriction.
 */

#ifndef __crypt_h   /* don't include more than once */
#define __crypt_h

#ifdef CRYPT
#  undef CRYPT
#endif
#define CRYPT  1    /* full version */

#define CR_MAJORVER        2
#define CR_MINORVER        7
#ifdef CR_BETA
#  define CR_BETA_VER      "m BETA"
#  define CR_VERSION_DATE  "13 April 1997"     /* last real code change */
#else
#  define CR_BETA_VER      ""
#  define CR_VERSION_DATE  "22 April 1997"     /* last public release date */
#  define CR_RELEASE
#endif

#ifndef __G         /* UnZip only, for now (DLL stuff) */
#  define __G
#  define __G__
#  define __GDEF
#  define __GPRO    void
#  define __GPRO__
#endif

#if defined(MSDOS) || defined(OS2) || defined(WIN32)
#  ifndef DOS_OS2_W32
#    define DOS_OS2_W32
#  endif
#endif

#if defined(DOS_OS2_W32) || defined(__human68k__)
#  ifndef DOS_H68_OS2_W32
#    define DOS_H68_OS2_W32
#  endif
#endif

#if defined(VM_CMS) || defined(MVS)
#  ifndef CMS_MVS
#    define CMS_MVS
#  endif
#endif

#ifdef REALLY_SHORT_SYMS
#  define decrypt_byte   dcrbyt
#endif

#define PWLEN  80   /* input buffer size for reading encryption key */
#define RAND_HEAD_LEN  12    /* length of encryption random header */

/* the crc_32_tab array has to be provided externally for the crypt calculus */
#ifndef UNZIP                   /* UnZip provides this in globals.h */
   extern ulg near *crc_32_tab;
#endif /* !UNZIP */

/* encode byte c, using temp t.  Warning: c must not have side effects. */
#define zencode(c,t)  (t=decrypt_byte(__G), update_keys(c), t^(c))

/* decode byte c in place */
#define zdecode(c)   update_keys(__G__ c ^= decrypt_byte(__G))

int  decrypt_byte OF((__GPRO));
int  update_keys OF((__GPRO__ int c));
void init_keys OF((__GPRO__ char *passwd));

#ifdef ZIP
   void crypthead OF((char *, ulg, FILE *));
#  ifdef UTIL
     int zipcloak OF((struct zlist far *, FILE *, FILE *, char *));
     int zipbare OF((__GPRO__ struct zlist far *, FILE *, FILE *, char *));
#  else
     unsigned zfwrite OF((zvoid *, extent, extent, FILE *));
     extern char *key;
#  endif
#endif /* ZIP */

#if (defined(UNZIP) && !defined(FUNZIP))
   int  decrypt OF((__GPRO));
#endif

#ifdef FUNZIP
   extern int encrypted;
#  ifdef NEXTBYTE
#    undef NEXTBYTE
#  endif
#  define NEXTBYTE \
   (encrypted? update_keys(__G__ getc(G.in)^decrypt_byte(__G)) : getc(G.in))
#endif /* FUNZIP */

#endif /* !__crypt_h */
