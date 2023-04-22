/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
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
 */

#ifndef __crypt_h /* don't include more than once */
#define __crypt_h

#undef CRYPT
#define CRYPT 1 /* full version */

/* full version */

#define CR_MAJORVER     2
#define CR_MINORVER     11
#define CR_BETA_VER     ""
#define CR_VERSION_DATE "05 Jan 2007" /* last public release date */
#define CR_RELEASE

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
#define zencode(c, t) (t = decrypt_byte(), update_keys(c), t ^ (c))

/* decode byte c in place */
#define zdecode(c) update_keys(c ^= decrypt_byte())

int decrypt_byte(void);
int update_keys(int c);
void init_keys(const char *passwd);

int decrypt(const char *passwrd);

#endif /* !__crypt_h */
