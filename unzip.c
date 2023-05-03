/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/*---------------------------------------------------------------------------

  UnZip - a zipfile extraction utility.  See below for make instructions, or
  read the comments in Makefile and the various Contents files for more de-
  tailed explanations.  To report a bug, submit a *complete* description via
  //www.info-zip.org/zip-bug.html; include machine type, operating system and
  version, compiler and version, and reasonably detailed error messages or
  problem report.  To join Info-ZIP, see the instructions in README.

  UnZip 5.x is a greatly expanded and partially rewritten successor to 4.x,
  which in turn was almost a complete rewrite of version 3.x.  For a detailed
  revision history, see UnzpHist.zip at quest.jpl.nasa.gov.  For a list of
  the many (near infinite) contributors, see "CONTRIBS" in the UnZip source
  distribution.

  UnZip 6.0 adds support for archives larger than 4 GiB using the Zip64
  extensions as well as support for Unicode information embedded per the
  latest zip standard additions.

  ---------------------------------------------------------------------------

  [from original zipinfo.c]

  This program reads great gobs of totally nifty information, including the
  central directory stuff, from ZIP archives ("zipfiles" for short).  It
  started as just a testbed for fooling with zipfiles, but at this point it
  is actually a useful utility.  It also became the basis for the rewrite of
  UnZip (3.16 -> 4.0), using the central directory for processing rather than
  the individual (local) file headers.

  Another dandy product from your buddies at Newtware!

  Author:  Greg Roelofs, newt@pobox.com, http://pobox.com/~newt/
           23 August 1990 -> April 1997

  ---------------------------------------------------------------------------

  Copyrights:  see accompanying file "LICENSE" in UnZip source distribution.
               (This software is free but NOT IN THE PUBLIC DOmain.)

  ---------------------------------------------------------------------------*/

#include "unzip.h" /* includes, typedefs, macros, prototypes, etc. */
#include "crypt.h"
#include "ttyio.h"

#include <langinfo.h>

/*************/
/* Constants */
/*************/

const unsigned mask_bits[17] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f,
                                0x003f, 0x007f, 0x00ff, 0x01ff, 0x03ff, 0x07ff,
                                0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff};

const char EndSigMsg[] = "\nnote:  didn't find end-of-central-dir "
                         "signature at end of central dir.\n";

const char CentSigMsg[] =
    "error:  expected central file header signature not found (file #%lu).\n";

static const char MustGiveExdir[] =
    "error:  must specify directory to which to extract with -d option\n";

static const char MustGivePasswd[] =
    "error:  must give decryption password with -P option\n";

static const char UnzipUsage[] = "\
Usage: unzip [-opts[modifiers]] file[.zip] [list] [-x xlist] [-d exdir]\n\
 Default action is to extract files in list, except those in xlist, to exdir;\n\
  file[.zip] may be a wildcard.\n\
  -p  extract files to pipe, no messages     -l  list files (short format)\n\
  -f  freshen existing files, create none    -t  test compressed archive data\n\
  -u  update files, create if necessary      -z  display archive comment only\n\
  -v  list verbosely/show version info       -T  timestamp archive to latest\n\
  -x  exclude files that follow (in xlist)   -d  extract files into exdir\n\
modifiers:\n\
  -n  never overwrite existing files         -q  quiet mode (-qq => quieter)\n\
  -o  overwrite files WITHOUT prompting      -a  auto-convert any text files\n\
  -j  junk paths (do not make directories)   -aa treat ALL files as text\n\
  -U  use escapes for all non-ASCII Unicode  -UU ignore any Unicode fields\n\
  -C  match filenames case-insensitively     -L  make (some) names lowercase\n\
  -X  restore UID/GID info                   -V  retain VMS version numbers\n\
  -K  keep setuid/setgid/tacky permissions\n\
Examples:\n\
  unzip data1 -x joe   => extract all files except joe from zipfile data1.zip\n\
  unzip -p foo | more  => send contents of foo.zip via pipe into program more\n\
  unzip -fo foo ReadMe => quietly replace existing ReadMe if archive file newer\n";

/* initialization of sigs is completed at runtime */
char central_hdr_sig[4] = {0, 0, 0x01, 0x02};
char local_hdr_sig[4] = {0, 0, 0x03, 0x04};
char end_central_sig[4] = {0, 0, 0x05, 0x06};
char end_central64_sig[4] = {0, 0, 0x06, 0x06};
char end_centloc64_sig[4] = {0, 0, 0x06, 0x07};

/* default filenames vector */
static const char *default_fnames[2] = {"*", NULL};

struct globals G;

static void init_globals()
{
    memzero(&G, sizeof(G));

    G.UzO.lflag = (-1);
    G.wildzipfn = "";
    G.pfnames = (char **) default_fnames;
    G.pxnames = (char **) &default_fnames[1];
    G.pInfo = G.info;
    G.sol = TRUE; /* at start of line */

    G.message = UzpMessagePrnt;
    G.mpause = UzpMorePause;
    G.decr_passwd = UzpPassword;

    G.echofd = -1;
}

/* upon interrupt, turn on echo and exit cleanly */
static void signal_handler(int signal)
{
    /* newline if not start of line to stderr */
    (*G.message)(slide, 0L, 0x21);
    echon();

    exit(IZ_CTRLC);
}

/*******************************/
/*  Primary UnZip entry point  */
/*******************************/

int main(int argc, char *argv[])
{
    int i;
    int retcode, error = FALSE;
    char *codeset;

    init_globals();

    /* initialize international char support to the current environment */
    setlocale(LC_CTYPE, "");

    /* see if can use UTF-8 Unicode locale */
    /* get the codeset (character set encoding) currently used */

    codeset = nl_langinfo(CODESET);
    /* is the current codeset UTF-8 ? */
    if (codeset != NULL && strcmp(codeset, "UTF-8") == 0) {
        /* successfully found UTF-8 char coding */
        G.native_is_utf8 = TRUE;
    } else {
        /* Current codeset is not UTF-8 or cannot be determined. */
        G.native_is_utf8 = FALSE;
    }
    /* Note: At least for UnZip, trying to change the process codeset to
     *       UTF-8 does not work.  For the example Linux setup of the
     *       UnZip maintainer, a successful switch to "en-US.UTF-8"
     *       resulted in garbage display of all non-basic ASCII characters.
     */

    /* initialize Unicode */
    G.unicode_escape_all = 0;
    G.unicode_mismatch = 0;

    G.unipath_version = 0;
    G.unipath_checksum = 0;
    G.unipath_filename = NULL;

    /* Set signal handler for restoring echo */
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);

#if defined(DEBUG)
    /* test if we can support large files - 10/6/04 EG */
    if (sizeof(off_t) < 8) {
        Info(slide, 1,
             ((char *) slide, "LARGE_FILE_SUPPORT set but not supported\n"));
        retcode = PK_BADERR;
        goto cleanup_and_exit;
    }
    /* test if we can show 64-bit values */
    {
        off_t z = ~(off_t) 0; /* z should be all 1s now */
        char *sz;

        sz = format_off_t(z, OFF_T_HEX_DOT_WID, "X");
        if (sz[0] != 'F' || strlen(sz) != 16) {
            z = 0;
        }

        /* shift z so only MSB is set */
        z <<= 63;
        sz = format_off_t(z, OFF_T_HEX_DOT_WID, "X");
        if (sz[0] != '8' || strlen(sz) != 16) {
            Info(slide, 1,
                 ((char *) slide, "Can't show 64-bit values correctly\n"));
            retcode = PK_BADERR;
            goto cleanup_and_exit;
        }
    }
#endif

    G.noargs = (argc == 1); /* no options, no zipfile, no anything */

    if ((error = envargs(&argc, &argv, ENV_UNZIP, ENV_UNZIP2)) != PK_OK)
        perror("envargs: cannot get memory for arguments");

    if (!error) {
        /* Check the length of all passed command line parameters.
         * Command arguments might get sent through the Info() message
         * system, which uses the sliding window area as string buffer.
         * As arguments may additionally get fed through one of the FnFilter
         * macros, we require all command line arguments to be shorter than
         * WSIZE/4 (and ca. 2 standard line widths for fixed message text).
         */
        for (i = 1; i < argc; i++) {
            if (strlen(argv[i]) > ((WSIZE >> 2) - 160)) {
                Info(slide, 1,
                     ((char *) slide,
                      "error:  command line "
                      "parameter #%d exceeds internal size limit\n",
                      i));
                retcode = PK_PARAM;
                goto cleanup_and_exit;
            }
        }
        error = uz_opts(&argc, &argv);
    }

    if (argc < 0 || error) {
        retcode = error;
        goto cleanup_and_exit;
    }

    /*---------------------------------------------------------------------------
        Now get the zipfile name from the command line and then process any re-
        maining options and file specifications.
      ---------------------------------------------------------------------------*/

    G.wildzipfn = *argv++;

    G.filespecs = argc;
    G.xfilespecs = 0;

    if (argc > 0) {
        int in_files = FALSE, in_xfiles = FALSE;
        char **pp = argv - 1;

        G.process_all_files = FALSE;
        G.pfnames = argv;
        while (*++pp) {
            Trace((stderr, "pp - argv = %d\n", pp - argv));
            if (!G.UzO.exdir && strncmp(*pp, "-d", 2) == 0) {
                int firstarg = (pp == argv);

                G.UzO.exdir = (*pp) + 2;
                if (in_files) { /* ... zipfile ... -d exdir ... */
                    *pp = NULL; /* terminate G.pfnames */
                    G.filespecs = pp - G.pfnames;
                    in_files = FALSE;
                } else if (in_xfiles) {
                    *pp = NULL; /* terminate G.pxnames */
                    G.xfilespecs = pp - G.pxnames;
                    /* "... -x xlist -d exdir":  nothing left */
                }
                /* first check for "-dexdir", then for "-d exdir" */
                if (*G.UzO.exdir == '\0') {
                    if (*++pp)
                        G.UzO.exdir = *pp;
                    else {
                        Info(slide, 1, ((char *) slide, MustGiveExdir));
                        /* don't extract here by accident */
                        retcode = PK_PARAM;
                        goto cleanup_and_exit;
                    }
                }
                if (firstarg) { /* ... zipfile -d exdir ... */
                    if (pp[1]) {
                        G.pfnames = pp + 1; /* argv+2 */
                        G.filespecs =
                            argc - (G.pfnames - argv); /* for now... */
                    } else {
                        G.process_all_files = TRUE;
                        G.pfnames =
                            (char **) default_fnames; /* GRR: necessary? */
                        G.filespecs = 0;              /* GRR: necessary? */
                        break;
                    }
                }
            } else if (!in_xfiles) {
                if (strcmp(*pp, "-x") == 0) {
                    in_xfiles = TRUE;
                    if (pp == G.pfnames) {
                        G.pfnames = (char **) default_fnames; /* defaults */
                        G.filespecs = 0;
                    } else if (in_files) {
                        *pp = 0;                      /* terminate G.pfnames */
                        G.filespecs = pp - G.pfnames; /* adjust count */
                        in_files = FALSE;
                    }
                    G.pxnames = pp + 1; /* excluded-names ptr starts after -x */
                    G.xfilespecs =
                        argc - (G.pxnames - argv); /* anything left */
                } else
                    in_files = TRUE;
            }
        }
    } else
        G.process_all_files = TRUE; /* for speed */

    if (G.UzO.exdir != NULL && !G.extract_flag) /* -d ignored */
        Info(slide, 1,
             ((char *) slide, "caution:  not extracting; -d ignored\n"));

    /* set Unicode-escape-all if option -U used */
    if (G.UzO.U_flag == 1)
        G.unicode_escape_all = TRUE;

    /*---------------------------------------------------------------------------
        Okey dokey, we have everything we need to get started.  Let's roll.
      ---------------------------------------------------------------------------*/

    retcode = process_zipfiles();

cleanup_and_exit:
    return (retcode);
}

int uz_opts(pargc, pargv)
int *pargc;
char ***pargv;
{
    char **argv, *s;
    int argc, c, error = FALSE, negative = 0, showhelp = 0;

    argc = *pargc;
    argv = *pargv;

    while (++argv, (--argc > 0 && *argv != NULL && **argv == '-')) {
        s = *argv + 1;
        while ((c = *s++) != 0) {
            switch (c) {
            case '-':
                ++negative;
                break;
            case 'a':
                if (negative) {
                    G.UzO.aflag = MAX(G.UzO.aflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.aflag;
                break;
            case 'b':
                if (negative) {
                    negative = 0; /* do nothing:  "-b" is default */
                } else {
                    G.UzO.aflag = 0;
                }
                break;
            case 'c':
                if (negative) {
                    G.UzO.cflag = FALSE, negative = 0;
#ifdef NATIVE
                    G.UzO.aflag = 0;
#endif
                } else {
                    G.UzO.cflag = TRUE;
#ifdef NATIVE
                    G.UzO.aflag = 2; /* so you can read it on the screen */
#endif
                }
                break;
            case 'C': /* -C:  match filenames case-insensitively */
                G.UzO.C_flag = !negative;
                negative = 0;
            case 'd':
                if (negative) { /* negative not allowed with -d exdir */
                    Info(slide, 1, ((char *) slide, MustGiveExdir));
                    return (PK_PARAM); /* don't extract here by accident */
                }
                if (G.UzO.exdir != NULL) {
                    Info(slide, 1,
                         ((char *) slide,
                          "error:  -d option "
                          "used more than once (only one exdir allowed)\n"));
                    return (PK_PARAM); /* GRR:  stupid restriction? */
                }
                /* first check for "-dexdir", then for "-d exdir" */
                G.UzO.exdir = s;
                if (*G.UzO.exdir == '\0') {
                    if (argc <= 1) {
                        /* else G.UzO.exdir points at extraction dir */
                        Info(slide, 1, ((char *) slide, MustGiveExdir));
                        return (PK_PARAM);
                    }
                    --argc;
                    G.UzO.exdir = *++argv;
                    if (*G.UzO.exdir == '-') {
                        Info(slide, 1, ((char *) slide, MustGiveExdir));
                        return (PK_PARAM);
                    }
                }
                /* G.UzO.exdir now points at extraction dir (-dexdir or
                 *  -d exdir); point s at end of exdir to avoid mis-
                 *  interpretation of exdir characters as more options
                 */
                if (*s != 0)
                    while (*++s != 0)
                        ;
                break;
            case 'D': /* -D: Skip restoring dir (or any) timestamp. */
                if (negative) {
                    G.UzO.D_flag = MAX(G.UzO.D_flag - negative, 0);
                    negative = 0;
                } else
                    G.UzO.D_flag++;
                break;
            case 'e': /* just ignore -e, -x options (extract) */
                break;
            case 'f': /* "freshen" (extract only newer files) */
                G.UzO.fflag = !negative;
                G.UzO.uflag = !negative;
                negative = 0;
                break;
            case 'h': /* just print help message and quit */
                if (showhelp == 0) {
                    if (*s == 'h')
                        showhelp = 2;
                    else {
                        showhelp = 1;
                    }
                }
                break;
            case 'j': /* junk pathnames/directory structure */
                G.UzO.jflag = !negative;
                negative = 0;
                break;
            case 'K':
                G.UzO.K_flag = !negative;
                negative = 0;
                break;
            case 'l':
                if (negative) {
                    G.UzO.vflag = MAX(G.UzO.vflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.vflag;
                break;
            case 'L': /* convert (some) filenames to lowercase */
                if (negative) {
                    G.UzO.L_flag = MAX(G.UzO.L_flag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.L_flag;
                break;
            case 'n': /* don't overwrite any files */
                G.UzO.overwrite_none = !negative;
                negative = 0;
                break;
            case 'o': /* OK to overwrite files without prompting */
                if (negative) {
                    G.UzO.overwrite_all =
                        MAX(G.UzO.overwrite_all - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.overwrite_all;
                break;
            case 'p': /* pipes:  extract to stdout, no messages */
                if (negative) {
                    G.UzO.qflag = MAX(G.UzO.qflag - 999, 0);
                    negative = 0;
                } else {
                    G.UzO.cflag = TRUE;
                    G.UzO.qflag += 999;
                }
                break;
            /* GRR:  yes, this is highly insecure, but dozens of people
             * have pestered us for this, so here we go... */
            case 'P':
                if (negative) { /* negative not allowed with -P passwd */
                    Info(slide, 1, ((char *) slide, MustGivePasswd));
                    return (PK_PARAM); /* don't extract here by accident */
                }
                if (G.UzO.pwdarg != NULL) {
                    break;
                }
                /* first check for "-Ppasswd", then for "-P passwd" */
                G.UzO.pwdarg = s;
                if (*G.UzO.pwdarg == '\0') {
                    if (argc <= 1) {
                        /* pwdarg points at decryption password */
                        Info(slide, 1, ((char *) slide, MustGivePasswd));
                        return (PK_PARAM);
                    }
                    --argc;
                    G.UzO.pwdarg = *++argv;
                    if (*G.UzO.pwdarg == '-') {
                        Info(slide, 1, ((char *) slide, MustGivePasswd));
                        return (PK_PARAM);
                    }
                }
                /* pwdarg now points at decryption password (-Ppasswd or
                 *  -P passwd); point s at end of passwd to avoid mis-
                 *  interpretation of passwd characters as more options
                 */
                if (*s != 0)
                    while (*++s != 0)
                        ;
                break;
            case 'q': /* quiet:  fewer comments/messages */
                if (negative) {
                    G.UzO.qflag = MAX(G.UzO.qflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.qflag;
                break;
            case 't':
                G.UzO.tflag = !negative;
                negative = 0;
                break;
            case 'T':
                G.UzO.T_flag = !negative;
                negative = 0;
                break;
            case 'u': /* update (extract only new and newer files) */
                G.UzO.uflag = !negative;
                negative = 0;
                break;
            case 'U': /* escape UTF-8, or disable UTF-8 support */
                if (negative) {
                    G.UzO.U_flag = MAX(G.UzO.U_flag - negative, 0);
                    negative = 0;
                } else
                    G.UzO.U_flag++;
                break;
            case 'v': /* verbose */
                if (negative) {
                    G.UzO.vflag = MAX(G.UzO.vflag - negative, 0);
                    negative = 0;
                } else if (G.UzO.vflag)
                    ++G.UzO.vflag;
                else
                    G.UzO.vflag = 2;
                break;
            case 'V': /* Version (retain VMS/DEC-20 file versions) */
                G.UzO.V_flag = !negative;
                negative = 0;
                break;
            case 'x': /* extract:  default */
                break;
            case 'X': /* restore owner/protection info (need privs?) */
                if (negative) {
                    G.UzO.X_flag = MAX(G.UzO.X_flag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.X_flag;
                break;
            case 'z': /* display only the archive comment */
                if (negative) {
                    G.UzO.zflag = MAX(G.UzO.zflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.zflag;
                break;
            case ':': /* allow "parent dir" path components */
                if (negative) {
                    G.UzO.ddotflag = MAX(G.UzO.ddotflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.ddotflag;
                break;
            case '^': /* allow control chars in filenames */
                if (negative) {
                    G.UzO.cflxflag = MAX(G.UzO.cflxflag - negative, 0);
                    negative = 0;
                } else
                    ++G.UzO.cflxflag;
                break;
            default:
                error = TRUE;
                break;

            } /* end switch */
        }     /* end while (not end of argument string) */
    }         /* end while (not done with switches) */

    /*---------------------------------------------------------------------------
        Check for nonsensical combinations of options.
      ---------------------------------------------------------------------------*/

    if (showhelp > 0) { /* just print help message and quit */
        *pargc = -1;
        return usage(PK_OK);
    }

    if ((G.UzO.cflag && (G.UzO.tflag || G.UzO.uflag)) ||
        (G.UzO.tflag && G.UzO.uflag) || (G.UzO.fflag && G.UzO.overwrite_none)) {
        Info(slide, 1,
             ((char *) slide, "error: -fn or any combination of "
                              "-c, -l, -p, -t, -u and -v options invalid\n"));
        error = TRUE;
    }
    if (G.UzO.aflag > 2)
        G.UzO.aflag = 2;
    if (G.UzO.overwrite_all && G.UzO.overwrite_none) {
        Info(slide, 1,
             ((char *) slide,
              "caution:  both -n and -o specified; ignoring -o\n"));
        G.UzO.overwrite_all = FALSE;
    }

    if ((argc-- == 0) || error) {
        *pargc = argc;
        *pargv = argv;
        if (G.UzO.vflag >= 2 && argc == -1) { /* "unzip -v" */
            return PK_OK;
        }
        if (!G.noargs && !error)
            error = TRUE; /* had options (not -h or -v) but no zipfile */
        return usage(error);
    }

    if (G.UzO.cflag || G.UzO.tflag || G.UzO.vflag || G.UzO.zflag ||
        G.UzO.T_flag)
        G.extract_flag = FALSE;
    else
        G.extract_flag = TRUE;

    *pargc = argc;
    *pargv = argv;
    return PK_OK;
}

#define QUOT  ' '
#define QUOTS ""

int usage(error) /* return PK-type error code */
int error;
{
    int flag = (error ? 1 : 0);

    Info(slide, flag, ((char *) slide, UnzipUsage));

    if (error)
        return PK_PARAM;
    else
        return PK_COOL; /* just wanted usage screen: no error */
}

void *checked_realloc(void *old, size_t sz)
{
    void *result = realloc(old, sz);
    if (result == NULL && sz > 0) {
        Info(slide, 1, ((char *) slide, "failed to allocate %d bytes\n", sz));
        exit(PK_MEM);
    }
    return result;
}
