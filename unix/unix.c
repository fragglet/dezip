/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/

/* Unix-specific routines for use with Info-ZIP's UnZip 5.41 and later. */

#include "unzip.h"

#define DIRENT

#include <dirent.h>

typedef struct uxdirattr {  /* struct for holding unix style directory */
    struct uxdirattr *next; /*  info until can be sorted and set at end */
    char *fn;               /* filename of directory */
    union {
        iztimes t3;        /* mtime, atime, ctime */
        struct utimbuf t2; /* modtime, actime */
    } u;
    unsigned perms;  /* same as min_info.file_attr */
    int have_uidgid; /* flag */
    uint32_t uidgid[2];
    char fnbuf[1]; /* buffer stub for directory name */
} uxdirattr;
#define UxAtt(d) ((uxdirattr *) d) /* typecast shortcut */

/*****************************/
/* Strings used multiple     */
/* times in unix.c           */
/*****************************/

/* messages of code for setting file/directory attributes */
static const char CannotSetItemUidGid[] =
    "warning:  cannot set UID %u and/or GID %u for %s\n          %s\n";
static const char CannotSetUidGid[] =
    " (warning) cannot set UID %u and/or GID %u\n          %s";
static const char CannotSetItemTimestamps[] =
    "warning:  cannot set modif./access times for %s\n          %s\n";
static const char CannotSetTimestamps[] =
    " (warning) cannot set modif./access times\n          %s";

char *do_wild(wildspec) const
    char *wildspec; /* only used first time on a given dir */
{
    struct dirent *file;

    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (!G.notfirstcall) { /* first call:  must initialize everything */
        G.notfirstcall = TRUE;

        if (!iswild(wildspec)) {
            strncpy(G.matchname, wildspec, FILNAMSIZ);
            G.matchname[FILNAMSIZ - 1] = '\0';
            G.have_dirname = FALSE;
            G.wild_dir = NULL;
            return G.matchname;
        }

        /* break the wildspec into a directory part and a wildcard filename */
        if ((G.wildname = (const char *) strrchr(wildspec, '/')) == NULL) {
            G.dirname = ".";
            G.dirnamelen = 1;
            G.have_dirname = FALSE;
            G.wildname = wildspec;
        } else {
            ++G.wildname; /* point at character after '/' */
            G.dirnamelen = G.wildname - wildspec;
            G.dirname = checked_malloc(G.dirnamelen + 1);
            strncpy(G.dirname, wildspec, G.dirnamelen);
            G.dirname[G.dirnamelen] = '\0'; /* terminate for strcpy below */
            G.have_dirname = TRUE;
        }

        if ((G.wild_dir = (void *) opendir(G.dirname)) != NULL) {
            while ((file = readdir((DIR *) G.wild_dir)) != NULL) {
                Trace((stderr, "do_wild:  readdir returns %s\n",
                       FnFilter1(file->d_name)));
                if (file->d_name[0] == '.' && G.wildname[0] != '.')
                    continue; /* Unix:  '*' and '?' do not match leading dot */
                if (match(file->d_name, G.wildname, 0) && /*0=case sens.*/
                    /* skip "." and ".." directory entries */
                    strcmp(file->d_name, ".") && strcmp(file->d_name, "..")) {
                    Trace((stderr, "do_wild:  match() succeeds\n"));
                    if (G.have_dirname) {
                        strcpy(G.matchname, G.dirname);
                        strcpy(G.matchname + G.dirnamelen, file->d_name);
                    } else
                        strcpy(G.matchname, file->d_name);
                    return G.matchname;
                }
            }
            /* if we get to here directory is exhausted, so close it */
            closedir((DIR *) G.wild_dir);
            G.wild_dir = NULL;
        }
        Trace((stderr, "do_wild:  opendir(%s) returns NULL\n",
               FnFilter1(G.dirname)));

        /* return the raw wildspec in case that works (e.g., directory not
         * searchable, but filespec was not wild and file is readable) */
        strncpy(G.matchname, wildspec, FILNAMSIZ);
        G.matchname[FILNAMSIZ - 1] = '\0';
        return G.matchname;
    }

    /* last time through, might have failed opendir but returned raw wildspec */
    if (G.wild_dir == NULL) {
        G.notfirstcall = FALSE; /* nothing left--reset for new wildspec */
        if (G.have_dirname)
            free(G.dirname);
        return NULL;
    }

    /* If we've gotten this far, we've read and matched at least one entry
     * successfully (in a previous call), so dirname has been copied into
     * matchname already.
     */
    while ((file = readdir((DIR *) G.wild_dir)) != NULL) {
        Trace((stderr, "do_wild:  readdir returns %s\n",
               FnFilter1(file->d_name)));
        if (file->d_name[0] == '.' && G.wildname[0] != '.')
            continue; /* Unix:  '*' and '?' do not match leading dot */
        if (match(file->d_name, G.wildname, 0)) { /* 0 == case sens. */
            Trace((stderr, "do_wild:  match() succeeds\n"));
            if (G.have_dirname) {
                /* strcpy(G.matchname, G.dirname); */
                strcpy(G.matchname + G.dirnamelen, file->d_name);
            } else
                strcpy(G.matchname, file->d_name);
            return G.matchname;
        }
    }

    closedir((DIR *) G.wild_dir); /* at least one entry read; nothing left */
    G.wild_dir = NULL;
    G.notfirstcall = FALSE; /* reset for new wildspec */
    if (G.have_dirname)
        free(G.dirname);
    return NULL;
}

#ifndef S_ISVTX
#define S_ISVTX 0001000 /* save swapped text even after use */
#endif

/* This is used to clear or keep the SUID and SGID bits on file permissions.
 * It's possible that a file in an archive could have one of these bits set
 * and, unknown to the person unzipping, could allow others to execute the
 * file as the user or group.  The new option -K bypasses this check.
 */

static unsigned filtattr(perms)
unsigned perms;
{
    /* keep setuid/setgid/tacky perms? */
    if (!G.UzO.K_flag)
        perms &= ~(S_ISUID | S_ISGID | S_ISVTX);

    return (0xffff & perms);
}

int mapattr()
{
    int r;
    uint32_t tmp = G.crec.external_file_attributes;

    G.pInfo->file_attr = 0;
    /* initialized to 0 for check in "default" branch below... */

    switch (G.pInfo->hostnum) {
    case AMIGA_:
        tmp = (unsigned) (tmp >> 17 & 7); /* Amiga RWE bits */
        G.pInfo->file_attr = (unsigned) (tmp << 6 | tmp << 3 | tmp);
        break;
    case THEOS_:
        tmp &= 0xF1FFFFFFL;
        if ((tmp & 0xF0000000L) != 0x40000000L)
            tmp &= 0x01FFFFFFL; /* not a dir, mask all ftype bits */
        else
            tmp &= 0x41FFFFFFL; /* leave directory bit as set */
        /* fall through! */
    case UNIX_:
    case VMS_:
    case ACORN_:
    case ATARI_:
    case ATHEOS_:
    case BEOS_:
    case QDOS_:
    case TANDEM_:
        G.pInfo->file_attr = (unsigned) (tmp >> 16);
        /* Check if the file is a (POSIX-compatible) symbolic link.
         * We restrict symlink support to those "made-by" hosts that
         * are known to support symbolic links.
         */
        G.pInfo->symlink =
            S_ISLNK(G.pInfo->file_attr) && SYMLINK_HOST(G.pInfo->hostnum);
        return 0;

    /* all remaining cases:  expand MSDOS read-only bit into write perms */
    case FS_FAT_:
        /* PKWARE's PKZip for Unix marks entries as FS_FAT_, but stores the
         * Unix attributes in the upper 16 bits of the external attributes
         * field, just like Info-ZIP's Zip for Unix.  We try to use that
         * value, after a check for consistency with the MSDOS attribute
         * bits (see below).
         */
        G.pInfo->file_attr = (unsigned) (tmp >> 16);
        /* fall through! */

    default:
        /* Ensure that DOS subdir bit is set when the entry's name ends
         * in a '/'.  Some third-party Zip programs fail to set the subdir
         * bit for directory entries.
         */
        if ((tmp & 0x10) == 0) {
            size_t fnlen = strlen(G.filename);
            if (fnlen > 0 && G.filename[fnlen - 1] == '/')
                tmp |= 0x10;
        }
        /* read-only bit --> write perms; subdir bit --> dir exec bit */
        tmp = !(tmp & 1) << 1 | (tmp & 0x10) >> 4;
        if ((G.pInfo->file_attr & 0700) == (unsigned) (0400 | tmp << 6)) {
            /* keep previous G.pInfo->file_attr setting, when its "owner"
             * part appears to be consistent with DOS attribute flags!
             */
            /* Entries "made by FS_FAT_" could have been zipped on a
             * system that supports POSIX-style symbolic links.
             */
            G.pInfo->symlink =
                S_ISLNK(G.pInfo->file_attr) && (G.pInfo->hostnum == FS_FAT_);
            return 0;
        }
        G.pInfo->file_attr = (unsigned) (0444 | tmp << 6 | tmp << 3 | tmp);
        break;
    } /* end switch (host-OS-created-by) */

    /* for originating systems with no concept of "group," "other," "system": */
    umask((int) (tmp = umask(0))); /* apply mask to expanded r/w(/x) perms */
    G.pInfo->file_attr &= ~tmp;

    return 0;
}

int mapname(renamed)
int renamed;
/*
 * returns:
 *  MPN_OK          - no problem detected
 *  MPN_INF_TRUNC   - caution (truncated filename)
 *  MPN_INF_SKIP    - info "skip entry" (dir doesn't exist)
 *  MPN_ERR_SKIP    - error -> skip entry
 *  MPN_ERR_TOOLONG - error -> path is too long
 *  [also MPN_VOL_LABEL, MPN_CREATED_DIR]
 */
{
    char pathcomp[FILNAMSIZ]; /* path-component buffer */
    char *pp, *cp = NULL;     /* character pointers */
    char *lastsemi = NULL;    /* pointer to last semi-colon in pathcomp */
    int killed_ddot = FALSE;  /* is set when skipping "../" pathcomp */
    int error = MPN_OK;
    register unsigned workch; /* hold the character being tested */

    /*---------------------------------------------------------------------------
        Initialize various pointers and counters and stuff.
      ---------------------------------------------------------------------------*/

    if (G.pInfo->vollabel)
        return MPN_VOL_LABEL; /* can't set disk volume labels in Unix */

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!G.UzO.fflag || renamed);

    G.created_dir = FALSE; /* not yet */

    /* user gave full pathname:  don't prepend rootpath */
    G.renamed_fullpath = (renamed && (*G.filename == '/'));

    checkdir_init(NULL);

    *pathcomp = '\0'; /* initialize translation buffer */
    pp = pathcomp;    /* point to translation buffer */
    if (G.UzO.jflag)  /* junking directories */
        cp = (char *) strrchr(G.filename, '/');
    if (cp == NULL)      /* no '/' or not junking dirs */
        cp = G.filename; /* point to internal zipfile-member pathname */
    else
        ++cp; /* point to start of last component of path */

    /*---------------------------------------------------------------------------
        Begin main loop through characters in filename.
      ---------------------------------------------------------------------------*/

    while ((workch = (uint8_t) *cp++) != 0) {

        switch (workch) {
        case '/': /* can assume -j flag not given */
            *pp = '\0';
            if (strcmp(pathcomp, ".") == 0) {
                /* don't bother appending "./" to the path */
                *pathcomp = '\0';
            } else if (!G.UzO.ddotflag && strcmp(pathcomp, "..") == 0) {
                /* "../" dir traversal detected, skip over it */
                *pathcomp = '\0';
                killed_ddot = TRUE; /* set "show message" flag */
            }
            /* when path component is not empty, append it now */
            if (*pathcomp != '\0' && ((error = checkdir_append_dir(pathcomp)) &
                                      MPN_MASK) > MPN_INF_TRUNC)
                return error;
            pp = pathcomp;   /* reset conversion buffer for next piece */
            lastsemi = NULL; /* leave direct. semi-colons alone */
            break;

        case ';': /* VMS version (or DEC-20 attrib?) */
            lastsemi = pp;
            *pp++ = ';'; /* keep for now; remove VMS ";##" */
            break;       /*  later, if requested */

        default:
            /* disable control character filter when requested,
             * else allow 8-bit characters (e.g. UTF-8) in filenames:
             */
            if (G.UzO.cflxflag ||
                (isprint(workch) || (128 <= workch && workch <= 254)))
                *pp++ = (char) workch;
        } /* end switch */

    } /* end while loop */

    /* Show warning when stripping insecure "parent dir" path components */
    if (killed_ddot && QCOND2) {
        printf("warning:  skipped \"../\" path component(s) in %s\n",
               FnFilter1(G.filename));
        if (!(error & ~MPN_MASK))
            error = (error & MPN_MASK) | PK_WARN;
    }

    /*---------------------------------------------------------------------------
        Report if directory was created (and no file to create:  filename ended
        in '/'), check name to be sure it exists, and combine path and name be-
        fore exiting.
      ---------------------------------------------------------------------------*/

    if (G.filename[strlen(G.filename) - 1] == '/') {
        checkdir_get_path(G.filename);
        if (!G.created_dir) {
            /* dir existed already; don't look for data to extract */
            return (error & ~MPN_MASK) | MPN_INF_SKIP;
        }
        if (QCOND2) {
            printf("   creating: %s\n", FnFilter1(G.filename));
        }
        /* Filter out security-relevant attributes bits. */
        G.pInfo->file_attr = filtattr(G.pInfo->file_attr);
        /* When extracting non-UNIX directories or when extracting
         * without UID/GID restoration or SGID preservation, any
         * SGID flag inherited from the parent directory should be
         * maintained to allow files extracted into this new folder
         * to inherit the GID setting from the parent directory.
         */
        if (G.pInfo->hostnum != UNIX_ || !(G.UzO.X_flag || G.UzO.K_flag)) {
            /* preserve SGID bit when inherited from parent dir */
            if (!stat(G.filename, &G.statbuf)) {
                G.pInfo->file_attr |= G.statbuf.st_mode & S_ISGID;
            } else {
                perror("Could not read directory attributes");
            }
        }

        /* set approx. dir perms (make sure can still read/write in dir) */
        if (chmod(G.filename, G.pInfo->file_attr | 0700))
            perror("chmod (directory attributes) error");
        /* set dir time (note trailing '/') */
        return (error & ~MPN_MASK) | MPN_CREATED_DIR;
    }

    *pp = '\0'; /* done with pathcomp:  terminate it */

    /* On UNIX (and compatible systems), "." and ".." are reserved for
     * directory navigation and cannot be used as regular file names.
     * These reserved one-dot and two-dot names are mapped to "_" and "__".
     */
    if (strcmp(pathcomp, ".") == 0)
        *pathcomp = '_';
    else if (strcmp(pathcomp, "..") == 0)
        strcpy(pathcomp, "__");

    if (*pathcomp == '\0') {
        Info(slide, 1,
             ((char *) slide, "mapname:  conversion of %s failed\n",
              FnFilter1(G.filename)));
        return (error & ~MPN_MASK) | MPN_ERR_SKIP;
    }

    checkdir_append_name(pathcomp); /* returns 1 if truncated: care? */
    checkdir_get_path(G.filename);

    return error;
}

/*
  extract-to dir:      a:path/
  buildpath:           path1/path2/ ...   (NULL-terminated)
  pathcomp:                filename

  mapname():
    loop over chars in zipfile member name
      checkdir(path component, COMPONENT | CREATEDIR) --> map as required?
        (d:/tmp/unzip/)                    (disk:[tmp.unzip.)
        (d:/tmp/unzip/jj/)                 (disk:[tmp.unzip.jj.)
        (d:/tmp/unzip/jj/temp/)            (disk:[tmp.unzip.jj.temp.)
    finally add filename itself and check for existence? (could use with rename)
        (d:/tmp/unzip/jj/temp/msg.outdir)  (disk:[tmp.unzip.jj.temp]msg.outdir)
    checkdir(name, GETPATH)     -->  copy path to name and free space
*/

/*
 * returns:
 *  MPN_OK          - no problem detected
 *  MPN_INF_TRUNC   - (on APPEND_NAME) truncated filename
 *  MPN_INF_SKIP    - path doesn't exist, not allowed to create
 *  MPN_ERR_SKIP    - path doesn't exist, tried to create and failed; or path
 *                    exists and is not a directory, but is supposed to be
 *  MPN_ERR_TOOLONG - path is too long
 */

/* Append the path component to the path being built and check for its
 * existence.  If doesn't exist and we are creating directories, do so
 * for this one; else signal success or error as appropriate.
 */
int checkdir_append_dir(char *pathcomp)
{
    int too_long = FALSE;

    Trace((stderr, "appending dir segment [%s]\n", FnFilter1(pathcomp)));
    while ((*G.end = *pathcomp++) != '\0')
        ++G.end;

    /* GRR:  could do better check, see if overrunning buffer as we go:
     * check end-buildpath after each append, set warning variable if
     * within 20 of FILNAMSIZ; then if var set, do careful check when
     * appending.  Clear variable when begin new path. */

    /* next check: need to append '/', at least one-char name, '\0' */
    if ((G.end - G.buildpath) > FILNAMSIZ - 3)
        too_long = TRUE;                 /* check if extracting dir? */
    if (stat(G.buildpath, &G.statbuf)) { /* path doesn't exist */
        if (!G.create_dirs) {            /* told not to create (freshening) */
            free(G.buildpath);
            return MPN_INF_SKIP; /* path doesn't exist: nothing to do */
        }
        if (too_long) {
            Info(slide, 1,
                 ((char *) slide, "checkdir error:  path too long: %s\n",
                  FnFilter1(G.buildpath)));
            free(G.buildpath);
            /* no room for filenames:  fatal */
            return MPN_ERR_TOOLONG;
        }
        if (mkdir(G.buildpath, 0777) == -1) { /* create the directory */
            Info(slide, 1,
                 ((char *) slide, "checkdir error:  cannot create %s\n\
                 %s\n\
                 unable to process %s.\n",
                  FnFilter2(G.buildpath), strerror(errno),
                  FnFilter1(G.filename)));
            free(G.buildpath);
            /* path didn't exist, tried to create, failed */
            return MPN_ERR_SKIP;
        }
        G.created_dir = TRUE;
    } else if (!S_ISDIR(G.statbuf.st_mode)) {
        Info(slide, 1,
             ((char *) slide,
              "checkdir error:  %s exists but is not directory\n\
                 unable to process %s.\n",
              FnFilter2(G.buildpath), FnFilter1(G.filename)));
        free(G.buildpath);
        /* path existed but wasn't dir */
        return MPN_ERR_SKIP;
    }
    if (too_long) {
        Info(slide, 1,
             ((char *) slide, "checkdir error:  path too long: %s\n",
              FnFilter1(G.buildpath)));
        free(G.buildpath);
        /* no room for filenames:  fatal */
        return MPN_ERR_TOOLONG;
    }
    *G.end++ = '/';
    *G.end = '\0';
    Trace((stderr, "buildpath now = [%s]\n", FnFilter1(G.buildpath)));
    return MPN_OK;
}

/* Copy full path to the string pointed at by pathcomp, and free
 * G.buildpath. */
void checkdir_get_path(char *pathcomp)
{
    strcpy(pathcomp, G.buildpath);
    Trace((stderr, "getting and freeing path [%s]\n", FnFilter1(pathcomp)));
    free(G.buildpath);
    G.buildpath = G.end = NULL;
}

/* Assume the path component is the filename; append it and return
 * without checking for existence. */
int checkdir_append_name(char *pathcomp)
{
    Trace((stderr, "appending filename [%s]\n", FnFilter1(pathcomp)));
    while ((*G.end = *pathcomp++) != '\0') {
        ++G.end;
        if ((G.end - G.buildpath) >= FILNAMSIZ) {
            *--G.end = '\0';
            Info(slide, 1,
                 ((char *) slide,
                  "checkdir warning:  path too long; truncating\n\
                   %s\n                -> %s\n",
                  FnFilter1(G.filename), FnFilter2(G.buildpath)));
            return MPN_INF_TRUNC; /* filename truncated */
        }
    }
    Trace((stderr, "buildpath now = [%s]\n", FnFilter1(G.buildpath)));
    /* could check for existence here, prompt for new name... */
    return MPN_OK;
}

/* Allocate and initialize buffer space for the file currently being
 * extracted.  If file was renamed with an absolute path, don't prepend
 * the extract-to path. */
void checkdir_init(char *pathcomp)
{
    Trace((stderr, "initializing buildpath to "));
    G.buildpath = checked_malloc(strlen(G.filename) + G.rootlen + 1);
    if (G.rootlen > 0 && !G.renamed_fullpath) {
        strcpy(G.buildpath, G.rootpath);
        G.end = G.buildpath + G.rootlen;
    } else {
        *G.buildpath = '\0';
        G.end = G.buildpath;
    }
    Trace((stderr, "[%s]\n", FnFilter1(G.buildpath)));
}

/* If appropriate, store the path in rootpath and create it if
 * necessary; else assume it's a zipfile member and return.  This path
 * segment gets used in extracting all members from every zipfile
 * specified on the command line. */
int checkdir_root(char *pathcomp)
{
    char *tmproot;

    Trace((stderr, "initializing root path to [%s]\n", FnFilter1(pathcomp)));
    if (pathcomp == NULL) {
        G.rootlen = 0;
        return MPN_OK;
    }
    if (G.rootlen > 0) /* rootpath was already set, nothing to do */
        return MPN_OK;
    if ((G.rootlen = strlen(pathcomp)) <= 0) {
        return MPN_OK;
    }
    tmproot = checked_malloc(G.rootlen + 2);
    strcpy(tmproot, pathcomp);
    if (tmproot[G.rootlen - 1] == '/') {
        tmproot[--G.rootlen] = '\0';
    }
    if (G.rootlen > 0 &&
        (stat(tmproot, &G.statbuf) ||
         !S_ISDIR(G.statbuf.st_mode))) { /* path does not exist */
        if (!G.create_dirs /* || iswild(tmproot) */) {
            free(tmproot);
            G.rootlen = 0;
            /* skip (or treat as stored file) */
            return MPN_INF_SKIP;
        }
        /* create the directory (could add loop here scanning tmproot
         * to create more than one level, but why really necessary?) */
        if (mkdir(tmproot, 0777) == -1) {
            Info(slide, 1,
                 ((char *) slide,
                  "checkdir:  cannot create extraction directory: %s\n\
           %s\n",
                  FnFilter1(tmproot), strerror(errno)));
            free(tmproot);
            G.rootlen = 0;
            /* path didn't exist, tried to create, and failed: */
            /* file exists, or 2+ subdir levels required */
            return MPN_ERR_SKIP;
        }
    }
    tmproot[G.rootlen++] = '/';
    tmproot[G.rootlen] = '\0';
    G.rootpath = checked_realloc(tmproot, G.rootlen + 1);
    Trace((stderr, "rootpath now = [%s]\n", FnFilter1(G.rootpath)));
    return MPN_OK;
}

void checkdir_end(char *pathcomp)
{
    Trace((stderr, "freeing rootpath\n"));
    if (G.rootlen > 0) {
        free(G.rootpath);
        G.rootlen = 0;
    }
}

static int get_extattribs(pzt, z_uidgid)
iztimes *pzt;
uint32_t z_uidgid[2];
{
    /*---------------------------------------------------------------------------
        Convert from MSDOS-format local time and date to Unix-format 32-bit GMT
        time:  adjust base year from 1980 to 1970, do usual conversions from
        yy/mm/dd hh:mm:ss to elapsed seconds, and account for timezone and day-
        light savings time differences.  If we have a Unix extra field, however,
        we're laughing:  both mtime and atime are ours.  On the other hand, we
        then have to check for restoration of UID/GID.
      ---------------------------------------------------------------------------*/
    int have_uidgid_flg;
    unsigned eb_izux_flg;

    eb_izux_flg =
        G.extra_field
            ? ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                               G.lrec.last_mod_dos_datetime, pzt, z_uidgid)
            : 0;
    if (eb_izux_flg & EB_UT_FL_MTIME) {
        TTrace((stderr, "\nget_extattribs:  Unix e.f. modif. time = %ld\n",
                pzt->mtime));
    } else {
        pzt->mtime = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }
    if (eb_izux_flg & EB_UT_FL_ATIME) {
        TTrace((stderr, "get_extattribs:  Unix e.f. access time = %ld\n",
                pzt->atime));
    } else {
        pzt->atime = pzt->mtime;
        TTrace((stderr, "\nget_extattribs:  modification/access times = %ld\n",
                pzt->mtime));
    }

    /* if -X option was specified and we have UID/GID info, restore it */
    have_uidgid_flg = G.UzO.X_flag && (eb_izux_flg & EB_UX2_VALID);
    return have_uidgid_flg;
}

void close_outfile() /* GRR: change to return PK-style warning level */
{
    union {
        iztimes t3;        /* mtime, atime, ctime */
        struct utimbuf t2; /* modtime, actime */
    } zt;
    uint32_t z_uidgid[2];
    int have_uidgid_flg;

    have_uidgid_flg = get_extattribs(&(zt.t3), z_uidgid);

    /*---------------------------------------------------------------------------
        If symbolic links are supported, allocate storage for a symlink control
        structure, put the uncompressed "data" and other required info in it,
      and add the structure to the "deferred symlinks" chain.  Since we know
      it's a symbolic link to start with, we shouldn't have to worry about
      overflowing unsigned ints with unsigned longs.
      ---------------------------------------------------------------------------*/

    if (G.symlnk) {
        size_t ucsize = (size_t) G.lrec.ucsize;
        size_t attribsize =
            sizeof(unsigned) + (have_uidgid_flg ? sizeof(z_uidgid) : 0);
        /* size of the symlink entry is the sum of
         *  (struct size (includes 1st '\0') + 1 additional trailing '\0'),
         *  system specific attribute data size (might be 0),
         *  and the lengths of name and link target.
         */
        size_t slnk_entrysize =
            (sizeof(slinkentry) + 1) + attribsize + ucsize + strlen(G.filename);
        slinkentry *slnk_entry;

        if (slnk_entrysize < ucsize) {
            Info(slide, 1,
                 ((char *) slide,
                  "warning:  symbolic link (%s) failed: mem alloc overflow\n",
                  FnFilter1(G.filename)));
            fclose(G.outfile);
            return;
        }

        slnk_entry = checked_malloc(slnk_entrysize);
        slnk_entry->next = NULL;
        slnk_entry->targetlen = ucsize;
        slnk_entry->attriblen = attribsize;
        memcpy(slnk_entry->buf, &(G.pInfo->file_attr), sizeof(unsigned));
        if (have_uidgid_flg)
            memcpy(slnk_entry->buf + 4, z_uidgid, sizeof(z_uidgid));
        slnk_entry->target = slnk_entry->buf + slnk_entry->attriblen;
        slnk_entry->fname = slnk_entry->target + ucsize + 1;
        strcpy(slnk_entry->fname, G.filename);

        /* move back to the start of the file to re-read the "link data" */
        rewind(G.outfile);

        if (fread(slnk_entry->target, 1, ucsize, G.outfile) != ucsize) {
            Info(slide, 1,
                 ((char *) slide, "warning:  symbolic link (%s) failed\n",
                  FnFilter1(G.filename)));
            free(slnk_entry);
            fclose(G.outfile);
            return;
        }
        fclose(G.outfile); /* close "link" file for good... */
        slnk_entry->target[ucsize] = '\0';
        if (QCOND2) {
            printf("-> %s ", FnFilter1(slnk_entry->target));
        }
        /* add this symlink record to the list of deferred symlinks */
        if (G.slink_last != NULL)
            G.slink_last->next = slnk_entry;
        else
            G.slink_head = slnk_entry;
        G.slink_last = slnk_entry;
        return;
    }

    /* if -X option was specified and we have UID/GID info, restore it */
    if (have_uidgid_flg
        /* check that both uid and gid values fit into their data sizes */
        && (uint32_t) (uid_t) (z_uidgid[0]) == z_uidgid[0] &&
        (uint32_t) (gid_t) (z_uidgid[1]) == z_uidgid[1]) {
        TTrace((stderr, "close_outfile:  restoring Unix UID/GID info\n"));
        if (fchown(fileno(G.outfile), (uid_t) z_uidgid[0],
                   (gid_t) z_uidgid[1])) {
            if (G.UzO.qflag)
                Info(slide, 1,
                     ((char *) slide, CannotSetItemUidGid, z_uidgid[0],
                      z_uidgid[1], FnFilter1(G.filename), strerror(errno)));
            else
                Info(slide, 1,
                     ((char *) slide, CannotSetUidGid, z_uidgid[0], z_uidgid[1],
                      strerror(errno)));
        }
    }

    /*---------------------------------------------------------------------------
        Change the file permissions from default ones to those stored in the
        zipfile.
      ---------------------------------------------------------------------------*/

    if (fchmod(fileno(G.outfile), filtattr(G.pInfo->file_attr)))
        perror("fchmod (file attributes) error");

    fclose(G.outfile);

    /* skip restoring time stamps on user's request */
    /* set the file's access and modification times */
    if (G.UzO.D_flag <= 1 && utime(G.filename, &(zt.t2))) {
        if (G.UzO.qflag)
            Info(slide, 1,
                 ((char *) slide, CannotSetItemTimestamps,
                  FnFilter1(G.filename), strerror(errno)));
        else
            Info(slide, 1,
                 ((char *) slide, CannotSetTimestamps, strerror(errno)));
    }
}

int set_symlnk_attribs(slnk_entry)
slinkentry *slnk_entry;
{
    if (slnk_entry->attriblen > 0 && slnk_entry->attriblen > sizeof(unsigned)) {
        uint32_t *z_uidgid_p = (void *) (slnk_entry->buf + sizeof(unsigned));
        /* check that both uid and gid values fit into their data sizes */
        if ((uint32_t) (uid_t) (z_uidgid_p[0]) == z_uidgid_p[0] &&
            (uint32_t) (gid_t) (z_uidgid_p[1]) == z_uidgid_p[1]) {
            TTrace((stderr,
                    "set_symlnk_attribs:  restoring Unix UID/GID info for\n\
        %s\n",
                    FnFilter1(slnk_entry->fname)));
            if (lchown(slnk_entry->fname, (uid_t) z_uidgid_p[0],
                       (gid_t) z_uidgid_p[1])) {
                Info(slide, 1,
                     ((char *) slide, CannotSetItemUidGid, z_uidgid_p[0],
                      z_uidgid_p[1], FnFilter1(slnk_entry->fname),
                      strerror(errno)));
            }
        }
    }
    /* currently, no error propagation... */
    return PK_OK;
}

/* messages of code for setting directory attributes */
static const char DirlistChmodFailed[] =
    "warning:  cannot set permissions for %s\n          %s\n";

int defer_dir_attribs(pd)
direntry **pd;
{
    uxdirattr *d_entry;

    d_entry = checked_malloc(sizeof(uxdirattr) + strlen(G.filename));
    *pd = (direntry *) d_entry;
    d_entry->fn = d_entry->fnbuf;
    strcpy(d_entry->fn, G.filename);

    d_entry->perms = G.pInfo->file_attr;

    d_entry->have_uidgid = get_extattribs(&(d_entry->u.t3), d_entry->uidgid);
    return PK_OK;
}

int set_direc_attribs(d)
direntry *d;
{
    int errval = PK_OK;

    if (UxAtt(d)->have_uidgid &&
        /* check that both uid and gid values fit into their data sizes */
        (uint32_t) (uid_t) (UxAtt(d)->uidgid[0]) == UxAtt(d)->uidgid[0] &&
        (uint32_t) (gid_t) (UxAtt(d)->uidgid[1]) == UxAtt(d)->uidgid[1] &&
        chown(UxAtt(d)->fn, (uid_t) UxAtt(d)->uidgid[0],
              (gid_t) UxAtt(d)->uidgid[1])) {
        Info(slide, 1,
             ((char *) slide, CannotSetItemUidGid, UxAtt(d)->uidgid[0],
              UxAtt(d)->uidgid[1], FnFilter1(d->fn), strerror(errno)));
        if (!errval)
            errval = PK_WARN;
    }
    /* Skip restoring directory time stamps on user' request. */
    /* restore directory timestamps */
    if (G.UzO.D_flag <= 0 && utime(d->fn, &UxAtt(d)->u.t2)) {
        Info(slide, 1,
             ((char *) slide, CannotSetItemTimestamps, FnFilter1(d->fn),
              strerror(errno)));
        if (!errval)
            errval = PK_WARN;
    }
    if (chmod(d->fn, UxAtt(d)->perms)) {
        Info(slide, 1,
             ((char *) slide, DirlistChmodFailed, FnFilter1(d->fn),
              strerror(errno)));
        if (!errval)
            errval = PK_WARN;
    }
    return errval;
}

int stamp_file(fname, modtime) const char *fname;
time_t modtime;
{
    struct utimbuf tp;

    tp.modtime = tp.actime = modtime;
    return (utime(fname, &tp));
}
