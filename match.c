/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/*---------------------------------------------------------------------------

  The match() routine recursively compares a string to a "pattern" (regular
  expression), returning TRUE if a match is found or FALSE if not.  This
  version is specifically for use with unzip.c:  as did the previous match()
  routines from SEA and J. Kercheval, it leaves the case (upper, lower, or
  mixed) of the string alone, but converts any uppercase characters in the
  pattern to lowercase if indicated by the global var pInfo->lcflag (which
  is to say, string is assumed to have been converted to lowercase already,
  if such was necessary).

  GRR:  reversed order of text, pattern in matche() (now same as match());
        added ignore_case/ic flags, Case() macro.

  PaulK:  replaced matche() with recmatch() from Zip, modified to have an
          ignore_case argument; replaced test frame with simpler one.

  ---------------------------------------------------------------------------

  Copyright on recmatch() from Zip's util.c (although recmatch() was almost
  certainly written by Mark Adler...ask me how I can tell :-) ):

     Copyright (C) 1990-1992 Mark Adler, Richard B. Wales, Jean-loup Gailly,
     Kai Uwe Rommel and Igor Mandrichenko.

     Permission is granted to any individual or institution to use, copy,
     or redistribute this software so long as all of the original files are
     included unmodified, that it is not sold for profit, and that this copy-
     right notice is retained.

  ---------------------------------------------------------------------------

  Match the pattern (wildcard) against the string (fixed):

     match(string, pattern, ignore_case, sepc);

  returns TRUE if string matches pattern, FALSE otherwise.  In the pattern:

     `*' matches any sequence of characters (zero or more)
     `?' matches any single character
     [SET] matches any character in the specified set,
     [!SET] or [^SET] matches any character not in the specified set.

  A set is composed of characters or ranges; a range looks like ``character
  hyphen character'' (as in 0-9 or A-Z).  [0-9a-zA-Z_] is the minimal set of
  characters allowed in the [..] pattern construct.  Other characters are
  allowed (i.e., 8-bit characters) if your system will support them.

  To suppress the special syntactic significance of any of ``[]*?!^-\'', in-
  side or outside a [..] construct, and match the character exactly, precede
  it with a ``\'' (backslash).

  Note that "*.*" and "*." are treated specially under MS-DOS if DOSWILD is
  defined.  See the DOSWILD section below for an explanation.  Note also
  that with VMSWILD defined, '%' is used instead of '?', and sets (ranges)
  are delimited by () instead of [].

  ---------------------------------------------------------------------------*/

/* define ToLower() in here (for Unix, define ToLower to be macro (using
 * isupper()); otherwise just use tolower() */
#include "unzip.h"

#define Case(x) (ic ? ToLower(x) : (x))

#define WILDCHAR  '?'
#define BEG_RANGE '['
#define END_RANGE ']'

static int recmatch(const uint8_t *pattern, const uint8_t *string,
                    int ignore_case);
static char *isshexp(const char *p);
static int namecmp(const char *s1, const char *s2);

/* match() is a shell to recmatch() to return only Boolean values. */

int match(string, pattern, ignore_case) const char *string, *pattern;
int ignore_case;
{
    return recmatch((uint8_t *) pattern, (uint8_t *) string, ignore_case) == 1;
}

/* Recursively compare the sh pattern p with the string s and return 1 if
 * they match, and 0 or 2 if they don't or if there is a syntax error in the
 * pattern.  This routine recurses on itself no more deeply than the number
 * of characters in the pattern. */
static int recmatch(p, s, ic) const uint8_t *p; /* sh pattern to match */
const uint8_t *s; /* string to which to match it */
int ic;           /* true for case insensitivity */
{
    unsigned int c; /* pattern char or start of range in [-] loop */

    /* Get first character, the pattern for new recmatch calls follows */
    c = *p;
    INCSTR(p);

    /* If that was the end of the pattern, match if string empty too */
    if (c == 0)
        return *s == 0;

    /* '?' (or '%') matches any character (but not an empty string). */
    if (c == WILDCHAR)
        return *s ? recmatch(p, s + CLEN(s), ic) : 0;

    /* '*' matches any number of characters, including zero */
    if (c == '*') {
        if (*p == 0)
            return 1;
        if (isshexp((const char *) p) != NULL) {
            /* pattern contains more wildcards, continue with recursion... */
            for (; *s; INCSTR(s))
                if ((c = recmatch(p, s, ic)) != 0)
                    return (int) c;
            return 2; /* 2 means give up--match will return false */
        }
        /* Optimization for rest of pattern being a literal string:
         * If there are no other shell expression chars in the rest
         * of the pattern behind the multi-char wildcard, then just
         * compare the literal string tail.
         */
        const uint8_t *srest;

        srest = s + (strlen((const char *) s) - strlen((const char *) p));
        if (srest - s < 0)
            /* remaining literal string from pattern is longer than rest
             * of test string, there can't be a match
             */
            return 0;

            /* compare the remaining literal pattern string with the last
             * bytes of the test string to check for a match
             */
#ifdef _MBCS
        const uint8_t *q = s;

        /* MBCS-aware code must not scan backwards into a string from
         * the end.
         * So, we have to move forward by character from our well-known
         * character position s in the test string until we have
         * advanced to the srest position.
         */
        while (q < srest)
            INCSTR(q);
        /* In case the byte *srest is a trailing byte of a multibyte
         * character in the test string s, we have actually advanced
         * past the position (srest).
         * For this case, the match has failed!
         */
        if (q != srest)
            return 0;
        return ((ic ? namecmp((const char *) p, (const char *) q)
                    : strcmp((const char *) p, (const char *) q)) == 0);
#else  /* !_MBCS */
        return ((ic ? namecmp((const char *) p, (const char *) srest)
                    : strcmp((const char *) p, (const char *) srest)) == 0);
#endif /* ?_MBCS */
    }

    /* Parse and process the list of characters and ranges in brackets */
    if (c == BEG_RANGE) {
        int e;            /* flag true if next char to be taken literally */
        const uint8_t *q; /* pointer to end of [-] group */
        int r;            /* flag true to match anything but the range */

        if (*s == 0) /* need a character to match */
            return 0;
        p += (r = (*p == '!' || *p == '^')); /* see if reverse */
        for (q = p, e = 0; *q; INCSTR(q))    /* find closing bracket */
            if (e)
                e = 0;
            else if (*q == '\\') /* GRR:  change to ^ for MS-DOS, OS/2? */
                e = 1;
            else if (*q == END_RANGE)
                break;
        if (*q != END_RANGE) /* nothing matches if bad syntax */
            return 0;
        for (c = 0, e = (*p == '-'); p < q; INCSTR(p)) {
            /* go through the list */
            if (!e && *p == '\\') /* set escape flag if \ */
                e = 1;
            else if (!e && *p == '-') /* set start of range if - */
                c = *(p - 1);
            else {
                unsigned int cc = Case(*s);

                if (*(p + 1) != '-')
                    for (c = c ? c : *p; c <= *p; c++) /* compare range */
                        if ((unsigned) Case(c) == cc) /* typecast for MSC bug */
                            return r ? 0 : recmatch(q + 1, s + 1, ic);
                c = e = 0; /* clear range, escape flags */
            }
        }
        return r ? recmatch(q + CLEN(q), s + CLEN(s), ic) : 0;
        /* bracket match failed */
    }

    /* if escape ('\\'), just compare next character */
    if (c == '\\' && (c = *p++) == 0) /* if \ at end, then syntax error */
        return 0;

    /* just a character--compare it */
    return Case((uint8_t) c) == Case(*s) ? recmatch(p, s + CLEN(s), ic) : 0;
}

/* If p is a sh expression, a pointer to the first special character is
   returned.  Otherwise, NULL is returned. */
static char *isshexp(p) const char *p;
{
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p + 1))
            p++;
        else if (*p == WILDCHAR || *p == '*' || *p == BEG_RANGE)
            return (char *) p;
    return NULL;
}

static int namecmp(s1, s2) const char *s1, *s2;
{
    int d;

    for (;;) {
        d = (int) ToLower((uint8_t) *s1) - (int) ToLower((uint8_t) *s2);

        if (d || *s1 == 0 || *s2 == 0)
            return d;

        s1++;
        s2++;
    }
}

int iswild(const char *p)
{
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p + 1))
            ++p;
        else if (*p == '?' || *p == '*' || *p == '[')
            return TRUE;

    return FALSE;
}

#ifdef TEST_MATCH

int main(int argc, char **argv)
{
    char pat[256], str[256];

    for (;;) {
        puts("Pattern (return to exit): ");
        gets(pat);
        if (!pat[0])
            break;
        for (;;) {
            puts("String (return for new pattern): ");
            gets(str);
            if (!str[0])
                break;
            printf("Case sensitive: %s  insensitive: %s\n",
                   match(str, pat, 0) ? "YES" : "NO",
                   match(str, pat, 1) ? "YES" : "NO");
        }
    }
    exit(0);
}

#endif /* TEST_MATCH */
