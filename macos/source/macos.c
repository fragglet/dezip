/*---------------------------------------------------------------------------

  mac.c

  Macintosh-specific routines for use with Info-ZIP's UnZip 5.4 and later.

  Contains:
                    do_wild ()
                    mapattr ()
                    checkdir ()
                    version ()
                    macmkdir ()
                    macopen ()
                    maccreat ()
                    macread ()
                    macwrite ()
                    macclose ()
                    maclseek ()
                    FindNewExtractFolder ()
                    SetFinderInfo ()
                    isMacOSexfield ()
                    makePPClong ()
                    makePPCword ()
                    PrintMacExtraInfo ()
                    GetExtraFieldData ()
                    DecodeMac3ExtraField ()
                    DecodeJLEEextraField ()
                    PrintTextEncoding ()
                    MacGlobalsInit ()

  ---------------------------------------------------------------------------*/


/*****************************************************************************/
/*  Includes                                                                 */
/*****************************************************************************/

#define UNZIP_INTERNAL
#include "unzip.h"

#include <script.h>
#include "pathname.h"
#include "helpers.h"
#include "macstuff.h"
#include "mactime.h"

/*****************************************************************************/
/*  Macros, typedefs                                                         */
/*****************************************************************************/

#define read_only   file_attr   /* for readability only */

#define MKDIR(path)     macmkdir(path)


/*****************************************************************************/
/*  Global Vars                                                              */
/*****************************************************************************/

/*  Note: sizeof() returns the size of this allusion
          13 is current length of "XtraStuf.mac:"      */
extern const char ResourceMark[13]; /* var is initialized in file pathname.c */

Boolean MacUnzip_Noisy;         /* MacUnzip_Noisy is also used by console */


/*****************************************************************************/
/*  Module level Vars                                                        */
/*****************************************************************************/

static int created_dir;        /* used in mapname(), checkdir() */
static int renamed_fullpath;   /* ditto */
static FSSpec CurrentFile;
static MACINFO newExtraField;  /* contains all extra-field data */

static firstcall_of_macopen = true;
static short MacZipMode;
static Boolean UseUT_ExtraField     = false;
static Boolean IgnoreEF_Macfilename = false;


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/

extern char *GetUnZipInfoVersions(void);

static OSErr SetFinderInfo(FSSpec *spec, MACINFO *mi);
static Boolean GetExtraFieldData(short *MacZipMode, MACINFO *mi);
static uch *scanMacOSexfield(uch *ef_ptr, unsigned ef_len,
                             short *MacZipMode);
static Boolean isMacOSexfield(unsigned id, unsigned size, short *MacZipMode);
static void PrintMacExtraInfo(void);
static void DecodeMac3ExtraField(ZCONST uch *buff, MACINFO *mi);
static void DecodeJLEEextraField(ZCONST uch *buff, MACINFO *mi);
static char *PrintTextEncoding(short script);
static void MakeMacOS_String(char *MacOS_Str,
            const char SpcChar1, const char SpcChar2,
            const char SpcChar3, const char SpcChar4);


/*****************************************************************************/
/*  Constants (strings, etc.)                                                */
/*****************************************************************************/

static ZCONST char Far CannotCreateFile[] = "error:  cannot create %s\n";


/*****************************************************************************/
/*  Functions                                                                */
/*****************************************************************************/

#ifndef SFX

/**********************/
/* Function do_wild() */   /* for porting:  dir separator; match(ignore_case) */
/**********************/

char *do_wild(__G__ wildspec)
    __GDEF
    char *wildspec;         /* only used first time on a given dir */
{
    static DIR *dir = (DIR *)NULL;
    static char *dirname, *wildname, matchname[FILNAMSIZ];
    static int firstcall=TRUE, have_dirname;
    static unsigned long dirnamelen;
    struct dirent *file;

    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (firstcall) {        /* first call:  must initialize everything */
        firstcall = FALSE;

        MacUnzip_Noisy = !uO.qflag;

        if (MacUnzip_Noisy) printf("%s \n\n", GetUnZipInfoVersions());

        /* break the wildspec into a directory part and a wildcard filename */
        if ((wildname = strrchr(wildspec, ':')) == (char *)NULL) {
            dirname = ":";
            dirnamelen = 1;
            have_dirname = FALSE;
            wildname = wildspec;
        } else {
            ++wildname;     /* point at character after ':' */
            dirnamelen = wildname - wildspec;
            if ((dirname = (char *)malloc(dirnamelen+1)) == (char *)NULL) {
                Info(slide, 0x201, ((char *)slide,
                  "warning:  cannot allocate wildcard buffers\n"));
                strcpy(matchname, wildspec);
                return matchname;   /* but maybe filespec was not a wildcard */
            }
            strncpy(dirname, wildspec, dirnamelen);
            dirname[dirnamelen] = '\0';   /* terminate for strcpy below */
            have_dirname = TRUE;
        }

        if ((dir = opendir(dirname)) != (DIR *)NULL) {
            while ((file = readdir(dir)) != (struct dirent *)NULL) {
                if (match(file->d_name, wildname, 0)) {  /* 0 == case sens. */
                    if (have_dirname) {
                        strcpy(matchname, dirname);
                        strcpy(matchname+dirnamelen, file->d_name);
                    } else
                        strcpy(matchname, file->d_name);
                    return matchname;
                }
            }
            /* if we get to here directory is exhausted, so close it */
            closedir(dir);
            dir = (DIR *)NULL;
        }

        /* return the raw wildspec in case that works (e.g., directory not
         * searchable, but filespec was not wild and file is readable) */
        strcpy(matchname, wildspec);
        return matchname;
    }

    /* last time through, might have failed opendir but returned raw wildspec */
    if (dir == (DIR *)NULL) {
        firstcall = TRUE;  /* nothing left to try--reset for new wildspec */
        if (have_dirname)
            free(dirname);
        return (char *)NULL;
    }

    closedir(dir);     /* have read at least one dir entry; nothing left */
    dir = (DIR *)NULL;
    firstcall = TRUE;  /* reset for new wildspec */
    if (have_dirname)
        free(dirname);
    return (char *)NULL;

} /* end function do_wild() */

#endif /* !SFX */





/***************************/
/* Function open_outfile() */
/***************************/

int open_outfile(__G)         /* return 1 if fail */
    __GDEF
{
    short outfd, fDataFork = TRUE;
    OSErr err;
    char CompletePath[NAME_MAX];
    char ArchiveDir[NAME_MAX];
    short CurrentFork;
    unsigned exdirlen;

#ifdef DLL
    if (G.redirect_data)
        return (redirect_outfile(__G) == FALSE);
#endif
    Trace((stderr, "open_outfile:  trying to open (%s) for writing\n",
      FnFilter1(G.filename)));

    exdirlen = strlen(uO.exdir);

    if (MacZipMode != UnKnown_EF)
    {
        fDataFork = (newExtraField.flags & EB_M3_FL_DATFRK) ? TRUE : FALSE;

        if (IgnoreEF_Macfilename)  {
            strcpy(ArchiveDir, &G.filename[exdirlen+1]);
            G.filename[exdirlen+1] = '\0';
            RfDfFilen2Real(ArchiveDir, ArchiveDir, MacZipMode,
                      (newExtraField.flags & EB_M3_FL_DATFRK), &CurrentFork);
            strcat(G.filename, ArchiveDir);
        } else {        /* use the filename from extra-field */
            G.filename[exdirlen+1] = '\0';
            strcat(G.filename,newExtraField.FullPath);
        }
    }
    else
    {
        if (!uO.aflag) {
         /* unknown type documents */
         /* all files are considered to be of type 'TEXT' and creator 'hscd' */
         /* this is the default type for CDROM ISO-9660 without Apple extensions */
            newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdType    =  'TEXT';
            newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdCreator =  'hscd';
        } else {
         /* unknown text-files defaults to 'TEXT' */
            newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdType    =  'TEXT';
         /* Bare Bones BBEdit */
            newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdCreator =  'R*ch';
        }
    }

    GetCompletePath(CompletePath, G.filename, &CurrentFile, &err);
    printerr("GetCompletePath open_outfile", (err != -43) && (err != 0),
             err, __LINE__, __FILE__, CompletePath);

    if ((outfd = maccreat(G.filename)) != -1) {
        outfd = macopen(CompletePath, (fDataFork) ? 1 : 2);
    }

    if (outfd == -1) {
        G.outfile = (FILE *)NULL;
        Info(slide, 0x401, ((char *)slide, LoadFarString(CannotCreateFile),
          FnFilter1(G.filename)));
        return 1;
    }
    G.outfile = (FILE *)outfd;
    Trace((stderr, "open_outfile:  successfully opened (%s) for writing\n",
      FnFilter1(G.filename)));

    return 0;

} /* end function open_outfile() */





/**********************/
/* Function mapattr() */
/**********************/

int mapattr(__G)
    __GDEF
{
    /* only care about read-only bit, so just look at MS-DOS side of attrs */
    G.pInfo->read_only = (unsigned)(G.crec.external_file_attributes & 1);
    return 0;

} /* end function mapattr() */





/************************/
/*  Function mapname()  */
/************************/
                             /* return 0 if no error, 1 if caution (filename */
int mapname(__G__ renamed)   /*  truncated), 2 if warning (skip file because */
    __GDEF                   /*  dir doesn't exist), 3 if error (skip file), */
    int renamed;             /*  or 10 if out of memory (skip file) */
{                            /*  [also IZ_VOL_LABEL, IZ_CREATED_DIR] */
    char pathcomp[FILNAMSIZ];    /* path-component buffer */
    char *pp, *cp=(char *)NULL;  /* character pointers */
    char *lastsemi=(char *)NULL; /* pointer to last semi-colon in pathcomp */
    int quote = FALSE;           /* flags */
    int error = 0;
    register unsigned workch;    /* hold the character being tested */


/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    if (G.pInfo->vollabel)
        return IZ_VOL_LABEL;    /* can't set disk volume labels on Macintosh */

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!uO.fflag || renamed);
    MacZipMode = false;

    created_dir = FALSE;        /* not yet */

    /* user gave full pathname:  don't prepend rootpath */
    renamed_fullpath = (renamed && (*G.filename == '/'));

    if (checkdir(__G__ (char *)NULL, INIT) == 10)
        return 10;              /* initialize path buffer, unless no memory */

    *pathcomp = '\0';           /* initialize translation buffer */
    pp = pathcomp;              /* point to translation buffer */

    if (uO.jflag)               /* junking directories */
        cp = (char *)strrchr(G.filename, '/');
    if (cp == (char *)NULL) {   /* no '/' or not junking dirs */
        cp = G.filename;        /* point to internal zipfile-member pathname */
        if (renamed_fullpath)
            ++cp;               /* skip over leading '/' */
    } else
        ++cp;                   /* point to start of last component of path */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {

        if (quote) {                 /* if character quoted, */
            *pp++ = (char)workch;    /*  include it literally */
            quote = FALSE;
        } else
            switch (workch) {
            case '/':             /* can assume -j flag not given */
                *pp = '\0';
                if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                    return error;
                pp = pathcomp;    /* reset conversion buffer for next piece */
                lastsemi = (char *)NULL;
                                  /* leave directory semi-colons alone */
                break;

            case ';':             /* VMS version (or DEC-20 attrib?) */
                lastsemi = pp;         /* keep for now; remove VMS ";##" */
                *pp++ = (char)workch;  /*  later, if requested */
                break;

            case '\026':          /* control-V quote for special chars */
                quote = TRUE;     /* set flag for next character */
                break;

            default:
                /* allow European characters in filenames: */
                if (isprint(workch) || (128 <= workch && workch <= 254))
                    *pp++ = (char)workch;
            } /* end switch */

    } /* end while loop */

    *pp = '\0';                   /* done with pathcomp:  terminate it */

    /* if not saving them, remove VMS version numbers (appended ";###") */
    if (!uO.V_flag && lastsemi) {
        pp = lastsemi + 1;
        while (isdigit((uch)(*pp)))
            ++pp;
        if (*pp == '\0')          /* only digits between ';' and end:  nuke */
            *lastsemi = '\0';
    }

/*---------------------------------------------------------------------------
    Report if directory was created (and no file to create:  filename ended
    in '/'), check name to be sure it exists, and combine path and name be-
    fore exiting.
  ---------------------------------------------------------------------------*/

    if (G.filename[strlen(G.filename) - 1] == '/') {
        checkdir(__G__ G.filename, GETPATH);
        if (created_dir) {
            if (QCOND2) {
                Info(slide, 0, ((char *)slide, "   creating: %s\n",
                  G.filename));
            }
            return IZ_CREATED_DIR;   /* set dir time (note trailing '/') */
        }
        return 2;   /* dir existed already; don't look for data to extract */
    }

    if (*pathcomp == '\0') {
        Info(slide, 1, ((char *)slide, "mapname:  conversion of %s failed\n",
          G.filename));
        return 3;
    }

    GetExtraFieldData(&MacZipMode, &newExtraField);

    checkdir(__G__ pathcomp, APPEND_NAME);  /* returns 1 if truncated: care? */
    checkdir(__G__ G.filename, GETPATH);

    return error;

} /* end function mapname() */





/***********************/
/* Function checkdir() */
/***********************/

int checkdir(__G__ pathcomp, flag)
    __GDEF
    char *pathcomp;
    int flag;
/*
 * returns:  1 - (on APPEND_NAME) truncated filename
 *           2 - path doesn't exist, not allowed to create
 *           3 - path doesn't exist, tried to create and failed; or
 *               path exists and is not a directory, but is supposed to be
 *           4 - path is too long
 *          10 - can't allocate memory for filename buffers
 */
{
    static int rootlen = 0;   /* length of rootpath */
    static char *rootpath;    /* user's "extract-to" directory */
    static char *buildpath;   /* full path (so far) to extracted file */
    static char *end;         /* pointer to end of buildpath ('\0') */

#   define FN_MASK   7
#   define FUNCTION  (flag & FN_MASK)


/*---------------------------------------------------------------------------
    APPEND_DIR:  append the path component to the path being built and check
    for its existence.  If doesn't exist and we are creating directories, do
    so for this one; else signal success or error as appropriate.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == APPEND_DIR) {
        int too_long = FALSE;
#ifdef SHORT_NAMES
        char *old_end = end;
#endif

        Trace((stderr, "appending dir segment [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0')
            ++end;
#ifdef SHORT_NAMES   /* path components restricted to 14 chars, typically */
        if ((end-old_end) > FILENAME_MAX)  /* GRR:  proper constant? */
            *(end = old_end + FILENAME_MAX) = '\0';
#endif

        /* GRR:  could do better check, see if overrunning buffer as we go:
         * check end-buildpath after each append, set warning variable if
         * within 20 of FILNAMSIZ; then if var set, do careful check when
         * appending.  Clear variable when begin new path. */

        if ((end-buildpath) > FILNAMSIZ-3)  /* need ':', one-char name, '\0' */
            too_long = TRUE;                /* check if extracting directory? */
        if (stat(buildpath, &G.statbuf)) {  /* path doesn't exist */
            if (!G.create_dirs) { /* told not to create (freshening) */
                free(buildpath);
                return 2;         /* path doesn't exist:  nothing to do */
            }
            if (too_long) {
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  path too long: %s\n", buildpath));
                free(buildpath);
                return 4;         /* no room for filenames:  fatal */
            }
            if (MKDIR(buildpath) == -1) {   /* create the directory */
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  cannot create %s\n\
                 unable to process %s.\n", buildpath, G.filename));
                free(buildpath);
                return 3;      /* path didn't exist, tried to create, failed */
            }
            created_dir = TRUE;
        } else if (!S_ISDIR(G.statbuf.st_mode)) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  %s exists but is not directory\n\
                 unable to process %s.\n", buildpath, G.filename));
            free(buildpath);
            return 3;          /* path existed but wasn't dir */
        }
        if (too_long) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  path too long: %s\n", buildpath));
            free(buildpath);
            return 4;         /* no room for filenames:  fatal */
        }
        *end++ = ':';
        *end = '\0';
        Trace((stderr, "buildpath now = [%s]\n", buildpath));
        return 0;

    } /* end if (FUNCTION == APPEND_DIR) */

/*---------------------------------------------------------------------------
    GETPATH:  copy full path to the string pointed at by pathcomp, and free
    buildpath.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == GETPATH) {
        strcpy(pathcomp, buildpath);
        Trace((stderr, "getting and freeing path [%s]\n", pathcomp));
        free(buildpath);
        buildpath = end = (char *)NULL;
        return 0;
    }

/*---------------------------------------------------------------------------
    APPEND_NAME:  assume the path component is the filename; append it and
    return without checking for existence.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == APPEND_NAME) {
#ifdef SHORT_NAMES
        char *old_end = end;
#endif

        Trace((stderr, "appending filename [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0') {
            ++end;
#ifdef SHORT_NAMES  /* truncate name at 14 characters, typically */
            if ((end-old_end) > FILENAME_MAX)      /* GRR:  proper constant? */
                *(end = old_end + FILENAME_MAX) = '\0';
#endif
            if ((end-buildpath) >= FILNAMSIZ) {
                *--end = '\0';
                Info(slide, 0x201, ((char *)slide,
                  "checkdir warning:  path too long; truncating\n\
                   %s\n                -> %s\n", G.filename, buildpath));
                return 1;   /* filename truncated */
            }
        }
        Trace((stderr, "buildpath now = [%s]\n", buildpath));
        return 0;  /* could check for existence here, prompt for new name... */
    }

/*---------------------------------------------------------------------------
    INIT:  allocate and initialize buffer space for the file currently being
    extracted.  If file was renamed with an absolute path, don't prepend the
    extract-to path.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == INIT) {
        Trace((stderr, "initializing buildpath to "));
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+2)) ==
            (char *)NULL)
            return 10;
        if ((rootlen > 0) && !renamed_fullpath) {
            strcpy(buildpath, rootpath);
            end = buildpath + rootlen;
        } else {
            end = buildpath;
            if (!renamed_fullpath && !uO.jflag) {
                *end++ = ':';           /* indicate relative path */
            }
            *end = '\0';
        }
        Trace((stderr, "[%s]\n", buildpath));
        return 0;
    }

/*---------------------------------------------------------------------------
    ROOT:  if appropriate, store the path in rootpath and create it if neces-
    sary; else assume it's a zipfile member and return.  This path segment
    gets used in extracting all members from every zipfile specified on the
    command line.
  ---------------------------------------------------------------------------*/

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (FUNCTION == ROOT) {
        Trace((stderr, "initializing root path to [%s]\n", pathcomp));
        if (pathcomp == (char *)NULL) {
            rootlen = 0;
            return 0;
        }
        if ((rootlen = strlen(pathcomp)) > 0) {
            if (pathcomp[rootlen-1] == ':') {
                pathcomp[--rootlen] = '\0';     /* strip trailing delimiter */
            }
            if (rootlen > 0 && (stat(pathcomp, &G.statbuf) ||
                !S_ISDIR(G.statbuf.st_mode)))       /* path does not exist */
            {
                if (!G.create_dirs /* || iswild(pathcomp) */ ) {
                    rootlen = 0;
                    return 2;   /* skip (or treat as stored file) */
                }
                /* create the directory (could add loop here to scan pathcomp
                 * and create more than one level, but why really necessary?) */
                if (MKDIR(pathcomp) == -1) {
                    Info(slide, 1, ((char *)slide,
                      "checkdir:  cannot create extraction directory: %s\n",
                      pathcomp));
                    rootlen = 0;   /* path didn't exist, tried to create, and */
                    return 3;  /* failed:  file exists, or 2+ levels required */
                }
            }
            if ((rootpath = (char *)malloc(rootlen+2)) == (char *)NULL) {
                rootlen = 0;
                return 10;
            }
            strcpy(rootpath, pathcomp);
            rootpath[rootlen++] = ':';
            rootpath[rootlen] = '\0';
            Trace((stderr, "rootpath now = [%s]\n", rootpath));
        }
        return 0;
    }
#endif /* !SFX || SFX_EXDIR */

/*---------------------------------------------------------------------------
    END:  free rootpath, immediately prior to program exit.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == END) {
        Trace((stderr, "freeing rootpath\n"));
        if (rootlen > 0) {
            free(rootpath);
            rootlen = 0;
        }
        return 0;
    }

    return 99;  /* should never reach */

} /* end function checkdir() */





/****************************/
/* Function close_outfile() */
/****************************/

void close_outfile(__G)    /* GRR: change to return PK-style warning level */
    __GDEF
{
#ifdef USE_EF_UT_TIME
    iztimes z_utime;
    unsigned eb_izux_flg;
#endif
    HParamBlockRec hpbr;
    OSErr err;
    FSSpec spec;
    char CompletePath[NAME_MAX];
    CInfoPBRec      fpb;

    if (fileno(G.outfile) == 1)
        return;         /* don't attempt to close or set time on stdout */

    err = (OSErr)fclose(G.outfile);
    printerr("macclose FSClose ",err, err, __LINE__, __FILE__, G.filename);

    GetCompletePath(CompletePath, G.filename, &spec, &err);
    printerr("GetCompletePath", err, err, __LINE__, __FILE__, G.filename);
    fpb.hFileInfo.ioNamePtr = spec.name;
    fpb.hFileInfo.ioVRefNum = spec.vRefNum;
    fpb.hFileInfo.ioDirID = spec.parID;
    fpb.hFileInfo.ioFDirIndex = 0;

    hpbr.fileParam.ioNamePtr = spec.name;
    hpbr.fileParam.ioVRefNum = spec.vRefNum;
    hpbr.fileParam.ioDirID = spec.parID;
    hpbr.fileParam.ioFDirIndex = 0;

    err = PBGetCatInfoSync((CInfoPBPtr)&fpb);
    printerr("PBGetCatInfoSync", err, err, __LINE__, __FILE__, G.filename);

    fpb.hFileInfo.ioDirID = spec.parID;
    if ((MacZipMode == UnKnown_EF) || UseUT_ExtraField ) {

#ifdef USE_EF_UT_TIME
        eb_izux_flg = ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                                       G.lrec.last_mod_dos_datetime, &z_utime, NULL);
        if (G.extra_field &&
#ifdef IZ_CHECK_TZ
            G.tz_is_valid &&
#endif
            (eb_izux_flg & EB_UT_FL_MTIME))
            {
            fpb.hFileInfo.ioFlMdDat = UnixFtime2MacFtime(z_utime.mtime);
            fpb.hFileInfo.ioFlCrDat = UnixFtime2MacFtime(z_utime.ctime);
            }

#ifdef DEBUG_TIME
            {
            struct tm *tp = gmtime(&z_utime.ctime);
            printf(
              "close_outfile:  Unix e.f. creat. time = %d/%2d/%2d  %2d:%2d:%2d -> %lu UTC\n",
              tp->tm_year, tp->tm_mon+1, tp->tm_mday,
              tp->tm_hour, tp->tm_min, tp->tm_sec, z_utime.ctime);
            tp = gmtime(&z_utime.mtime);
            printf(
              "close_outfile:  Unix e.f. modif. time = %d/%2d/%2d  %2d:%2d:%2d -> %lu UTC\n",
              tp->tm_year, tp->tm_mon+1, tp->tm_mday,
              tp->tm_hour, tp->tm_min, tp->tm_sec, z_utime.mtime);
            }
#endif /* DEBUG_TIME */


#else /* !USE_EF_UT_TIME */
        TTrace((stderr, "close_outfile:  using DOS-Datetime ! \n",
              z_utime.mtime));
        dtr.year = (((G.lrec.last_mod_dos_datetime >> 25) & 0x7f) + 1980);
        dtr.month = ((G.lrec.last_mod_dos_datetime >> 21) & 0x0f);
        dtr.day = ((G.lrec.last_mod_dos_datetime >> 16) & 0x1f);

        dtr.hour = ((G.lrec.last_mod_dos_datetime >> 11) & 0x1f);
        dtr.minute = ((G.lrec.last_mod_dos_datetime >> 5) & 0x3f);
        dtr.second = ((G.lrec.last_mod_dos_datetime << 1) & 0x3e);

        DateToSeconds(&dtr, (unsigned long *)&fpb.hFileInfo.ioFlMdDat);
        fpb.hFileInfo.ioFlCrDat = fpb.hFileInfo.ioFlMdDat;
#endif /* ?USE_EF_UT_TIME */

        if (err == noErr)
            err = PBSetCatInfoSync((CInfoPBPtr)&fpb);
        if (err != noErr)
            printf("error (%d): cannot set the time for %s\n", err, G.filename);
    }

    /* set read-only perms if needed */
    if ((err == noErr) && G.pInfo->read_only) {
        err = PBHSetFLockSync(&hpbr);
        printerr("PBHSetFLockSync",err,err,__LINE__,__FILE__,G.filename);
    }

    /* finally set FinderInfo */
    if (MacZipMode != UnKnown_EF) {
        err = SetFinderInfo(&CurrentFile, &newExtraField);
        printerr("close_outfile SetFinderInfo ", err, err,
                 __LINE__, __FILE__, G.filename);
    }

} /* end function close_outfile() */





#ifndef SFX

/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{
#if 0
    char buf[40];
#endif

    sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
      "gcc ", __VERSION__,
#else
#  if 0
      "cc ", (sprintf(buf, " version %d", _RELEASE), buf),
#  else
#  ifdef __MWERKS__
      "CodeWarrior C", "",
#  else
#  ifdef THINK_C
      "Think C", "",
#  else
#  ifdef MPW
      "MPW C", "",
#  else
      "unknown compiler", "",
#  endif
#  endif
#  endif
#  endif
#endif

      "MacOS",

#if defined(foobar) || defined(FOOBAR)
      " (Foo BAR)",    /* hardware or OS version */
#else
      "",
#endif /* Foo BAR */

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    (*G.message)((zvoid *)&G, slide, (ulg)strlen((char *)slide), 0);

} /* end function version() */

#endif /* !SFX */





/***********************/
/* Function macmkdir() */
/***********************/

int macmkdir(char *path)
{
    OSErr err = -1;
    OSErr err_rc;
    char CompletePath[NAME_MAX];
    FSSpec spec;
    Boolean isDirectory = false;
    short CurrentFork;
    unsigned pathlen;
    long dirID;

    GetExtraFieldData(&MacZipMode, &newExtraField);

    if (MacZipMode != UnKnown_EF) {
        RfDfFilen2Real(CompletePath, G.filename, MacZipMode,
                       (newExtraField.flags & EB_M3_FL_NOCHANGE), &CurrentFork);
        if (CurrentFork == ResourceFork)
            /* don't build a 'XtraStuf.mac:' dir  */
            return 0;
    }

    if (!IgnoreEF_Macfilename) {
        pathlen = strlen(path);
        strcpy(path, uO.exdir);
        strcat(path, ":");
        strcat(path, newExtraField.FullPath);
        path[pathlen] = 0x00;
    }

    GetCompletePath(CompletePath, path, &spec, &err);
    printerr("GetCompletePath", (err != -43) && (err != -120) && (err != 0),
             err, __LINE__, __FILE__, path);

    err = FSpGetDirectoryID(&spec, &dirID, &isDirectory);
    printerr("macmkdir FSpGetDirectoryID ", (err != -43) && (err != 0),
             err, __LINE__, __FILE__, path);
    if (err != -43)     /* -43 = file/directory not found  */
        return 0;
    else {
        HParamBlockRec    hpbr;

        hpbr.fileParam.ioCompletion = NULL;
        hpbr.fileParam.ioNamePtr = spec.name;
        hpbr.fileParam.ioVRefNum = spec.vRefNum;
        hpbr.fileParam.ioDirID = spec.parID;
        err = PBDirCreateSync(&hpbr);
        printerr("macmkdir PBDirCreateSync ", err,
                 err, __LINE__, __FILE__, CompletePath);

        if (MacZipMode != UnKnown_EF) {
            err_rc = SetFinderInfo(&spec, &newExtraField);
            printerr("macmkdir SetFinderInfo ", err_rc,
                     err_rc, __LINE__, __FILE__, CompletePath);
        }
    }

    return (int)err;
} /* macmkdir */




/**********************/
/* Function macopen() */
/**********************/

short macopen(char *sz, short nFlags)
{
    OSErr   err;
    char    chPerms = (!nFlags) ? fsRdPerm : fsRdWrPerm;
    short   nFRefNum;
    static char CompletePath[NAME_MAX];

    /* we only need the filespec of the zipfile;
       filespec of the other files (to be extracted) will be
       determined by open_outfile() */
    if (firstcall_of_macopen) {
        GetCompletePath(CompletePath, sz, &CurrentFile, &err);
        printerr("GetCompletePath", (err != -43) && (err != 0),
                 err, __LINE__, __FILE__, sz);
        /* we are working only with pathnames, this can cause big problems
         * on a Mac ...
         */
        if (CheckMountedVolumes(CompletePath) > 1)
            DoWarnUserDupVol(CompletePath);
        firstcall_of_macopen = false;
    }

    if (nFlags > 1) {
        err = HOpenRF(CurrentFile.vRefNum, CurrentFile.parID, CurrentFile.name,
                      chPerms, &nFRefNum);
        printerr("HOpenRF", (err != -43) && (err != 0) && (err != -54),
                 err, __LINE__, __FILE__, sz);
    } else {
        err = HOpen(CurrentFile.vRefNum, CurrentFile.parID, CurrentFile.name,
                    chPerms, &nFRefNum);
        printerr("HOpen", (err != -43) && (err != 0),
                 err, __LINE__, __FILE__, sz);
    }

    if ( err || (nFRefNum == 1) )
        return -1;
    else {
        if ( nFlags )
            SetEOF( nFRefNum, 0 );
        return nFRefNum;
    }
}





/***********************/
/* Function maccreat() */
/***********************/

short maccreat(char *sz)
{
    OSErr   err;
    char scriptTag = newExtraField.fpb.hFileInfo.ioFlXFndrInfo.fdScript;

    sz = sz;

    /* Set fdScript in FXInfo
     * The negative script constants (smSystemScript, smCurrentScript,
     * and smAllScripts) don't make sense on disk.  So only use scriptTag
     * if scriptTag >= smRoman (smRoman is 0).
     * fdScript is valid if high bit is set (see IM-6, page 9-38)
     */
    scriptTag = (scriptTag >= smRoman) ?
                ((char)scriptTag | (char)0x80) : (smRoman);
    newExtraField.fpb.hFileInfo.ioFlXFndrInfo.fdScript = scriptTag;

    err = FSpCreate(&CurrentFile,
                    newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdCreator,
                    newExtraField.fpb.hFileInfo.ioFlFndrInfo.fdType,
                    newExtraField.fpb.hFileInfo.ioFlXFndrInfo.fdScript);
    err = printerr("FSpCreate maccreat ", (err != -48) && (err != 0),
                   err, __LINE__, __FILE__, G.filename);

    if (err == noErr)
        return noErr;
    else
        return -1;
}





/**********************/
/* Function macread() */
/**********************/

short macread(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;

    (void)FSRead( nFRefNum, &lcb, pb );

    return (short)lcb;
}




/***********************/
/* Function macwrite() */
/***********************/

long macwrite(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;
    OSErr   err;
    FILE    *stream;

    if ( (nFRefNum == 1) || (nFRefNum == 2) )
        {
        stream = (nFRefNum == 1 ? stdout : stderr);
        pb[cb] = '\0';           /* terminate C-string */
                                 /* assumes writable buffer (e.g., slide[]) */
                                 /* with room for one more char at end of buf */
        MakeMacOS_String( pb, ' ', ' ', ' ', ' ');
        lcb = fprintf(stream, pb);
        }
    else
        err = FSWrite( nFRefNum, &lcb, pb );


    if (err != 0)
        {
        errno = ERANGE;
        return -1;
        }


    return (long)lcb;
}





/***********************/
/* Function macclose() */
/***********************/

short macclose(short nFRefNum)
{
OSErr err;

err = FSClose( nFRefNum );
printerr("macclose FSClose ",err,err, __LINE__,__FILE__,G.filename);


return err;
}





/***********************/
/* Function maclseek() */
/***********************/

long maclseek(short nFRefNum, long lib, short nMode)
{
    ParamBlockRec   pbr;

    if (nMode == SEEK_SET)
        nMode = fsFromStart;
    else if (nMode == SEEK_CUR)
        nMode = fsFromMark;
    else if (nMode == SEEK_END)
        nMode = fsFromLEOF;
    pbr.ioParam.ioRefNum = nFRefNum;
    pbr.ioParam.ioPosMode = nMode;
    pbr.ioParam.ioPosOffset = lib;
    (void)PBSetFPosSync(&pbr);
    return pbr.ioParam.ioPosOffset;
}




/***********************************/
/* Function FindNewExtractFolder() */
/***********************************/

char *FindNewExtractFolder(char *ExtractPath)
{
char buffer[NAME_MAX];
short count = 0, length = strlen(ExtractPath) - 2;
OSErr err;
FSSpec Spec;
long theDirID;
Boolean isDirectory;

for (count = 0; count < 99; count++)
    {
    memset(buffer,0,sizeof(buffer));
    ExtractPath[length] = 0;
    sprintf(buffer,"%s%d",ExtractPath,count);
    GetCompletePath(ExtractPath, buffer, &Spec,&err);
    err = FSpGetDirectoryID(&Spec, &theDirID, &isDirectory);
    if (err == -43) return ExtractPath;
    }

return ExtractPath; /* can't find unique path, defaults to Extract-Folder */
}



/* The following functions are dealing with the extra-field handling, only. */

/****************************/
/* Function SetFinderInfo() */
/****************************/

static OSErr SetFinderInfo(FSSpec *spec, MACINFO *mi)
{
    OSErr err;
    CInfoPBRec      fpb;

    fpb.hFileInfo.ioNamePtr   = (StringPtr) &(spec->name);
    fpb.hFileInfo.ioVRefNum   = spec->vRefNum;
    fpb.hFileInfo.ioDirID     = spec->parID;
    fpb.hFileInfo.ioFDirIndex = 0;

    err = PBGetCatInfo(&fpb, false);
    printerr("PBGetCatInfo SetFinderInfo ", err, err,
             __LINE__, __FILE__, G.filename);

    if  ((MacZipMode == JohnnyLee_EF) || (MacZipMode == NewZipMode_EF))
    {
        if (!UseUT_ExtraField)  {
            fpb.hFileInfo.ioFlCrDat = mi->fpb.hFileInfo.ioFlCrDat;
            fpb.hFileInfo.ioFlMdDat = mi->fpb.hFileInfo.ioFlMdDat;
        }

        fpb.hFileInfo.ioFlFndrInfo   = mi->fpb.hFileInfo.ioFlFndrInfo;
    }

    if (MacZipMode == NewZipMode_EF)
    {
        if (uO.E_flag) PrintMacExtraInfo();

        fpb.hFileInfo.ioFlXFndrInfo  = mi->fpb.hFileInfo.ioFlXFndrInfo;

        fpb.hFileInfo.ioFVersNum  = mi->fpb.hFileInfo.ioFVersNum;
        fpb.hFileInfo.ioACUser    = mi->fpb.hFileInfo.ioACUser;

        if (!UseUT_ExtraField) {
            fpb.hFileInfo.ioFlBkDat = mi->fpb.hFileInfo.ioFlBkDat;
#ifdef USE_EF_UT_TIME
            if (
#ifdef IZ_CHECK_TZ
                G.tz_is_valid &&
#endif
                !(mi->flags & EB_M3_FL_NOUTC))
                {
#ifdef DEBUG_TIME
            {
            printf("\nSetFinderInfo:  Mac modif: %lu local -> UTOffset: %d before AdjustForTZmoveMac\n",
              fpb.hFileInfo.ioFlCrDat, mi->Cr_UTCoffs);
            }
#endif /* DEBUG_TIME */
                fpb.hFileInfo.ioFlCrDat =
                  AdjustForTZmoveMac(fpb.hFileInfo.ioFlCrDat, mi->Cr_UTCoffs);
                fpb.hFileInfo.ioFlMdDat =
                  AdjustForTZmoveMac(fpb.hFileInfo.ioFlMdDat, mi->Md_UTCoffs);
                fpb.hFileInfo.ioFlBkDat =
                  AdjustForTZmoveMac(fpb.hFileInfo.ioFlBkDat, mi->Bk_UTCoffs);
#ifdef DEBUG_TIME
            {
            printf("SetFinderInfo:  Mac modif: %lu local -> UTOffset: %d after AdjustForTZmoveMac\n",
              fpb.hFileInfo.ioFlCrDat, mi->Cr_UTCoffs);
            }
#endif /* DEBUG_TIME */

                }
#endif /* USE_EF_UT_TIME */
        }

        if (mi->FinderComment)  {
            C2PStr(mi->FinderComment);
            err = FSpDTSetComment(spec, (unsigned char *) mi->FinderComment);
            printerr("FSpDTSetComment:",err , err,
                     __LINE__, __FILE__, mi->FullPath);
        }
    }

    /* Restore ioDirID field in pb which was changed by PBGetCatInfo */
    fpb.hFileInfo.ioDirID = spec->parID;
    err = PBSetCatInfo(&fpb, false);

    return err;
} /* SetFinderInfo() */




/*
** Scan the extra fields in extra_field, and look for a MacOS EF; return a
** pointer to that EF, or NULL if it's not there.
*/
static uch *scanMacOSexfield(uch *ef_ptr, unsigned ef_len,
                             short *MacZipMode)
{
    while (ef_ptr != NULL && ef_len >= EB_HEADSIZE) {
        unsigned eb_id  = makeword(EB_ID  + ef_ptr);
        unsigned eb_len = makeword(EB_LEN + ef_ptr);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            Trace((stderr,
              "scanMacOSexfield: block length %u > rest ef_size %u\n", eb_len,
              ef_len - EB_HEADSIZE));
            break;
        }

        if (isMacOSexfield(eb_id, eb_len, MacZipMode)) {
            return ef_ptr;
        }

        ef_ptr += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return NULL;
}




static Boolean isMacOSexfield(unsigned id, unsigned size, short *MacZipMode)
{
    switch (id)
        {
        case EF_ZIPIT:
        case EF_ZIPIT2:
            {               /* we do not support ZipIt's format completly  */
            Info(slide, 0x221, ((char *)slide,
              "warning: found %s ZipIt extra field (size %u) -> "\
              "file is probably not usable!!\n",
              (id == EF_ZIPIT2 ? "short": "long", size)));
            IgnoreEF_Macfilename = true;
            return false;
            }

        case EF_MAC3:
            {
            *MacZipMode = NewZipMode_EF;
            IgnoreEF_Macfilename = false;
            return true;
            }

        case EF_JLMAC:
            {
            *MacZipMode = JohnnyLee_EF;
            IgnoreEF_Macfilename = true;
            return true;
            }

        default:
            {
            *MacZipMode = UnKnown_EF;
            IgnoreEF_Macfilename = true;
            return false;
            }
        }

    return false;
}




/*
** Return a unsigned long from a four-byte sequence
** in big endian format
*/

ulg makePPClong(ZCONST uch *sig)
{
    return (((ulg)sig[0]) << 24)
         + (((ulg)sig[1]) << 16)
         + (((ulg)sig[2]) << 8)
         +  ((ulg)sig[3]);
}




/*
** Return a unsigned short from a two-byte sequence
** in big endian format
*/

ush makePPCword(ZCONST uch *b)
{
    return (ush)((b[0] << 8) | b[1]);
}




/*
** Print mac extra-field
**
*/

static void PrintMacExtraInfo(void)
{
#define MY_FNDRINFO fpb.hFileInfo.ioFlFndrInfo
    DateTimeRec  MacTime;
    static ZCONST char space[] = "                                    ";
    static ZCONST char line[]  = "------------------------------------"\
                                 "------------------------------";

    printf("\n\n%s", line);

    printf("\nFullPath      = [%s]", newExtraField.FullPath);
    printf("\nFinderComment = [%s]", newExtraField.FinderComment);
    printf("\nText Encoding Base (Filename)       \"%s\" \n",
        PrintTextEncoding(newExtraField.fpb.hFileInfo.ioFlXFndrInfo.fdScript));

    printf("\nExtraField Flags :                  %s  0x%x  %4d",
        sBit2Str(newExtraField.flags), newExtraField.flags,
        newExtraField.flags);

    printf("\n%sExtra Field is %s", space,
           (newExtraField.flags & EB_M3_FL_UNCMPR ?
            "Uncompressed" : "Compressed"));
    printf("\n%sFile Dates are in %u Bit", space,
           (newExtraField.flags & EB_M3_FL_TIME64 ? 64 : 32));
    printf("\n%sFile UTC time adjustments are %ssupported", space,
           (newExtraField.flags & EB_M3_FL_NOUTC ? "not " : ""));
    printf("\n%sFile Name is %schanged", space,
           (newExtraField.flags & EB_M3_FL_NOCHANGE ? "not " : ""));
    printf("\n%sFile is a %s\n", space,
           (newExtraField.flags & EB_M3_FL_DATFRK ?
            "Datafork" : "Resourcefork"));

    /* not all type / creator codes are printable */
    if (isprint((char)(newExtraField.MY_FNDRINFO.fdType >> 24)) &&
        isprint((char)(newExtraField.MY_FNDRINFO.fdType >> 16)) &&
        isprint((char)(newExtraField.MY_FNDRINFO.fdType >> 8)) &&
        isprint((char)newExtraField.MY_FNDRINFO.fdType))
    {
        printf("\nFile Type =                         [%c%c%c%c]  0x%lx",
            (char)(newExtraField.MY_FNDRINFO.fdType >> 24),
            (char)(newExtraField.MY_FNDRINFO.fdType >> 16),
            (char)(newExtraField.MY_FNDRINFO.fdType >> 8),
            (char)(newExtraField.MY_FNDRINFO.fdType),
            newExtraField.MY_FNDRINFO.fdType);
    }
    else
    {
        printf("\nFile Type =                                     0x%lx",
            newExtraField.MY_FNDRINFO.fdType);
    }

    if (isprint((char)(newExtraField.MY_FNDRINFO.fdCreator >> 24)) &&
        isprint((char)(newExtraField.MY_FNDRINFO.fdCreator >> 16)) &&
        isprint((char)(newExtraField.MY_FNDRINFO.fdCreator >> 8)) &&
        isprint((char)newExtraField.MY_FNDRINFO.fdCreator))
    {
        printf("\nFile Creator =                      [%c%c%c%c]  0x%lx",
            (char)(newExtraField.MY_FNDRINFO.fdCreator >> 24),
            (char)(newExtraField.MY_FNDRINFO.fdCreator >> 16),
            (char)(newExtraField.MY_FNDRINFO.fdCreator >> 8),
            (char)(newExtraField.MY_FNDRINFO.fdCreator),
            newExtraField.MY_FNDRINFO.fdCreator);
    }
    else
    {
        printf("\nFile Creator =                                  0x%lx",
            newExtraField.MY_FNDRINFO.fdCreator);
    }

    printf("\n\nDates (local time of archiving location):");
    SecondsToDate(newExtraField.fpb.hFileInfo.ioFlCrDat, &MacTime);
    printf("\n    Created  =                      %4d/%2d/%2d %2d:%2d:%2d  ",
           MacTime.year, MacTime.month, MacTime.day,
           MacTime.hour, MacTime.minute, MacTime.second);
    SecondsToDate(newExtraField.fpb.hFileInfo.ioFlMdDat, &MacTime);
    printf("\n    Modified =                      %4d/%2d/%2d %2d:%2d:%2d  ",
           MacTime.year, MacTime.month, MacTime.day,
           MacTime.hour, MacTime.minute, MacTime.second);
    SecondsToDate(newExtraField.fpb.hFileInfo.ioFlBkDat, &MacTime);
    printf("\n    Backup   =                      %4d/%2d/%2d %2d:%2d:%2d  ",
        MacTime.year, MacTime.month, MacTime.day,
        MacTime.hour, MacTime.minute, MacTime.second);

    if (!(newExtraField.flags & EB_M3_FL_NOUTC)) {
        printf("\nGMT Offset of Creation time  =      %4ld sec  %2d h",
          newExtraField.Cr_UTCoffs, (int)newExtraField.Cr_UTCoffs / (60 * 60));
        printf("\nGMT Offset of Modification time  =  %4ld sec  %2d h",
          newExtraField.Md_UTCoffs, (int)newExtraField.Md_UTCoffs / (60 * 60));
        printf("\nGMT Offset of Backup time  =        %4ld sec  %2d h",
          newExtraField.Bk_UTCoffs, (int)newExtraField.Bk_UTCoffs / (60 * 60));
    }

    printf("\n\nFinder Flags :                      %s  0x%x  %4d",
        sBit2Str(newExtraField.MY_FNDRINFO.fdFlags),
        newExtraField.MY_FNDRINFO.fdFlags,
        newExtraField.MY_FNDRINFO.fdFlags);

    printf("\nFinder Icon Position =              X: %4d",
        newExtraField.MY_FNDRINFO.fdLocation.h);

    printf("\n                                    Y: %4d",
        newExtraField.MY_FNDRINFO.fdLocation.v);

    printf("\n\nText Encoding Base (System/MacZip)  \"%s\"",
        PrintTextEncoding(newExtraField.TextEncodingBase));

    printf("\n%s", line);
#undef MY_FNDRINFO
}




/*
** Decode mac extra-field and assign the data to the structure
**
*/

static Boolean GetExtraFieldData(short *MacZipMode, MACINFO *mi)
{
uch *ptr;
uch *attrbuff  = NULL;
int  retval = PK_OK;
Boolean MallocWasUsed = false;

ptr = scanMacOSexfield(G.extra_field, G.lrec.extra_field_length, MacZipMode);

/* MacOS is no preemptive OS therefore do some (small) event-handling */
UserStop();

if (uO.J_flag) {
    *MacZipMode = UnKnown_EF;
    IgnoreEF_Macfilename = true;
    return false;
}


if (ptr != NULL)
{
      /*   Collect the data from the extra field buffer. */
    mi->header    = makeword(ptr);    ptr += 2;
    mi->data      = makeword(ptr);    ptr += 2;

    switch (*MacZipMode)
        {
        case NewZipMode_EF:
          {
            mi->size      =  makelong(ptr); ptr += 4;
            mi->flags     =  makeword(ptr); ptr += 2;
            /* Type/Creator are always uncompressed */
            mi->fpb.hFileInfo.ioFlFndrInfo.fdType    = makePPClong(ptr);
            ptr += 4;
            mi->fpb.hFileInfo.ioFlFndrInfo.fdCreator = makePPClong(ptr);
            ptr += 4;

            if (!(mi->flags & EB_M3_FL_UNCMPR)) {
                 /* compressed extra field is not yet tested,
                    therefore it's currently not used by default */
                attrbuff = (uch *)malloc(mi->size);
                if (attrbuff == NULL) {
                    /* No memory to uncompress attributes */
                    Info(slide, 0x201, ((char *)slide,
                         "Can't allocate memory to uncompress file "\
                         "attributes.\n"));
                    *MacZipMode = UnKnown_EF;
                            /* EF-Block is unusable, ignore it */
                    return false;
                }
                else MallocWasUsed = true;

                retval = memextract(__G__ attrbuff, mi->size, ptr,
                                    mi->data - EB_MAC3_HLEN);

                if (retval != PK_OK) {
                    /* error uncompressing attributes */
                    Info(slide, 0x201, ((char *)slide,
                         "Error uncompressing file attributes.\n"));
                    free(attrbuff);
                    *MacZipMode = UnKnown_EF;
                            /* EF-Block unusable, ignore it */
                    return false;
                }
            } else {            /* file attributes are uncompressed */
                attrbuff = ptr;
            }
            DecodeMac3ExtraField(attrbuff, mi);
            if (MallocWasUsed) free(attrbuff);
            return true;
            break;
          }

        case JohnnyLee_EF:
          {
            if (strncmp((char *)ptr, "JLEE", 4) == 0) {
                /* Johnny Lee's old MacZip e.f. was found */
                attrbuff  = ptr + 4;

                DecodeJLEEextraField(attrbuff, mi);
                return true;
            } else {
                /* second signature did not match, ignore EF block */
                *MacZipMode = UnKnown_EF;
                return false;
            }
            break;
          }

        default:
          {  /* just to make sure */
            *MacZipMode = UnKnown_EF;
            IgnoreEF_Macfilename = true;
            return false;
            break;
          }
        }
}  /* if (ptr != NULL)  */

/* no Mac extra field was found */
return false;
}




/*
** Assign the new Mac3 Extra-Field to the structure
**
*/

static void DecodeMac3ExtraField(ZCONST uch *buff, MACINFO *mi)
{               /* extra-field info of the new MacZip implementation */
                /* compresssed extra-field starts here (if compressed) */
mi->fpb.hFileInfo.ioFlFndrInfo.fdFlags      =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdLocation.v =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdLocation.h =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdFldr       =  makeword(buff); buff += 2;

mi->fpb.hFileInfo.ioFlXFndrInfo.fdIconID    =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdUnused[0] =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdUnused[1] =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdUnused[2] =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdScript    = *buff;           buff += 1;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdXFlags    = *buff;           buff += 1;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdComment   =  makeword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlXFndrInfo.fdPutAway   =  makelong(buff); buff += 4;

mi->fpb.hFileInfo.ioFVersNum                = *buff;           buff += 1;
mi->fpb.hFileInfo.ioACUser                  = *buff;           buff += 1;

if (mi->flags & EB_M3_FL_TIME64) {
    Info(slide, 0x201, ((char *)slide,
         "Don't support 64 bit Timevalues; get a newer version of MacZip \n"));
    UseUT_ExtraField = true;
    buff += 24; /* jump over the date values */
} else {
    UseUT_ExtraField = false;
    mi->fpb.hFileInfo.ioFlCrDat   =  makelong(buff); buff += 4;
    mi->fpb.hFileInfo.ioFlMdDat   =  makelong(buff); buff += 4;
    mi->fpb.hFileInfo.ioFlBkDat   =  makelong(buff); buff += 4;
}

if (!(mi->flags & EB_M3_FL_NOUTC))  {
    mi->Cr_UTCoffs =  makelong(buff); buff += 4;
    mi->Md_UTCoffs =  makelong(buff); buff += 4;
    mi->Bk_UTCoffs =  makelong(buff); buff += 4;
}

/* TextEncodingBase type & values */
/* (values 0-32 correspond to the Script Codes defined in "Inside Macintosh",
    Text pages 6-52 and 6-53) */
mi->TextEncodingBase =  makeword(buff); buff += 2;
if (mi->TextEncodingBase >= kTextEncodingUnicodeV1_1)  {
    Info(slide, 0x201, ((char *)slide,
         "Don't support Unicoded Filenames; get a newer version of MacZip\n"));
    IgnoreEF_Macfilename = true;
}

mi->FullPath      = (char *)buff; buff += strlen(mi->FullPath) + 1;
mi->FinderComment = (char *)buff; buff += strlen(mi->FinderComment) + 1;

if (uO.i_flag) IgnoreEF_Macfilename = true;

}




/*
** Assign the new JLEE Extra-Field to the structure
**
*/

static void DecodeJLEEextraField(ZCONST uch *buff, MACINFO *mi)
{ /*  extra-field info of Johnny Lee's old MacZip  */
mi->fpb.hFileInfo.ioFlFndrInfo.fdType       = makePPClong(buff); buff += 4;
mi->fpb.hFileInfo.ioFlFndrInfo.fdCreator    = makePPClong(buff); buff += 4;
mi->fpb.hFileInfo.ioFlFndrInfo.fdFlags      = makePPCword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdLocation.v = makePPCword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdLocation.h = makePPCword(buff); buff += 2;
mi->fpb.hFileInfo.ioFlFndrInfo.fdFldr       = makePPCword(buff); buff += 2;

mi->fpb.hFileInfo.ioFlCrDat                =  makePPClong(buff); buff += 4;
mi->fpb.hFileInfo.ioFlMdDat                =  makePPClong(buff); buff += 4;
mi->flags                                  =  makePPClong(buff); buff += 4;

     /* scriptTag isn't stored in Johnny Lee's ef definiton */
newExtraField.fpb.hFileInfo.ioFlXFndrInfo.fdScript = smRoman;
}




/*
** Return char* to describe the text encoding
**
*/

static char *PrintTextEncoding(short script)
{
char *info;
static char buffer[14];
/* TextEncodingBase type & values */
/* (values 0-32 correspond to the Script Codes defined in
   Inside Macintosh: Text pages 6-52 and 6-53 */

switch (script) {               /* Mac OS encodings*/
    case kTextEncodingMacRoman:         info = "Roman";             break;
    case kTextEncodingMacJapanese:      info = "Japanese";          break;
    case kTextEncodingMacChineseTrad:   info = "ChineseTrad";       break;
    case kTextEncodingMacKorean:        info = "Korean";            break;
    case kTextEncodingMacArabic:        info = "Arabic";            break;
    case kTextEncodingMacHebrew:        info = "Hebrew";            break;
    case kTextEncodingMacGreek:         info = "Greek";             break;
    case kTextEncodingMacCyrillic:      info = "Cyrillic";          break;
    case kTextEncodingMacDevanagari:    info = "Devanagari";        break;
    case kTextEncodingMacGurmukhi:      info = "Gurmukhi";          break;
    case kTextEncodingMacGujarati:      info = "Gujarati";          break;
    case kTextEncodingMacOriya:         info = "Oriya";             break;
    case kTextEncodingMacBengali:       info = "Bengali";           break;
    case kTextEncodingMacTamil:         info = "Tamil";             break;
    case kTextEncodingMacTelugu:        info = "Telugu";            break;
    case kTextEncodingMacKannada:       info = "Kannada";           break;
    case kTextEncodingMacMalayalam:     info = "Malayalam";         break;
    case kTextEncodingMacSinhalese:     info = "Sinhalese";         break;
    case kTextEncodingMacBurmese:       info = "Burmese";           break;
    case kTextEncodingMacKhmer:         info = "Khmer";             break;
    case kTextEncodingMacThai:          info = "Thai";              break;
    case kTextEncodingMacLaotian:       info = "Laotian";           break;
    case kTextEncodingMacGeorgian:      info = "Georgian";          break;
    case kTextEncodingMacArmenian:      info = "Armenian";          break;
    case kTextEncodingMacChineseSimp:   info = "ChineseSimp";       break;
    case kTextEncodingMacTibetan:       info = "Tibetan";           break;
    case kTextEncodingMacMongolian:     info = "Mongolian";         break;
    case kTextEncodingMacEthiopic:      info = "Ethiopic";          break;
    case kTextEncodingMacCentralEurRoman: info = "CentralEurRoman"; break;
    case kTextEncodingMacVietnamese:    info = "Vietnamese";        break;
    case kTextEncodingMacExtArabic:     info = "ExtArabic";         break;

    case kTextEncodingUnicodeV1_1:      info = "Unicode V 1.1";     break;
    case kTextEncodingUnicodeV2_0:      info = "Unicode V 2.0";     break;

    default:  {
        sprintf(buffer,"Code: 0x%x",(short) script);
        info = buffer;
        break;
        }
    }

return info;
}



/*
** Init Globals
**
*/

void   MacGlobalsInit(__GPRO)
{
newExtraField.FullPath      = NULL;
newExtraField.FinderComment = NULL;

firstcall_of_macopen = true;

MacZipMode = UnKnown_EF;
IgnoreEF_Macfilename = true;

}



/*
** Convert the MacOS-Strings (Filenames/Findercomments) to a most compatible.
** These strings will be stored in the public area of the zip-archive.
** Every foreign platform (outside macos) will access these strings
** for extraction.
*/

static void MakeMacOS_String(char *MacOS_Str,
            const char SpcChar1, const char SpcChar2,
            const char SpcChar3, const char SpcChar4)
{
    char *tmpPtr;
    register uch curch;

    for (tmpPtr = MacOS_Str; (curch = *tmpPtr) != '\0'; tmpPtr++)
    {
        if (curch == SpcChar1)
            *tmpPtr = SpcChar2;
        else
        if (curch == SpcChar3)
            *tmpPtr = SpcChar4;
        else  /* default */
            if (curch > 127)
               {
                   *tmpPtr = (char)ISO8859_1_to_MacRoman[curch - 128];
               }
    }  /* end for */
}
