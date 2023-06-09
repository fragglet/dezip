/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2007-Mar-04 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/* inflate.c -- by Mark Adler
   version c17e, 30 Mar 2007 */

/*
   History:
   vers    date          who           what
   ----  ---------  --------------  ------------------------------------
    a    ~~ Feb 92  M. Adler        used full (large, one-step) lookup table
    b1   21 Mar 92  M. Adler        first version with partial lookup tables
    b2   21 Mar 92  M. Adler        fixed bug in fixed-code blocks
    b3   22 Mar 92  M. Adler        sped up match copies, cleaned up some
    b4   25 Mar 92  M. Adler        added prototypes; removed window[] (now
                                    is the responsibility of unzip.h--also
                                    changed name to slide[]), so needs diffs
                                    for unzip.c and unzip.h (this allows
                                    compiling in the small model on MSDOS);
                                    fixed cast of q in huft_build();
    b5   26 Mar 92  M. Adler        got rid of unintended macro recursion.
    b6   27 Mar 92  M. Adler        got rid of nextbyte() routine.  fixed
                                    bug in inflate_fixed().
    c1   30 Mar 92  M. Adler        removed lbits, dbits environment variables.
                                    changed BMAX to 16 for explode.  Removed
                                    OUTB usage, and replaced it with flush()--
                                    this was a 20% speed improvement!  Added
                                    an explode.c (to replace unimplod.c) that
                                    uses the huft routines here.  Removed
                                    register union.
    c2    4 Apr 92  M. Adler        fixed bug for file sizes a multiple of 32k.
    c3   10 Apr 92  M. Adler        reduced memory of code tables made by
                                    huft_build significantly (factor of two to
                                    three).
    c4   15 Apr 92  M. Adler        added NOMEMCPY do kill use of memcpy().
                                    worked around a Turbo C optimization bug.
    c5   21 Apr 92  M. Adler        added the WSIZE #define to allow reducing
                                    the 32K window size for specialized
                                    applications.
    c6   31 May 92  M. Adler        added some typecasts to eliminate warnings
    c7   27 Jun 92  G. Roelofs      added some more typecasts (444:  MSC bug).
    c8    5 Oct 92  J-l. Gailly     added ifdef'd code to deal with PKZIP bug.
    c9    9 Oct 92  M. Adler        removed a memory error message (~line 416).
    c10  17 Oct 92  G. Roelofs      changed ULONG/UWORD/byte to ulg/ush/uch,
                                    removed old inflate, renamed inflate_entry
                                    to inflate, added Mark's fix to a comment.
   c10.5 14 Dec 92  M. Adler        fix up error messages for incomplete trees.
    c11   2 Jan 93  M. Adler        fixed bug in detection of incomplete
                                    tables, and removed assumption that EOB is
                                    the longest code (bad assumption).
    c12   3 Jan 93  M. Adler        make tables for fixed blocks only once.
    c13   5 Jan 93  M. Adler        allow all zero length codes (pkzip 2.04c
                                    outputs one zero length code for an empty
                                    distance tree).
    c14  12 Mar 93  M. Adler        made inflate.c standalone with the
                                    introduction of inflate.h.
   c14b  16 Jul 93  G. Roelofs      added (unsigned) typecast to w at 470.
   c14c  19 Jul 93  J. Bush         changed v[N_MAX], l[288], ll[28x+3x] arrays
                                    to static for Amiga.
   c14d  13 Aug 93  J-l. Gailly     de-complicatified Mark's c[*p++]++ thing.
   c14e   8 Oct 93  G. Roelofs      changed memset() to memzero().
   c14f  22 Oct 93  G. Roelofs      renamed quietflg to qflag; made Trace()
                                    conditional; added inflate_free().
   c14g  28 Oct 93  G. Roelofs      changed l/(lx+1) macro to pointer (Cray bug)
   c14h   7 Dec 93  C. Ghisler      huft_build() optimizations.
   c14i   9 Jan 94  A. Verheijen    set fixed_t{d,l} to NULL after freeing;
                    G. Roelofs      check NEXTBYTE macro for EOF.
   c14j  23 Jan 94  G. Roelofs      removed Ghisler "optimizations"; ifdef'd
                                    EOF check.
   c14k  27 Feb 94  G. Roelofs      added some typecasts to avoid warnings.
   c14l   9 Apr 94  G. Roelofs      fixed split comments on preprocessor lines
                                    to avoid bug in Encore compiler.
   c14m   7 Jul 94  P. Kienitz      modified to allow assembler version of
                                    inflate_codes() (define ASM_INFLATECODES)
   c14n  22 Jul 94  G. Roelofs      changed fprintf to macro for DLL versions
   c14o  23 Aug 94  C. Spieler      added a newline to a debug statement;
                    G. Roelofs      added another typecast to avoid MSC warning
   c14p   4 Oct 94  G. Roelofs      added (voidp *) cast to free() argument
   c14q  30 Oct 94  G. Roelofs      changed fprintf macro to MESSAGE()
   c14r   1 Nov 94  G. Roelofs      fixed possible redefinition of CHECK_EOF
   c14s   7 May 95  S. Maxwell      OS/2 DLL globals stuff incorporated;
                    P. Kienitz      "fixed" ASM_INFLATECODES macro/prototype
   c14t  18 Aug 95  G. Roelofs      added UZinflate() to use zlib functions;
                                    changed voidp to void; moved huft_build()
                                    and huft_free() to end of file
   c14u   1 Oct 95  G. Roelofs      moved G into definition of MESSAGE macro
   c14v   8 Nov 95  P. Kienitz      changed ASM_INFLATECODES to use a regular
                                    call with instead of a macro
    c15   3 Aug 96  M. Adler        fixed bomb-bug on random input data (Adobe)
   c15b  24 Aug 96  M. Adler        more fixes for random input data
   c15c  28 Mar 97  G. Roelofs      changed USE_ZLIB fatal exit code from
                                    PK_MEM2 to PK_MEM3
    c16  20 Apr 97  J. Altman       added memzero(v[]) in huft_build()
   c16b  29 Mar 98  C. Spieler      modified DLL code for slide redirection
   c16c  04 Apr 99  C. Spieler      fixed memory leaks when processing gets
                                    stopped because of input data errors
   c16d  05 Jul 99  C. Spieler      take care of FLUSH() return values and
                                    stop processing in case of errors
    c17  31 Dec 00  C. Spieler      added preliminary support for Deflate64
   c17a  04 Feb 01  C. Spieler      complete integration of Deflate64 support
   c17b  16 Feb 02  C. Spieler      changed type of "extra bits" arrays and
                                    corresponding huft_build() parameter e from
                                    ush into uch, to save space
   c17c   9 Mar 02  C. Spieler      fixed NEEDBITS() "read beyond EOF" problem
                                    with CHECK_EOF enabled
   c17d  23 Jul 05  C. Spieler      fixed memory leaks in inflate_dynamic()
                                    when processing invalid compressed literal/
                                    distance table data
   c17e  30 Mar 07  C. Spieler      in inflate_dynamic(), initialize tl and td
                                    to prevent freeing unallocated huft tables
                                    when processing invalid compressed data and
                                    hitting premature EOF, do not reuse td as
                                    temp work ptr during tables decoding
 */

/*
   Inflate deflated (PKZIP's method 8 compressed) data.  The compression
   method searches for as much of the current string of bytes (up to a
   length of 258) in the previous 32K bytes.  If it doesn't find any
   matches (of at least length 3), it codes the next byte.  Otherwise, it
   codes the length of the matched string and its distance backwards from
   the current position.  There is a single Huffman code that codes both
   single bytes (called "literals") and match lengths.  A second Huffman
   code codes the distance information, which follows a length code.  Each
   length or distance code actually represents a base value and a number
   of "extra" (sometimes zero) bits to get to add to the base value.  At
   the end of each deflated block is a special end-of-block (EOB) literal/
   length code.  The decoding process is basically: get a literal/length
   code; if EOB then done; if a literal, emit the decoded byte; if a
   length then get the distance and emit the referred-to bytes from the
   sliding window of previously emitted data.

   There are (currently) three kinds of inflate blocks: stored, fixed, and
   dynamic.  The compressor outputs a chunk of data at a time and decides
   which method to use on a chunk-by-chunk basis.  A chunk might typically
   be 32K to 64K, uncompressed.  If the chunk is uncompressible, then the
   "stored" method is used.  In this case, the bytes are simply stored as
   is, eight bits per byte, with none of the above coding.  The bytes are
   preceded by a count, since there is no longer an EOB code.

   If the data are compressible, then either the fixed or dynamic methods
   are used.  In the dynamic method, the compressed data are preceded by
   an encoding of the literal/length and distance Huffman codes that are
   to be used to decode this block.  The representation is itself Huffman
   coded, and so is preceded by a description of that code.  These code
   descriptions take up a little space, and so for small blocks, there is
   a predefined set of codes, called the fixed codes.  The fixed method is
   used if the block ends up smaller that way (usually for quite small
   chunks); otherwise the dynamic method is used.  In the latter case, the
   codes are customized to the probabilities in the current block and so
   can code it much better than the pre-determined fixed codes can.

   The Huffman codes themselves are decoded using a multi-level table
   lookup, in order to maximize the speed of decoding plus the speed of
   building the decoding tables.  See the comments below that precede the
   lbits and dbits tuning parameters.

   GRR:  return values(?)
           0  OK
           1  incomplete table
           2  bad input
           3  not enough memory
         the following return codes are passed through from FLUSH() errors
           50 (PK_DISK)   "overflow of output space"
           80 (IZ_CTRLC)  "canceled by user's request"
 */

/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.  The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
  14. The Deflate64 (PKZIP method 9) variant of the compression algorithm
      differs from "classic" deflate in the following 3 aspect:
      a) The size of the sliding history window is expanded to 64 kByte.
      b) The previously unused distance codes #30 and #31 code distances
         from 32769 to 49152 and 49153 to 65536.  Both codes take 14 bits
         of extra data to determine the exact position in their 16 kByte
         range.
      c) The last lit/length code #285 gets a different meaning. Instead
         of coding a fixed maximum match length of 258, it is used as a
         "generic" match length code, capable of coding any length from
         3 (min match length + 0) to 65538 (min match length + 65535).
         This means that the length code #285 takes 16 bits (!) of uncoded
         extra data, added to a fixed min length of 3.
      Changes a) and b) would have been transparent for valid deflated
      data, but change c) requires to switch decoder configurations between
      Deflate and Deflate64 modes.
 */

#include "unzip.h"

/* #define DEBUG */

/* marker for "unused" huft code, and corresponding check macro */
#define INVALID_CODE       99
#define IS_INVALID_CODE(c) ((c) == INVALID_CODE)

#ifndef WSIZE        /* default is 32K resp. 64K */
#define WSIZE 65536L /* window size--must be a power of two, and */
#endif

/* some buffer counters must be capable of holding 64k for Deflate64 */
#define UINT_D64 unsigned

#ifndef NEXTBYTE /* default is to simply get a byte from stdin */
#define NEXTBYTE getchar()
#endif

#ifndef MESSAGE /* only used twice, for fixed strings--NOT general-purpose */
#define MESSAGE(str, len, flag) fprintf(stderr, (char *) (str))
#endif

#ifndef FLUSH /* default is to simply write the buffer to stdout */
#define FLUSH(n)                                                            \
    (((size_t) fwrite(redirSlide, 1, (size_t) (n), stdout) == (size_t) (n)) \
         ? 0                                                                \
         : PKDISK)
#endif

#ifndef Trace
#ifdef DEBUG
#define Trace(x) fprintf x
#else
#define Trace(x)
#endif
#endif

/*---------------------------------------------------------------------------*/

/* Function prototypes */
int inflate_codes(struct huft *tl, struct huft *td, unsigned bl, unsigned bd);
static int inflate_stored(void);
static int inflate_fixed(void);
static int inflate_dynamic(void);
static int inflate_block(int *e);

/* The inflate algorithm uses a sliding 32K byte window on the uncompressed
   stream to find repeated byte strings.  This is implemented here as a
   circular buffer.  The index is updated simply by incrementing and then
   and'ing with 0x7fff (32K-1). */
/* It is left to other modules to supply the 32K area.  It is assumed
   to be usable as if it were declared "uint8_t slide[32768];" or as just
   "uint8_t *slide;" and then malloc'ed in the latter case.  The definition
   must be in unzip.h, included above. */

/* Tables for deflate from PKZIP's appnote.txt. */
/* - Order of the bit length code lengths */
static const unsigned border[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                  11, 4,  12, 3, 13, 2, 14, 1, 15};

/* - Copy lengths for literal codes 257..285 */
static const uint16_t cplens64[] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 3,  0,  0};
/* For Deflate64, the code 285 is defined differently. */
static const uint16_t cplens32[] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};
/* note: see note #13 above about the 258 in this list. */
/* - Extra bits for literal codes 257..285 */
static const uint8_t cplext64[] = {
    0,           0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2,  2,
    2,           3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16, INVALID_CODE,
    INVALID_CODE};
static const uint8_t cplext32[] = {
    0,           0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2,           3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, INVALID_CODE,
    INVALID_CODE};

/* - Copy offsets for distance codes 0..29 (0..31 for Deflate64) */
static const uint16_t cpdist[] = {
    1,    2,    3,    4,    5,    7,     9,     13,    17,    25,   33,
    49,   65,   97,   129,  193,  257,   385,   513,   769,   1025, 1537,
    2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32769, 49153};

/* - Extra bits for distance codes 0..29 (0..31 for Deflate64) */
static const uint8_t cpdext64[] = {0,  0,  0,  0,  1,  1,  2,  2,  3,  3, 4,
                                   4,  5,  5,  6,  6,  7,  7,  8,  8,  9, 9,
                                   10, 10, 11, 11, 12, 12, 13, 13, 14, 14};
static const uint8_t cpdext32[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
    11, 11, 12, 12, 13, 13,
    // this comment is here to appease clang-format
    INVALID_CODE, INVALID_CODE};

#define MAXLITLENS 288
#define MAXDISTS   32

/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j);
        x = b & mask_bits[j];
        DUMPBITS(j);

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed and are initialized at the beginning of a
   routine that uses these macros from a global bit buffer and count.

   In order to not ask for more bits than there are in the compressed
   stream, the Huffman tables are constructed to only ask for just
   enough bits to make up the end-of-block code (value 256).  Then no
   bytes need to be "returned" to the buffer at the end of the last
   block.  See the huft_build() routine.

   Actually, the precautions mentioned above are not sufficient to
   prevent fetches of bits beyound the end of the last block in every
   case. When the last code fetched before the end-of-block code was
   a very short distance code (shorter than "distance-prefetch-bits" -
   "end-of-block code bits"), this last distance code fetch already
   exausts the available data.  To prevent failure of extraction in this
   case, the "read beyond EOF" check delays the raise of the "invalid
   data" error until an actual overflow of "used data" is detected.
   This error condition is only fulfilled when the "number of available
   bits" counter k is found to be negative in the NEEDBITS() macro.

   An alternate fix for that problem adjusts the size of the distance code
   base table so that it does not exceed the length of the end-of-block code
   plus the minimum length of a distance code. This alternate fix can be
   enabled by defining the preprocessor symbol FIX_PAST_EOB_BY_TABLEADJUST.
 */

#define NEEDBITS(n)                    \
    do {                               \
        while ((int) k < (int) (n)) {  \
            int c = NEXTBYTE;          \
            if (c == EOF) {            \
                if ((int) k >= 0)      \
                    break;             \
                retval = 1;            \
                goto cleanup_and_exit; \
            }                          \
            b |= ((uint32_t) c) << k;  \
            k += 8;                    \
        }                              \
    } while (0)

#define DUMPBITS(n) \
    do {            \
        b >>= (n);  \
        k -= (n);   \
    } while (0)

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   are not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

/* bits in base literal/length lookup table */
static const unsigned lbits = 9;
/* bits in base distance lookup table */
static const unsigned dbits = 6;

/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
int inflate_codes(tl, td, bl, bd)
struct huft *tl, *td; /* literal/length and distance decoder tables */
unsigned bl, bd;      /* number of bits decoded by tl[] and td[] */
{
    register unsigned e; /* table entry flag/number of extra bits */
    unsigned d;          /* index for copy */
    UINT_D64 n;          /* length for copy (deflate64: might be 64k+2) */
    UINT_D64 w;          /* current window position (deflate64: up to 64k) */
    struct huft *t;      /* pointer to table entry */
    unsigned ml, md;     /* masks for bl and bd bits */
    register uint32_t b; /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local copies of globals */
    b = G.bb; /* initialize bit buffer */
    k = G.bk;
    w = G.wp; /* initialize window position */

    /* inflate the coded data */
    ml = mask_bits[bl]; /* precompute masks for speed */
    md = mask_bits[bd];
    while (1) {
        /* do until end of block */
        NEEDBITS(bl);
        t = tl + ((unsigned) b & ml);
        while (1) {
            DUMPBITS(t->b);

            if ((e = t->e) == 32) {
                /* then it's a literal */
                redirSlide[w++] = (uint8_t) t->v.n;
                if (w == WSIZE) {
                    if ((retval = FLUSH(w)) != 0)
                        goto cleanup_and_exit;
                    w = 0;
                }
                break;
            }

            if (e < 31) {
                /* then it's a length */
                /* get length of block to copy */
                NEEDBITS(e);
                n = t->v.n + ((unsigned) b & mask_bits[e]);
                DUMPBITS(e);

                /* decode distance of block to copy */
                NEEDBITS(bd);
                t = td + ((unsigned) b & md);
                while (1) {
                    DUMPBITS(t->b);
                    if ((e = t->e) < 32)
                        break;
                    if (IS_INVALID_CODE(e))
                        return 1;
                    e &= 31;
                    NEEDBITS(e);
                    t = t->v.t + ((unsigned) b & mask_bits[e]);
                }
                NEEDBITS(e);
                d = (unsigned) w - t->v.n - ((unsigned) b & mask_bits[e]);
                DUMPBITS(e);

                /* do the copy */
                do {
                    e = (unsigned) (WSIZE - ((d &= (unsigned) (WSIZE - 1)) >
                                                     (unsigned) w
                                                 ? (UINT_D64) d
                                                 : w));
                    if ((UINT_D64) e > n)
                        e = (unsigned) n;
                    n -= e;
                    do {
                        redirSlide[w++] = redirSlide[d++];
                    } while (--e);
                    if (w == WSIZE) {
                        if ((retval = FLUSH(w)) != 0)
                            goto cleanup_and_exit;
                        w = 0;
                    }
                } while (n);
                break;
            }

            if (e == 31) {
                /* it's the EOB signal */
                /* sorry for this goto, but we have to exit two loops at once */
                goto cleanup_decode;
            }

            if (IS_INVALID_CODE(e))
                return 1;

            e &= 31;
            NEEDBITS(e);
            t = t->v.t + ((unsigned) b & mask_bits[e]);
        }
    }
cleanup_decode:

    /* restore the globals from the locals */
    G.wp = (unsigned) w; /* restore global window pointer */
    G.bb = b;            /* restore global bit buffer */
    G.bk = k;

cleanup_and_exit:
    /* done */
    return retval;
}

/* "decompress" an inflated type 0 (stored) block. */
static int inflate_stored()
{
    UINT_D64 w;          /* current window position (deflate64: up to 64k!) */
    unsigned n;          /* number of bytes in block */
    register uint32_t b; /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local copies of globals */
    Trace((stderr, "\nstored block"));
    b = G.bb; /* initialize bit buffer */
    k = G.bk;
    w = G.wp; /* initialize window position */

    /* go to byte boundary */
    n = k & 7;
    DUMPBITS(n);

    /* get the length and its complement */
    NEEDBITS(16);
    n = ((unsigned) b & 0xffff);
    DUMPBITS(16);
    NEEDBITS(16);
    if (n != (unsigned) ((~b) & 0xffff))
        return 1; /* error in compressed data */
    DUMPBITS(16);

    /* read and output the compressed data */
    while (n--) {
        NEEDBITS(8);
        redirSlide[w++] = (uint8_t) b;
        if (w == WSIZE) {
            if ((retval = FLUSH(w)) != 0)
                goto cleanup_and_exit;
            w = 0;
        }
        DUMPBITS(8);
    }

    /* restore the globals from the locals */
    G.wp = (unsigned) w; /* restore global window pointer */
    G.bb = b;            /* restore global bit buffer */
    G.bk = k;

cleanup_and_exit:
    return retval;
}

/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
static int inflate_fixed(void)
{
    /* if first time, set up tables for fixed blocks */
    Trace((stderr, "\nliteral block"));
    if (G.fixed_tl == NULL) {
        int i;           /* temporary variable */
        unsigned l[288]; /* length list for huft_build */

        /* literal table */
        for (i = 0; i < 144; i++)
            l[i] = 8;
        for (; i < 256; i++)
            l[i] = 9;
        for (; i < 280; i++)
            l[i] = 7;
        for (; i < 288; i++) /* make a complete, but wrong code set */
            l[i] = 8;
        G.fixed_bl = 7;
        if ((i = huft_build(l, 288, 257, G.cplens, G.cplext, &G.fixed_tl,
                            &G.fixed_bl)) != 0) {
            G.fixed_tl = NULL;
            return i;
        }

        /* distance table */
        for (i = 0; i < MAXDISTS; i++) /* make an incomplete code set */
            l[i] = 5;
        G.fixed_bd = 5;
        if ((i = huft_build(l, MAXDISTS, 0, cpdist, G.cpdext, &G.fixed_td,
                            &G.fixed_bd)) > 1) {
            huft_free(G.fixed_tl);
            G.fixed_td = G.fixed_tl = NULL;
            return i;
        }
    }

    /* decompress until an end-of-block code */
    return inflate_codes(G.fixed_tl, G.fixed_td, G.fixed_bl, G.fixed_bd);
}

/* decompress an inflated type 2 (dynamic Huffman codes) block. */
static int inflate_dynamic(void)
{
    unsigned i; /* temporary variables */
    unsigned j;
    unsigned l;             /* last length */
    unsigned m;             /* mask for bit lengths table */
    unsigned n;             /* number of lengths to get */
    struct huft *tl = NULL; /* literal/length code table */
    struct huft *td = NULL; /* distance code table */
    struct huft *th; /* temp huft table pointer used in tables decoding */
    unsigned bl;     /* lookup bits for tl */
    unsigned bd;     /* lookup bits for td */
    unsigned nb;     /* number of bit length codes */
    unsigned nl;     /* number of literal/length codes */
    unsigned nd;     /* number of distance codes */
    unsigned
        ll[MAXLITLENS + MAXDISTS]; /* lit./length and distance code lengths */
    register uint32_t b;           /* bit buffer */
    register unsigned k;           /* number of bits in bit buffer */
    int retval = 0; /* error code returned: initialized to "no error" */

    /* make local bit buffer */
    Trace((stderr, "\ndynamic block"));
    b = G.bb;
    k = G.bk;

    /* read in table lengths */
    NEEDBITS(5);
    nl = 257 + ((unsigned) b & 0x1f); /* number of literal/length codes */
    DUMPBITS(5);
    NEEDBITS(5);
    nd = 1 + ((unsigned) b & 0x1f); /* number of distance codes */
    DUMPBITS(5);
    NEEDBITS(4);
    nb = 4 + ((unsigned) b & 0xf); /* number of bit length codes */
    DUMPBITS(4);
    if (nl > MAXLITLENS || nd > MAXDISTS)
        return 1; /* bad lengths */

    /* read in bit-length-code lengths */
    for (j = 0; j < nb; j++) {
        NEEDBITS(3);
        ll[border[j]] = (unsigned) b & 7;
        DUMPBITS(3);
    }
    for (; j < 19; j++)
        ll[border[j]] = 0;

    /* build decoding table for trees--single level, 7 bit lookup */
    bl = 7;
    retval = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl);
    if (bl == 0) /* no bit lengths */
        retval = 1;
    if (retval) {
        if (retval == 1)
            huft_free(tl);
        return retval; /* incomplete code set */
    }

    /* read in literal and distance code lengths */
    n = nl + nd;
    m = mask_bits[bl];
    i = l = 0;
    while (i < n) {
        NEEDBITS(bl);
        j = (th = tl + ((unsigned) b & m))->b;
        DUMPBITS(j);
        j = th->v.n;
        if (j < 16)          /* length of code in bits (0..15) */
            ll[i++] = l = j; /* save last length in l */
        else if (j == 16) {
            /* repeat last length 3 to 6 times */
            NEEDBITS(2);
            j = 3 + ((unsigned) b & 3);
            DUMPBITS(2);
            if ((unsigned) i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = l;
        } else if (j == 17) {
            /* 3 to 10 zero length codes */
            NEEDBITS(3);
            j = 3 + ((unsigned) b & 7);
            DUMPBITS(3);
            if ((unsigned) i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = 0;
            l = 0;
        } else {
            /* j == 18: 11 to 138 zero length codes */
            NEEDBITS(7);
            j = 11 + ((unsigned) b & 0x7f);
            DUMPBITS(7);
            if ((unsigned) i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = 0;
            l = 0;
        }
    }

    /* free decoding table for trees */
    huft_free(tl);

    /* restore the global bit buffer */
    G.bb = b;
    G.bk = k;

    /* build the decoding tables for literal/length and distance codes */
    bl = lbits;
    retval = huft_build(ll, nl, 257, G.cplens, G.cplext, &tl, &bl);
    if (bl == 0) /* no literals or lengths */
        retval = 1;
    if (retval) {
        if (retval == 1) {
            if (!G.UzO.qflag)
                MESSAGE((uint8_t *) "(incomplete l-tree)  ", 21L, 1);
            huft_free(tl);
        }
        return retval; /* incomplete code set */
    }
    bd = dbits;
    retval = huft_build(ll + nl, nd, 0, cpdist, G.cpdext, &td, &bd);
    if (retval == 1)
        retval = 0;
    if (bd == 0 && nl > 257) /* lengths but no distances */
        retval = 1;
    if (retval) {
        if (retval == 1) {
            if (!G.UzO.qflag)
                MESSAGE((uint8_t *) "(incomplete d-tree)  ", 21L, 1);
            huft_free(td);
        }
        huft_free(tl);
        return retval;
    }

    /* decompress until an end-of-block code */
    retval = inflate_codes(tl, td, bl, bd);

cleanup_and_exit:
    /* free the decoding tables, return */
    if (tl != NULL)
        huft_free(tl);
    if (td != NULL)
        huft_free(td);
    return retval;
}

/* decompress an inflated block */
static int inflate_block(e)
int *e; /* last block flag */
{
    unsigned t;          /* block type */
    register uint32_t b; /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local bit buffer */
    b = G.bb;
    k = G.bk;

    /* read in last block bit */
    NEEDBITS(1);
    *e = (int) b & 1;
    DUMPBITS(1);

    /* read in block type */
    NEEDBITS(2);
    t = (unsigned) b & 3;
    DUMPBITS(2);

    /* restore the global bit buffer */
    G.bb = b;
    G.bk = k;

    /* inflate that block type */
    if (t == 2)
        return inflate_dynamic();
    if (t == 0)
        return inflate_stored();
    if (t == 1)
        return inflate_fixed();

    /* bad block type */
    retval = 2;

cleanup_and_exit:
    return retval;
}

/* decompress an inflated entry */
int inflate(is_defl64)
int is_defl64;
{
    int e; /* last block flag */
    int r; /* result code */
#ifdef DEBUG
    unsigned h = 0; /* maximum struct huft's malloc'ed */
#endif

    /* initialize window, bit buffer */
    G.wp = 0;
    G.bk = 0;
    G.bb = 0;

    if (is_defl64) {
        G.cplens = cplens64;
        G.cplext = cplext64;
        G.cpdext = cpdext64;
        G.fixed_tl = G.fixed_tl64;
        G.fixed_bl = G.fixed_bl64;
        G.fixed_td = G.fixed_td64;
        G.fixed_bd = G.fixed_bd64;
    } else {
        G.cplens = cplens32;
        G.cplext = cplext32;
        G.cpdext = cpdext32;
        G.fixed_tl = G.fixed_tl32;
        G.fixed_bl = G.fixed_bl32;
        G.fixed_td = G.fixed_td32;
        G.fixed_bd = G.fixed_bd32;
    }

    /* decompress until the last block */
    do {
#ifdef DEBUG
        G.hufts = 0;
#endif
        if ((r = inflate_block(&e)) != 0)
            return r;
#ifdef DEBUG
        if (G.hufts > h)
            h = G.hufts;
#endif
    } while (!e);

    Trace((stderr, "\n%u bytes in Huffman tables (%u/entry)\n",
           h * (unsigned) sizeof(struct huft), (unsigned) sizeof(struct huft)));

    if (is_defl64) {
        G.fixed_tl64 = G.fixed_tl;
        G.fixed_bl64 = G.fixed_bl;
        G.fixed_td64 = G.fixed_td;
        G.fixed_bd64 = G.fixed_bd;
    } else {
        G.fixed_tl32 = G.fixed_tl;
        G.fixed_bl32 = G.fixed_bl;
        G.fixed_td32 = G.fixed_td;
        G.fixed_bd32 = G.fixed_bd;
    }

    /* flush out redirSlide and return (success, unless final FLUSH failed) */
    return (FLUSH(G.wp));
}

int inflate_free()
{
    if (G.fixed_tl != NULL) {
        huft_free(G.fixed_td);
        huft_free(G.fixed_tl);
        G.fixed_td = G.fixed_tl = NULL;
    }
    return 0;
}

/* If BMAX needs to be larger than 16, then h and x[] should be uint32_t. */
#define BMAX  16  /* maximum bit length of any code (16 for explode) */
#define N_MAX 288 /* maximum number of codes in any set */

/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
int huft_build(b, n, s, d, e, t, m) const
    unsigned *b;   /* code lengths in bits (all assumed <= BMAX) */
unsigned n;        /* number of codes (assumed <= N_MAX) */
unsigned s;        /* number of simple-valued codes (0..s-1) */
const uint16_t *d; /* list of base values for non-simple codes */
const uint8_t *e;  /* list of extra bits for non-simple codes */
struct huft **t;   /* result: starting table */
unsigned *m;       /* maximum lookup bits, returns actual */
{
    unsigned a;              /* counter for codes of length k */
    unsigned c[BMAX + 1];    /* bit length count table */
    unsigned el;             /* length of EOB code (value 256) */
    unsigned f;              /* i repeats in table every f entries */
    int g;                   /* maximum code length */
    int h;                   /* table level */
    register unsigned i;     /* counter, current code */
    register unsigned j;     /* counter */
    register int k;          /* number of bits in current code */
    int lx[BMAX + 1];        /* memory for l[-1..BMAX-1] */
    int *l = lx + 1;         /* stack of bits per table */
    register unsigned *p;    /* pointer into c[], b[], or v[] */
    register struct huft *q; /* points to current table */
    struct huft r;           /* table entry for structure assignment */
    struct huft *u[BMAX];    /* table stack */
    unsigned v[N_MAX];       /* values in order of bit length */
    register int w;          /* bits before this table == (l * h) */
    unsigned x[BMAX + 1];    /* bit offsets, then code stack */
    unsigned *xp;            /* pointer into x */
    int y;                   /* number of dummy codes added */
    unsigned z;              /* number of entries in current table */

    /* Generate counts for each bit length */
    el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
    memzero((char *) c, sizeof(c));
    p = (unsigned *) b;
    i = n;
    do {
        c[*p]++;
        p++; /* assume all entries <= BMAX */
    } while (--i);
    if (c[0] == n) {
        /* null input--all zero length codes */
        *t = NULL;
        *m = 0;
        return 0;
    }

    /* Find minimum and maximum length, bound *m by those */
    for (j = 1; j <= BMAX; j++)
        if (c[j])
            break;
    k = j; /* minimum code length */
    if (*m < j)
        *m = j;
    for (i = BMAX; i; i--)
        if (c[i])
            break;
    g = i; /* maximum code length */
    if (*m > i)
        *m = i;

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1)
        if ((y -= c[j]) < 0)
            return 2; /* bad input: more codes than bits */
    if ((y -= c[i]) < 0)
        return 2;
    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;
    xp = x + 2;
    while (--i) { /* note that i == g from above */
        *xp++ = (j += *p++);
    }

    /* Make a table of values in order of bit lengths */
    memzero((char *) v, sizeof(v));
    p = (unsigned *) b;
    i = 0;
    do {
        if ((j = *p++) != 0)
            v[x[j]++] = i;
    } while (++i < n);
    n = x[g]; /* set n to length of v */

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;  /* first Huffman code is zero */
    p = v;         /* grab values in bit order */
    h = -1;        /* no tables yet--level -1 */
    w = l[-1] = 0; /* no bits decoded yet */
    u[0] = NULL;   /* just to keep compilers happy */
    q = NULL;      /* ditto */
    z = 0;         /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++) {
        a = c[k];
        while (a--) {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + l[h]) {
                w += l[h++]; /* add bits already decoded */

                /* compute minimum size table less than or equal to *m bits */
                z = (z = g - w) > *m ? *m : z; /* upper limit */
                                               /* try a k-w bit table */
                if ((f = 1 << (j = k - w)) > a + 1) {
                    /* too few codes for k-w bit table */
                    f -= a + 1; /* deduct codes from patterns left */
                    xp = c + k;
                    /* try smaller tables up to z bits */
                    while (++j < z) {
                        if ((f <<= 1) <= *++xp)
                            break; /* enough codes to use up j bits */
                        f -= *xp;  /* else deduct codes from patterns */
                    }
                }
                if ((unsigned) w + j > el && (unsigned) w < el)
                    j = el - w; /* make EOB code end at table */
                z = 1 << j;     /* table entries for j-bit table */
                l[h] = j;       /* set table size in stack */

                /* allocate and link in new table */
                q = checked_malloc((z + 1) * sizeof(struct huft));
#ifdef DEBUG
                G.hufts += z + 1; /* track memory usage */
#endif
                *t = q + 1; /* link to list for huft_free() */
                *(t = &(q->v.t)) = NULL;
                u[h] = ++q; /* table starts after link */

                /* connect to last table, if there is one */
                if (h) {
                    x[h] = i; /* save pattern for backing up */
                    r.b =
                        (uint8_t) l[h - 1]; /* bits to dump before this table */
                    r.e = (uint8_t) (32 + j); /* bits in this table */
                    r.v.t = q;                /* pointer to this table */
                    j = (i & ((1 << w) - 1)) >> (w - l[h - 1]);
                    u[h - 1][j] = r; /* connect to last table */
                }
            }

            /* set up table entry in r */
            r.b = (uint8_t) (k - w);
            if (p >= v + n)
                r.e = INVALID_CODE; /* out of values--invalid code */
            else if (*p < s) {
                r.e = (uint8_t) (*p < 256 ? 32
                                          : 31); /* 256 is end-of-block code */
                r.v.n = (uint16_t) *p++; /* simple code is just the value */
            } else {
                r.e = e[*p - s]; /* non-simple--look up in lists */
                r.v.n = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f)
                q[j] = r;

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i & j; j >>= 1)
                i ^= j;
            i ^= j;

            /* backup over finished tables */
            while ((i & ((1 << w) - 1)) != x[h])
                w -= l[--h]; /* don't need to update q */
        }
    }

    /* return actual size of base table */
    *m = l[0];

    /* Return true (1) if we were given an incomplete table */
    return y != 0 && g != 1;
}

/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
int huft_free(t)
struct huft *t; /* table to free */
{
    register struct huft *p, *q;

    /* Go through linked list, freeing from the malloced (t[-1]) address. */
    p = t;
    while (p != NULL) {
        q = (--p)->v.t;
        free((void *) p);
        p = q;
    }
    return 0;
}
