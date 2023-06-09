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

  NOTE on copyright history:
  Previous versions of this source package (up to version 2.8) were
  not copyrighted and put in the public domain.  If you cannot comply
  with the Info-Zip LICENSE, you may want to look for one of those
  public domain versions.
 */

/*
  This encryption code is a direct transcription of the algorithm from
  Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
  file (appnote.txt) is distributed with the PKZIP program (even in the
  version without encryption capabilities).
 */

#define ZCRYPT_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include "ttyio.h"

#ifndef FALSE
#define FALSE 0
#endif

static int testp(const uint8_t *h);
static int testkey(const uint8_t *h, const char *key);

#ifndef Trace
#ifdef CRYPT_DEBUG
#define Trace(x) fprintf x
#else
#define Trace(x)
#endif
#endif

#include "crc32.h"

#define CRY_CRC_TAB CRC_32_TAB

/***********************************************************************
 * Return the next byte in the pseudo-random sequence
 */
int decrypt_byte()
{
    unsigned int temp = ((unsigned) G.keys[2] & 0xffff) | 2;
    return (int) (((temp * (temp ^ 1)) >> 8) & 0xff);
}

/***********************************************************************
 * Update the encryption keys with the next byte of plain text
 */
int update_keys(c)
int c; /* byte of plain text */
{
    int keyshift;
    G.keys[0] = CRC32(G.keys[0], c, CRY_CRC_TAB);
    G.keys[1] = (G.keys[1] + (G.keys[0] & 0xff)) * 134775813L + 1;
    keyshift = (int) (G.keys[1] >> 24);
    G.keys[2] = CRC32(G.keys[2], keyshift, CRY_CRC_TAB);
    return c;
}

/***********************************************************************
 * Initialize the encryption keys and the random header according to
 * the given password.
 */
void init_keys(passwd) const
    char *passwd; /* password string with which to modify keys */
{
    G.keys[0] = 305419896L;
    G.keys[1] = 591751049L;
    G.keys[2] = 878082192L;
    while (*passwd != '\0') {
        update_keys((int) *passwd);
        passwd++;
    }
}

/***********************************************************************
 * Initialize the local copy of the table of precomputed crc32 values.
 * Whereas the public crc32-table is optimized for crc32 calculations
 * on arrays of bytes, the crypt code needs the crc32 values in an
 * byte-order-independent form as 32-bit unsigned numbers. On systems
 * with Big-Endian byte order using the optimized crc32 code, this
 * requires inverting the byte-order of the values in the
 * crypt-crc32-table.
 */

/***********************************************************************
 * Get the password and set up keys for current zipfile member.
 * Return PK_ class error.
 */
int decrypt(passwrd) const char *passwrd;
{
    uint16_t b;
    int n, r;
    uint8_t h[RAND_HEAD_LEN];

    Trace((stdout, "\n[incnt = %d]: ", G.incnt));

    /* get header once (turn off "encrypted" flag temporarily so we don't
     * try to decrypt the same data twice) */
    G.pInfo->encrypted = FALSE;
    defer_leftover_input();
    for (n = 0; n < RAND_HEAD_LEN; n++) {
        /* 2012-11-23 SMS.  (OUSPG report.)
         * Quit early if compressed size < HEAD_LEN.  The resulting
         * error message ("unable to get password") could be improved,
         * but it's better than trying to read nonexistent data, and
         * then continuing with a negative G.csize.  (See
         * fileio.c:readbyte()).
         */
        if ((b = NEXTBYTE) == (uint16_t) EOF) {
            return PK_ERR;
        }
        h[n] = (uint8_t) b;
        Trace((stdout, " (%02x)", h[n]));
    }
    undefer_input();
    G.pInfo->encrypted = TRUE;

    if (G.newzip) { /* this is first encrypted member in this zipfile */
        G.newzip = FALSE;
        if (passwrd != NULL) { /* user gave password on command line */
            if (!G.key) {
                G.key = checked_strdup(passwrd);
                G.nopwd = TRUE; /* inhibit password prompting! */
            }
        } else { /* get rid of previous zipfile's key */
            free(G.key);
            G.key = NULL;
        }
    }

    /* if have key already, test it; else allocate memory for it */
    if (!G.key) {
        G.key = checked_malloc(IZ_PWLEN + 1);
    } else if (!testp(h)) {
        return PK_COOL; /* existing password OK (else prompt for new) */
    } else if (G.nopwd) {
        return PK_WARN; /* user indicated no more prompting */
    }

    /* try a few keys */
    n = 0;
    do {
        r = (*G.decr_passwd)(&n, G.key, IZ_PWLEN + 1, G.zipfn, G.filename);
        if (r != IZ_PW_ENTERED) { /* user replied "skip" or "skip all" */
            *G.key = '\0';        /*   We try the NIL password, ... */
            n = 0;                /*   and cancel fetch for this item. */
        }
        if (!testp(h))
            return PK_COOL;
        if (r == IZ_PW_CANCELALL) /* User replied "Skip all" */
            G.nopwd = TRUE;       /*   inhibit any further PW prompt! */
    } while (n > 0);

    return PK_WARN;
}

/***********************************************************************
 * Test the password.  Return -1 if bad, 0 if OK.
 */
static int testp(h) const uint8_t *h;
{
    int r;
    char *key_translated;

    /* On systems with "obscure" native character coding (e.g., EBCDIC),
     * the first test translates the password to the "main standard"
     * character coding. */

#ifdef STR_TO_CP1
    /* allocate buffer for translated password */
    key_translated = checked_malloc(strlen(G.key) + 1);
    /* first try, test password translated "standard" charset */
    r = testkey(h, STR_TO_CP1(key_translated, G.key));
#else  /* !STR_TO_CP1 */
    /* first try, test password as supplied on the extractor's host */
    r = testkey(h, G.key);
#endif /* ?STR_TO_CP1 */

#ifdef STR_TO_CP2
    if (r != 0) {
#ifndef STR_TO_CP1
        /* now prepare for second (and maybe third) test with translated pwd */
        key_translated = checked_malloc(strlen(G.key) + 1);
        return -1;
#endif
        /* second try, password translated to alternate ("standard") charset */
        r = testkey(h, STR_TO_CP2(key_translated, G.key));
#ifdef STR_TO_CP3
        if (r != 0)
            /* third try, password translated to another "standard" charset */
            r = testkey(h, STR_TO_CP3(key_translated, G.key));
#endif
#ifndef STR_TO_CP1
        free(key_translated);
#endif
    }
#endif /* STR_TO_CP2 */

#ifdef STR_TO_CP1
    free(key_translated);
    if (r != 0) {
        /* last resort, test password as supplied on the extractor's host */
        r = testkey(h, G.key);
    }
#endif /* STR_TO_CP1 */

    return r;
}

static int testkey(h, key) const uint8_t *h; /* decrypted header */
const char *key;                             /* decryption password to test */
{
    uint16_t b;
#ifdef ZIP10
    uint16_t c;
#endif
    int n;
    uint8_t *p;
    uint8_t hh[RAND_HEAD_LEN]; /* decrypted header */

    /* set keys and save the encrypted header */
    init_keys(key);
    memcpy(hh, h, RAND_HEAD_LEN);

    /* check password */
    for (n = 0; n < RAND_HEAD_LEN; n++) {
        zdecode(hh[n]);
        Trace((stdout, " %02x", hh[n]));
    }

    Trace((stdout,
           "\n  lrec.crc= %08lx  crec.crc= %08lx  pInfo->ExtLocHdr= %s\n",
           G.lrec.crc32, G.pInfo->crc, G.pInfo->ExtLocHdr ? "true" : "false"));
    Trace((stdout, "  incnt = %d  unzip offset into zipfile = %ld\n", G.incnt,
           G.cur_zipfile_bufstart + (G.inptr - G.inbuf)));

    /* same test as in zipbare(): */

#ifdef ZIP10 /* check two bytes */
    c = hh[RAND_HEAD_LEN - 2], b = hh[RAND_HEAD_LEN - 1];
    Trace((stdout,
      "  (c | (b<<8)) = %04x  (crc >> 16) = %04x  lrec.time = %04x\n",
      (uint16_t)(c | (b<<8)), (uint16_t)(G.lrec.crc32 >> 16),
      ((uint16_t)G.lrec.last_mod_dos_datetime & 0xffff))));
    if ((uint16_t) (c | (b << 8)) !=
        (G.pInfo->ExtLocHdr ? ((uint16_t) G.lrec.last_mod_dos_datetime & 0xffff)
                            : (uint16_t) (G.lrec.crc32 >> 16)))
        return -1; /* bad */
#else
    b = hh[RAND_HEAD_LEN - 1];
    Trace((stdout, "  b = %02x  (crc >> 24) = %02x  (lrec.time >> 8) = %02x\n",
           b, (uint16_t) (G.lrec.crc32 >> 24),
           ((uint16_t) G.lrec.last_mod_dos_datetime >> 8) & 0xff));
    if (b != (G.pInfo->ExtLocHdr
                  ? ((uint16_t) G.lrec.last_mod_dos_datetime >> 8) & 0xff
                  : (uint16_t) (G.lrec.crc32 >> 24)))
        return -1; /* bad */
#endif
    /* password OK:  decrypt current buffer contents before leaving */
    for (n = (long) G.incnt > G.csize ? (int) G.csize : G.incnt, p = G.inptr;
         n--; p++)
        zdecode(*p);
    return 0; /* OK */
}
