/* ------------------------------------------------------------- */
/*
 * Shrinking is a Dynamic Ziv-Lempel-Welch compression algorithm
 * with partial clearing.
 *
 */

void partial_clear()
{
    register int pr;
    register int cd;

    /* mark all nodes as potentially unused */
    for (cd = first_ent; cd < free_ent; cd++)
        prefix_of[cd] |= 0x8000;

    /* unmark those that are used by other nodes */
    for (cd = first_ent; cd < free_ent; cd++) {
        pr = prefix_of[cd] & 0x7fff;    /* reference to another node? */
        if (pr >= first_ent)            /* flag node as referenced */
            prefix_of[pr] &= 0x7fff;
    }

    /* clear the ones that are still marked */
    for (cd = first_ent; cd < free_ent; cd++)
        if ((prefix_of[cd] & 0x8000) != 0)
            prefix_of[cd] = -1;

    /* find first cleared node as next free_ent */
    cd = first_ent;
    while ((cd < maxcodemax) && (prefix_of[cd] != -1))
        cd++;
    free_ent = cd;
}


/* ------------------------------------------------------------- */

void unShrink()
{
#define  GetCode(dest) READBIT(codesize,dest)

    register int code;
    register int stackp;
    int finchar;
    int oldcode;
    int incode;


    /* decompress the file */
    maxcodemax = 1 << max_bits;
    codesize = init_bits;
    maxcode = (1 << codesize) - 1;
    free_ent = first_ent;
    offset = 0;
    sizex = 0;

    for (code = maxcodemax; code > 255; code--)
        prefix_of[code] = -1;

    for (code = 255; code >= 0; code--) {
        prefix_of[code] = 0;
        suffix_of[code] = code;
    }

    GetCode(oldcode);
    if (zipeof)
        return;
    finchar = oldcode;

    OUTB(finchar);

    stackp = hsize;

    while (!zipeof) {
        GetCode(code);
        if (zipeof)
            return;

        while (code == clear) {
            GetCode(code);
            switch (code) {

            case 1:{
                    codesize++;
                    if (codesize == max_bits)
                        maxcode = maxcodemax;
                    else
                        maxcode = (1 << codesize) - 1;
                }
                break;

            case 2:
                partial_clear();
                break;
            }

            GetCode(code);
            if (zipeof)
                return;
        }


        /* special case for KwKwK string */
        incode = code;
        if (prefix_of[code] == -1) {
            stack[--stackp] = finchar;
            code = oldcode;
        }


        /* generate output characters in reverse order */
        while (code >= first_ent) {
            stack[--stackp] = suffix_of[code];
            code = prefix_of[code];
        }

        finchar = suffix_of[code];
        stack[--stackp] = finchar;


        /* and put them out in forward order, block copy */
        if ((hsize-stackp+outcnt) < OUTBUFSIZ) {
            zmemcpy(outptr,&stack[stackp],hsize-stackp);
            outptr += hsize-stackp;
            outcnt += hsize-stackp;
            stackp = hsize;
        }

        /* output byte by byte if we can't go by blocks */
        else while (stackp < hsize)
            OUTB(stack[stackp++]);


        /* generate new entry */
        code = free_ent;
        if (code < maxcodemax) {
            prefix_of[code] = oldcode;
            suffix_of[code] = finchar;

            do
                code++;
            while ((code < maxcodemax) && (prefix_of[code] != -1));

            free_ent = code;
        }

        /* remember previous code */
        oldcode = incode;
    }
}

