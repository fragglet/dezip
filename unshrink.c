/*---------------------------------------------------------------------------

  unshrink.c

  Shrinking is a dynamic Lempel-Ziv-Welch compression algorithm with partial
  clearing.  Sadly, it uses more memory than any of the other algorithms (at
  a minimum, 8K+8K+16K, assuming 16-bit short ints), and this does not even
  include the output buffer (the other algorithms leave the uncompressed data
  in the work area, typically called slide[]).  For machines with a 64KB data
  space, this is a problem, particularly when text conversion is required and
  line endings have more than one character.  UnZip's solution is to use two
  roughly equal halves of outbuf for the ASCII conversion in such a case; the
  "unshrink" argument to flush() signals that this is the case.

  For large-memory machines, a second outbuf is allocated for translations,
  but only if unshrinking and only if translations are required.

              | binary mode  |        text mode
    ---------------------------------------------------
    big mem   |  big outbuf  | big outbuf + big outbuf2  <- malloc'd here
    small mem | small outbuf | half + half small outbuf


  ---------------------------------------------------------------------------*/


#include "unzip.h"

/*      MAX_BITS   13   (in unzip.h; defines size of global work area)  */
#define INIT_BITS  9
#define FIRST_ENT  257
#define CLEAR      256

#define OUTB(c) {\
    *outptr++=(uch)(c);\
    if (++outcnt==outbufsiz) {\
        flush(outbuf,outcnt,TRUE);\
        outcnt=0L;\
        outptr=outbuf;\
    }\
}

static void partial_clear __((void));

int codesize, maxcode, maxcodemax, free_ent;




/*************************/
/*  Function unshrink()  */
/*************************/

int unshrink()   /* return PK-type error code */
{
    register int code;
    register int stackp;
    int finchar;
    int oldcode;
    int incode;
    unsigned int outbufsiz;


    /* non-memory-limited machines:  allocate second (large) buffer for
     * textmode conversion in flush(), but only if needed */
#ifndef SMALL_MEM
    if (pInfo->textmode && !outbuf2 &&
        (outbuf2 = (uch *)malloc(TRANSBUFSIZ)) == NULL)
        return PK_MEM2;
#endif

    outptr = outbuf;
    outcnt = 0L;
    if (pInfo->textmode)
        outbufsiz = RAWBUFSIZ;
    else
        outbufsiz = OUTBUFSIZ;

    /* decompress the file */
    codesize = INIT_BITS;
    maxcode = (1 << codesize) - 1;
    maxcodemax = HSIZE;         /* (1 << MAX_BITS) */
    free_ent = FIRST_ENT;

    code = maxcodemax;
/*
    OvdL: -Ox with SCO's 3.2.0 cc gives
    a. warning: overflow in constant multiplication
    b. segmentation fault (core dumped) when using the executable
    for (code = maxcodemax; code > 255; code--)
        prefix_of[code] = -1;
 */
    do {
        prefix_of[code] = -1;
    } while (--code > 255);

    for (code = 255; code >= 0; code--) {
        prefix_of[code] = 0;
        suffix_of[code] = (uch)code;
    }

    READBITS(codesize,oldcode)  /* ; */
    if (zipeof)
        return PK_COOL;
    finchar = oldcode;

    OUTB(finchar)

    stackp = HSIZE;

    while (!zipeof) {
        READBITS(codesize,code)  /* ; */
        if (zipeof) {
            if (outcnt > 0L)
                flush(outbuf, outcnt, TRUE);   /* flush last, partial buffer */
            return PK_COOL;
        }

        while (code == CLEAR) {
            READBITS(codesize,code)  /* ; */
            switch (code) {
                case 1:
                    codesize++;
                    if (codesize == MAX_BITS)
                        maxcode = maxcodemax;
                    else
                        maxcode = (1 << codesize) - 1;
                    break;

                case 2:
                    partial_clear();
                    break;
            }

            READBITS(codesize,code)  /* ; */
            if (zipeof) {
                if (outcnt > 0L)
                    flush(outbuf, outcnt, TRUE);   /* partial buffer */
                return PK_COOL;
            }
        }


        /* special case for KwKwK string */
        incode = code;
        if (prefix_of[code] == -1) {
            stack[--stackp] = (uch)finchar;
            code = oldcode;
        }
        /* generate output characters in reverse order */
        while (code >= FIRST_ENT) {
            if (prefix_of[code] == -1) {
                stack[--stackp] = (uch)finchar;
                code = oldcode;
            } else {
                stack[--stackp] = suffix_of[code];
                code = prefix_of[code];
            }
        }

        finchar = suffix_of[code];
        stack[--stackp] = (uch)finchar;


        /* and put them out in forward order, block copy */
        if ((HSIZE - stackp + outcnt) < outbufsiz) {
            /* GRR:  this is not necessarily particularly efficient:
             *       typically output only 2-5 bytes per loop (more
             *       than a dozen rather rare?) */
            memcpy(outptr, &stack[stackp], HSIZE - stackp);
            outptr += HSIZE - stackp;
            outcnt += HSIZE - stackp;
            stackp = HSIZE;
        }
        /* output byte by byte if we can't go by blocks */
        else
            while (stackp < HSIZE)
                OUTB(stack[stackp++])


        /* generate new entry */
        code = free_ent;
        if (code < maxcodemax) {
            prefix_of[code] = oldcode;
            suffix_of[code] = (uch)finchar;

            do
                code++;
            while ((code < maxcodemax) && (prefix_of[code] != -1));

            free_ent = code;
        }
        /* remember previous code */
        oldcode = incode;
    }

    /* never reached? */
    /* flush last, partial buffer */
    if (outcnt > 0L)
        flush(outbuf, outcnt, TRUE);

    return PK_OK;

} /* end function unshrink() */



/******************************/
/*  Function partial_clear()  */
/******************************/

static void partial_clear()
{
    register int pr;
    register int cd;

    /* mark all nodes as potentially unused */
    for (cd = FIRST_ENT; cd < free_ent; cd++)
        prefix_of[cd] |= 0x8000;

    /* unmark those that are used by other nodes */
    for (cd = FIRST_ENT; cd < free_ent; cd++) {
        pr = prefix_of[cd] & 0x7fff;    /* reference to another node? */
        if (pr >= FIRST_ENT)    /* flag node as referenced */
            prefix_of[pr] &= 0x7fff;
    }

    /* clear the ones that are still marked */
    for (cd = FIRST_ENT; cd < free_ent; cd++)
        if ((prefix_of[cd] & 0x8000) != 0)
            prefix_of[cd] = -1;

    /* find first cleared node as next free_ent */
    cd = FIRST_ENT;
    while ((cd < maxcodemax) && (prefix_of[cd] != -1))
        cd++;
    free_ent = cd;
}
