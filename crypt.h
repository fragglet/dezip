/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
  (the contents of which are also included in (un)zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
  crypt.h (full version) by Info-ZIP.   Last revised:  [see CR_VERSION_DATE]

  The main encryption/decryption source code for Info-Zip software was
  originally written in Europe.  To the best of our knowledge, it can
  be freely distributed in both source and object forms from any country,
  including the USA under License Exception TSU of the U.S. Export
  Administration Regulations (section 740.13(e)) of 6 June 2002.

  NOTE on copyright history:
  Previous versions of this source package (up to version 2.8) were
  not copyrighted and put in the public domain.  If you cannot comply
  with the Info-Zip LICENSE, you may want to look for one of those
  public domain versions.
 */

#ifndef __crypt_h /* don't include more than once */
#define __crypt_h

#undef CRYPT
/*
   Logic of selecting "full crypt" code:
   a) default behaviour:
      - dummy crypt code when compiling UnZipSFX stub, to minimize size
      - full crypt code when used to compile Zip, UnZip and fUnZip
   b) USE_CRYPT defined:
      - always full crypt code
   c) NO_CRYPT defined:
      - never full crypt code
   NO_CRYPT takes precedence over USE_CRYPT
 */
#define CRYPT 1 /* full version */

/* full version */

#define CR_MAJORVER     2
#define CR_MINORVER     11
#define CR_BETA_VER     ""
#define CR_VERSION_DATE "05 Jan 2007" /* last public release date */
#define CR_RELEASE

#ifndef __G /* UnZip only, for now (DLL stuff) */
#define __G
#define __G__
#define __GDEF
#define __GPRO void
#define __GPRO__
#endif

/* To allow combining of Zip and UnZip static libraries in a single binary,
 * the Zip and UnZip versions of the crypt core functions have to be named
 * differently.
 */

#define IZ_PWLEN 80 /* input buffer size for reading encryption key */
#ifndef PWLEN       /* for compatibility with previous zcrypt release... */
#define PWLEN IZ_PWLEN
#endif
#define RAND_HEAD_LEN 12 /* length of encryption random header */

/* the crc_32_tab array has to be provided externally for the crypt calculus */

/* encode byte c, using temp t.  Warning: c must not have side effects. */
#define zencode(c, t) (t = decrypt_byte(__G), update_keys(c), t ^ (c))

/* decode byte c in place */
#define zdecode(c) update_keys(__G__ c ^= decrypt_byte(__G))

int decrypt_byte OF((__GPRO));
int update_keys OF((__GPRO__ int c));
void init_keys OF((__GPRO__ const char *passwd));

int decrypt OF((__GPRO__ const char *passwrd));

#endif /* !__crypt_h */
