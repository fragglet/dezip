/*---------------------------------------------------------------------------

  mapname.c

  This routine changes DEC-20, VAX/VMS, and DOS-style filenames into normal
  Unix names (and vice versa, in some cases); it also creates any necessary 
  directories, if the -d switch was specified.

  ---------------------------------------------------------------------------

  Action:  renames argument files as follows:

     strips Unix and PKZIP DOS path name from front (up to rightmost '/') if
       present.
     strips DEC device:, node:: names from front (up to rightmost ':') if
       present.  (This also takes care of any DOS drive: artifacts.)
     strips DEC-20 <directory> or VMS [directory] name if present.
     strips DEC-20 version number from end (everything after 2nd dot) if
       present.
     strips VMS generation number from end (everything after ';') if present,
       unless "-V" switch specified.
     honors DEC-20 CTRL-V quote for special characters.
     discards unquoted unprintable characters.
     [VMS] converts Unix-style pathnames to VMS style.

     Returns 0 if no error, 1 if filename truncated, 2 for any other error.

  ---------------------------------------------------------------------------

  Author:  David Kirschbaum, 25 Apr 90
     (Based on good old xxu.c by Frank da Cruz, CUCCA)
     Subsequent tweaks by Bill Davidsen, James Dugal, Larry Jones,
     Mark Edwards, Greg Roelofs, Antoine Verheijen.

  ---------------------------------------------------------------------------

  Dot notes:

     - Unix allows multiple dots in directory names; MS-DOS and OS/2 FAT
       allow one; VMS does not allow any.  Things are almost as bad with
       regular filenames (VMS allows a single dot but TOPS-20 allows two,
       if you count the one in front of the version number).  As of v4.04,
       mapname converts directory-name dots to underscores on VMS, but it
       otherwise leaves the dots alone.  Since it is now possible to create
       zipfiles under Unix, this whole routine pretty much needs to be
       rewritten (different rules for each output OS and for different 
       parts of the path name).
     - If each zip program stores local-format names (like the coming VMS
       one will), it would probably be best to convert to an intermediate 
       format first (assuming we're not extracting under the same OS as
       that under which the zipfile was created), then from that to the 
       current operating system's format.

  ------------------------------------------------------------------------- */


#include "unzip.h"


/********************/
/*  Mapname Defines */
/********************/

#ifdef VMS
#define PERMS   0
#else
#define PERMS   0777
#endif

#ifndef NO_MKDIR
#ifdef DOS_OS2
#if (_MSC_VER >= 600)           /* have special MSC mkdir prototype */
#include <direct.h>
#else                           /* own prototype because dir.h conflicts?? */
int mkdir(const char *path);
#endif                          /* !(MSC 6.0 or later) */
#define MKDIR(path,mode)   mkdir(path)
#else                           /* !DOS_OS2 */
#define MKDIR(path,mode)   mkdir(path,mode)
#endif
#endif                          /* !NO_MKDIR */

/***************************/
/*  Function mapped_name() */
/***************************/

mapped_name()
{
#ifdef NO_MKDIR
    char command[FILNAMSIZ+40]; /* buffer for system() call */
#endif
#ifdef VMS
    int stat_val;               /* temp. holder for stat() return value */
    char *dp, *xp;              /* ptrs to directory name */
#endif
    char name[FILNAMSIZ];       /* file name buffer */
    char *pp, *cp, *cdp;        /* character pointers */
    char delim = '\0';          /* Directory Delimiter */
    int dc = 0;                 /* Counters */
    int quote = FALSE;          /* Flags */
    int indir = FALSE;
    int done = FALSE;
    register int workch;        /* hold the character being tested */



/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

#ifdef MAP_DEBUG
    fprintf(stderr, "%s ", filename);   /* echo name of this file */
#endif
    pp = name;                  /* Point to translation buffer */
    *name = '\0';               /* Initialize buffer */

    if (dflag) {                /* -d => retain directory structure */
        cdp = (char *) malloc(strlen(filename) + 3);  /* place for */
        if (cdp == NULL) {      /*   holding directory name */
            fprintf(stderr, "malloc failed in conversion of [%s]\n", filename);
            return (2);
        }
#ifdef VMS
        *cdp++ = '[';
        xp = cdp;               /* always points to last non-NULL char */
        *cdp++ = '.';
#endif
#ifdef MACOS
        *cdp = ':';             /* the Mac uses ':' as a directory separator */
        cdp[1] = '\0';
#else
        *cdp = '\0';
#endif
    }
    dc = 0;                     /* Filename dot counter */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    for (cp = filename; (workch = *cp++) != 0  &&  !done;) {

        if (quote) {            /* If this char quoted... */
            *pp++ = workch;     /*  include it literally. */
            quote = FALSE;
        } else if (indir) {     /* If in directory name... */
            if (workch == delim)
                indir = FALSE;  /*  look for end delimiter. */
        } else
            switch (workch) {
            case '<':           /* Discard DEC-20 directory name */
                indir = TRUE;
                delim = '>';
                break;
            case '[':           /* Discard VMS directory name */
                indir = TRUE;
                delim = ']';
                break;
            case '/':           /* Discard Unix path name... */
            case '\\':          /*  or MS-DOS path name...
                                 *  UNLESS -d flag was given. */
                /*
                 * Special processing case:  if -d flag was specified on
                 * command line, create any necessary directories included
                 * in the pathname.  Creation of directories is straight-
                 * forward on BSD and MS-DOS machines but requires use of
                 * the system() command on SysV systems (or any others which
                 * don't have mkdir()).  The stat() check is necessary with
                 * MSC because it doesn't have an EEXIST errno, and it saves
                 * the overhead of multiple system() calls on SysV machines.
                 */

                if (dflag) {
                    *pp = '\0';
#ifdef VMS
                    dp = name;
                    while (*++xp = *dp++)   /* copy name to cdp, while */
                        if (*xp == '.')     /*   changing all dots... */
                            *xp = '_';      /*   ...to underscores */
                    strcpy(xp, ".dir");    /* add extension for stat check */
                    stat_val = stat(cdp, &statbuf);
                    *xp = '\0';         /* remove extension for all else */
                    if (stat_val) {     /* doesn't exist, so create */
#else
                    strcat(cdp, name);
                    if (stat(cdp, &statbuf)) {  /* doesn't exist, so create */
#endif
#ifdef NO_MKDIR
                        sprintf(command, "IFS=\" \t\n\" /bin/mkdir %s 2>/dev/null", cdp);
                        if (system(command)) {
#else
                        if (MKDIR(cdp, PERMS) == -1) {
#endif
                            perror(cdp);
                            free(cdp);
                            fprintf(stderr, "Unable to process [%s]\n", filename);
                            return (2);
                        }
                    } else if (!(statbuf.st_mode & S_IFDIR)) {
                        fprintf(stderr, "%s:  exists but is not a directory\n",
                                cdp);
                        free(cdp);
                        fprintf(stderr, "unable to process [%s]\n", filename);
                        return (2);
                    }
#ifdef VMS
                    *xp = '/';  /* for now... (mkdir()) */
#else /* !VMS */
#ifdef MACOS
                    strcat(cdp, ":");
#else /* !MACOS */
                    strcat(cdp, "/");
#endif /* ?MACOS */
#endif /* ?VMS */
                }               /***** FALL THROUGH to ':' case  **** */
            case ':':           /* Discard DEC dev: or node:: name */
                pp = name;
                break;
            case '.':                   /* DEC-20 generation number
                                         * or MS-DOS type */
#ifdef NUKE_DOTS
                if (++dc == 1)          /* Keep first dot */
                    *pp++ = workch;
#else
                ++dc;                   /* Not used, but what the hell. */
                *pp++ = workch;
#endif
                break;
            case ';':                   /* VMS generation or DEC-20 attrib */
                if (V_flag)             /* If requested, save VMS ";##" */
                    *pp++ = workch;     /*  version info; else discard */
                else                    /*  everything starting with */
                    done = TRUE;        /*  semicolon.  (Worry about */
                break;                  /*  DEC-20 later.) */
            case '\026':                /* Control-V quote for special chars */
                quote = TRUE;           /* Set flag for next time. */
                break;
            default:                    /* some other char */
                if (isdigit(workch))    /* '0'..'9' */
                    *pp++ = workch;     /* accept them, no tests */
                else {
                    if (workch == ' ')  /* change blanks to underscore */
                        *pp++ = '_';
                    else if (isprint(workch))   /* Other printable, just keep */
                        *pp++ = workch;
                }
            }                   /* switch */
    }                           /* for loop */
    *pp = '\0';                 /* Done with name, terminate it */

/*---------------------------------------------------------------------------
    We COULD check for existing names right now, create a "unique" name, etc.
    However, since other unzips don't do that...we won't bother.  Maybe an-
    other day, ne?  If this went bad, the name'll either be nulled out (in
    which case we'll return non-0) or following procedures won't be able to
    create the extracted file, and other error msgs will result.
  ---------------------------------------------------------------------------*/

    if (*name == '\0') {
        fprintf(stderr, "conversion of [%s] failed\n", filename);
        return (2);
    }
    if (dflag) {
#ifdef VMS
        *xp++ = ']';            /* proper end-of-dir-name delimiter */
        if (xp == cdp) {        /* no path-name stuff, so... */
            strcpy(filename, name);  /* copy file name into global */
            cdp -= 2;           /*   prepare to free malloc'd space */
        } else {                /* we've added path-name stuff... */
            *xp = '\0';         /*   so terminate... */
            dp = cdp;           /*   and convert to VMS subdir separators:  */
            while (*++dp)       /*   (skip first char:  better not be "/") */
                if (*dp == '/') /*   change all slashes */
                    *dp = '.';  /*     to dots */
            cdp -= 2;           /*   include leading bracket and dot */
            strcpy(filename, cdp);   /* copy VMS-style path name into global */
            strcat(filename, name);  /* concatenate file name to global */
        }
#else
        strcpy(filename, cdp);  /* Either "" or slash-terminated path */
        strcat(filename, name); /* append file name to path name */
#endif
        free(cdp);
    } else
        strcpy(filename, name); /* copy converted name into global */

#if FILENAME_MAX < (FILNAMSIZ - 1)
    /*
     * Check the length of the file name and truncate if necessary.
     */
    if (FILENAME_MAX < strlen(filename)) {
        fprintf(stderr, "warning:  filename too long--truncating.\n");
        filename[FILENAME_MAX] = '\0';
        fprintf(stderr, "[ %s ]\n", filename);
        return (1);             /* 1:  warning error */
    }
#endif

    return (0);
}
