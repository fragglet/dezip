/*---------------------------------------------------------------------------

  mapname.c

  This routine changes DEC-20, VAX/VMS, and DOS-style filenames into normal
  Unix names; it also creates any necessary directories, if the -d switch
  was specified.  (We're assuming, of course, that someday somebody will be
  creating files on DECs and VAX/VMS systems--which seems almost likely now,
  thanks to Rich Wales....)

  ---------------------------------------------------------------------------

  Action:  renames argument files as follows:

     strips Unix and PKZIP DOS path name from front (up to rightmost '/') if
       present.
     strips DEC device:, node:: names from front (up to rightmost ':') if
       present.  (This also takes care of any DOS drive: artifacts.)
     strips DEC-20 <directory> or VMS [directory] name if present.
     strips DEC-20 version number from end (everything after 2nd dot) if
       present.
     strips VMS generation number from end (everything after ';') if present.
     honors DEC-20 CTRL-V quote for special characters.
     discards unquoted unprintable characters.

     Returns non-0 if filename zeroed out.

  ---------------------------------------------------------------------------

     Author:  David Kirschbaum, 25 Apr 90
     (Based on good old xxu.c by Frank da Cruz, CUCCA)

  ---------------------------------------------------------------------------

  Version history:

     3.05:  Bill Davidsen did some tweaking (25 Apr 90).
     3.06?  David Kirschbaum fixed digit-gobbling when -m switch enabled
             (27 Apr 90).
     3.11:  James Dugal added support for parent directory creation (17 Aug
             90).
     3.14:  Changed dir mode from 0755 to 0777, per James Dugal and Larry
             Jones (30 Aug 90).
     3.16:  Mark Edwards added mkdir() fix for SysV systems which lack the
             function (19 Sep 90).
     3.90:  Greg Roelofs removed -m stuff; added stat() check for -d option
             (19 Sep 90).

  ---------------------------------------------------------------------------

  Dot notes:

     - Unix allows multiple dots in directory names; MS-DOS and OS/2 FAT
       allow one; VMS does not allow any.  Things are almost as bad with
       regular filenames (VMS allows a single dot but TOPS-20 allows two,
       if you count the one in front of the version number).  As of v3.96,
       mapname leaves the dots alone.  Since the only system currently
       capable of creating zipfiles is DOS or OS/2 FAT (not counting evil
       scum who use binary editors on their zipfiles...not that we know any-
       one like THAT, of course... :) ), the only known problems involve
       recreating a DOS directory-tree (with dots in the directory names)
       under VMS.  Once Rich releases his zipfile-creator program, things
       are going to be a REAL mess...but we'll burn that bridge when we come
       to it.  Since I can't help anticipating, however, here's a suggestion.
       For non-Unix systems, use a two-pass method when parsing the directory+
       filename:  keep a pointer to the first char after the last delimiter;
       when reach next delimiter, go back and convert all dots save the last
       one to underscores (and the last one, too, if VMS).  Then truncate if
       necessary (but most OS's seem to do that automagically).

  ------------------------------------------------------------------------- */


#include "unzip.h"


/******************** */
/*  Mapname Defines */
/******************** */

#ifdef VMS
#define PERMS   0
#else
#define PERMS   0777
#endif

#ifndef NO_MKDIR
#ifdef DOS_OS2                  /* don't wanna use dir.h: conflicts... */
int mkdir(const char *path);    /*  so prototype it here */
#define MKDIR(path,mode)   mkdir(path)
#else
#define MKDIR(path,mode)   mkdir(path,mode)
#endif
#endif





/*************************** */
/*  Function mapped_name() */
/*************************** */

mapped_name()
{
#ifdef NO_MKDIR
    char command[STRSIZ + 40];  /* Buffer for system() call */
#endif
#ifdef VMS
    int stat_val;               /* temp. holder for stat() return value */
#endif
    char name[STRSIZ];          /* file name buffer */
    char *pp, *cp, *xp, *cdp;   /* Character pointers */
    char delim = '\0';          /* Directory Delimiter */
    int dc = 0;                 /* Counters */
    int quote = 0;              /* Flags */
    int indir = 0;
    register int workch;        /* hold the character being tested */



/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ------------------------------------------------------------------------- */

    xp = filename;              /* Copy pointer for simplicity */
#ifdef MAP_DEBUG
    fprintf(stderr, "%s ", xp); /* Echo name of this file */
#endif
    pp = name;                  /* Point to translation buffer */
    *name = '\0';               /* Initialize buffer */

    if (dflag) {                /* -d => retain directory structure */
        cdp = (char *) malloc(strlen(filename) + 1);    /* place for containing */
        if (cdp == NULL) {      /*   directory name */
            fprintf(stderr, "Malloc failed in conversion of [%s]\n", filename);
            return (0);
        }
        *cdp = '\0';
    }
    dc = 0;                     /* Filename dot counter */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ------------------------------------------------------------------------- */

    for (cp = xp; (workch = *cp++) != 0;) {     /* "!= 0":  for Turbo C */

        if (quote) {            /* If this char quoted... */
            *pp++ = workch;     /*  include it literally. */
            quote = 0;
        } else if (indir) {     /* If in directory name... */
            if (workch == delim)
                indir = 0;      /*  look for end delimiter. */
        } else
            switch (workch) {
            case '<':           /* Discard DEC-20 directory name */
                indir = 1;
                delim = '>';
                break;
            case '[':           /* Discard VMS directory name */
                indir = 1;
                delim = ']';
                break;
            case '/':           /* Discard Unix path name... */
            case '\\':          /*  or MS-DOS path name... */
                /*  UNLESS -d flag was given. */

                /*------------------------------------------------------------
                 Special processing case:  if -d flag was specified on command
                 line, create any necessary directories included in the path-
                 name.  Creation of directories is straightforward on BSD and
                 MS-DOS machines but requires use of the system() command on
                 SysV systems (or any others which don't have mkdir()).  The
                 stat() check is necessary with MSC because it doesn't have an
                 EEXIST errno, and it saves the overhead of multiple system()
                 calls on SysV machines.  Don't know how much overhead is in-
                 volved in a mkdir() call...
                  ---------------------------------------------------------- */

                if (dflag) {
                    *pp = '\0';
                    strcat(cdp, name);
#ifdef VMS
                    strcat(cdp, ".dir");        /* add ext. for stat check */
                    stat_val = stat(cdp, &statbuf);
                    cdp[strlen(cdp) - 4] = '\0'; /* remove ext. for all else */
                    if (stat_val) {     /* doesn't exist, so create */
#else
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
                            return (0);
                        }
                    } else if (!(statbuf.st_mode & S_IFDIR)) {
                        fprintf(stderr, "%s:  exists but is not a directory\n",
                                cdp);
                        free(cdp);
                        fprintf(stderr, "unable to process [%s]\n", filename);
                        return (0);
                    }
                    strcat(cdp, "/");
                }               /***** FALL THROUGH to ':' case  **** */
            case ':':           /* Discard DEC dev: or node:: name */
                pp = name;
                break;
            case '.':                   /* DEC -20 generation number
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
                /* Discard everything starting with */
                break;                  /*  semicolon */
            case '\026':                /* Control-V quote for special chars */
                quote = 1;              /* Set flag for next time. */
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
  -------------------------------------------------------------------------- */

    if (*name == '\0') {
        fprintf(stderr, "conversion of [%s] failed\n", filename);
        return (0);
    }
    if (dflag) {
        strcpy(filename, cdp);  /* Either "" or slash-terminated path */
        free(cdp);
        strcat(filename, name);
    } else
        strcpy(filename, name); /* copy converted name into global */
    return (1);
}
