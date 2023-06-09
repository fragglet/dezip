/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/*
  The main encryption/decryption source code for Info-Zip software was
  originally written in Europe.  To the best of our knowledge, it can
  be freely distributed in both source and object forms from any country,
  including the USA under License Exception TSU of the U.S. Export
  Administration Regulations (section 740.13(e)) of 6 June 2002.
 */

#ifndef __crypt_h /* don't include more than once */
#define __crypt_h

#define IZ_PWLEN      80 /* input buffer size for reading encryption key */
#define RAND_HEAD_LEN 12 /* length of encryption random header */

/* encode byte c, using temp t.  Warning: c must not have side effects. */
#define zencode(c, t) (t = decrypt_byte(), update_keys(c), t ^ (c))

/* decode byte c in place */
#define zdecode(c) update_keys(c ^= decrypt_byte())

int decrypt_byte(void);
int update_keys(int c);
void init_keys(const char *passwd);

int decrypt(const char *passwrd);

#endif /* !__crypt_h */
