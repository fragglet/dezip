/*
 * routines common to TANDEM
 */

#define UNZIP_INTERNAL
#include "unzip.h"

#include <tal.h>
#include "$system.zsysdefs.zsysc" nolist
#include <cextdecs(FILE_GETINFOLISTBYNAME_, \
                   FILENAME_SCAN_,          \
                   INTERPRETTIMESTAMP,      \
                   CONVERTTIMESTAMP         \
                  )>
#include <cextdecs(FILENAME_FINDSTART_, \
                   FILENAME_FINDNEXT_,  \
                   FILENAME_FINDFINISH_ \
                  )>
#include <cextdecs(SETMODE)>
#ifdef LICENSED
#include <cextdecs(COMPUTETIMESTAMP,     \
                   PROCESS_GETINFO_,     \
                   PROCESS_GETINFOLIST_, \
                   PROCESSHANDLE_NULLIT_ \
                  )>
#endif /* LICENSED */

char *in2ex OF((char *));



void zexit(status)
  int status;
{
  terminate_program (0,0,status,,,);   /* Exit(>0) creates saveabend files */
}


#ifdef fopen
#  undef fopen
#endif

FILE *zipopen(fname, opt)
  const char *fname;
  const char *opt;
{
  int fdesc, fnum, err;

  if (strcmp(opt,FOPW) == 0)
    if ((fdesc = creat(fname,,100,500)) != -1) {
      fnum = fdtogfn(fdesc);
      err = SETMODE(fnum, SET_FILE_BUFFERSIZE, TANDEM_BLOCKSIZE);
      err = SETMODE(fnum, SET_FILE_BUFFERED, 0, 0);
      err = SETMODE(fnum, SET_FILE_BUFFERED, 0, 1);
      err = close(fdesc);
    }

  return fopen(fname,opt);
}
#define fopen zipopen


#ifdef putc
#  undef putc
#endif

int zputc(ch, fptr)
  int ch;
  FILE *fptr;
{
  int err;
  err = putc(ch,fptr);
  fflush(fptr);
  return err;
}
#define putc zputc


#ifdef LICENSED
_tal _priv short FILE_CHANGELABEL_ (
 short,          /* IN */
 short,          /* IN */
 short _far *    /* IN */
 );

_c _callable int changelabel OF((short, const short *, const short *));

_c _callable int changelabel(fnum, modtime, actime)
  short fnum;
  const short *modtime;
  const short *actime;
{
  int err;

  err = FILE_CHANGELABEL_(fnum, 16, modtime);
  if (!err)
    err = FILE_CHANGELABEL_(fnum, 17, actime);
  return err;
}

int islicensed OF((void));

int islicensed(void)
{
  #define plist_items 1
  #define plist_size 10

  short myphandle[ZSYS_VAL_PHANDLE_WLEN];
  short licensetag[plist_items] = {37};
  short licensed[plist_size];
  short maxlen = plist_size;
  short items = plist_items;
  short resultlen[1], err;

  err = PROCESSHANDLE_NULLIT_(myphandle);

  if (!err)
    err = PROCESS_GETINFO_(myphandle);

  if (!err)
    err = PROCESS_GETINFOLIST_(/*cpu*/,
                               /*pin*/,
                               /*nodename*/,
                               /*nodenamelen*/,
                               myphandle,
                               licensetag,
                               items,
                               licensed,
                               maxlen,
                               resultlen
                              );

  if (err != 0)
    return 0;
  else
    return licensed[0];
}
#endif /* LICENSED */


int utime OF((const char *, const ztimbuf *));

int utime(file, time)
  const char *file;
  const ztimbuf *time;
{
#ifdef LICENSED
  int fdesc, result, err;
  union timestamp_ov {
    long long fulltime;
    short wordtime[4];
  };
  union timestamp_ov lasttime, opentime;
  struct tm *modt, *opent;
  short datetime[8], errormask[1], fnum;

  if (islicensed() ) {
    /* Attempt to update file label */
    modt = gmtime( &time->modtime );

    datetime[0] = modt->tm_year + 1900;
    datetime[1] = modt->tm_mon + 1;
    datetime[2] = modt->tm_mday;
    datetime[3] = modt->tm_hour;
    datetime[4] = modt->tm_min;
    datetime[5] = modt->tm_sec;
    datetime[6] = datetime[7] = 0;
    errormask[0] = 0;
    lasttime.fulltime = COMPUTETIMESTAMP (datetime, errormask);

    opent = gmtime( &time->actime );

    datetime[0] = opent->tm_year + 1900;
    datetime[1] = opent->tm_mon + 1;
    datetime[2] = opent->tm_mday;
    datetime[3] = opent->tm_hour;
    datetime[4] = opent->tm_min;
    datetime[5] = opent->tm_sec;
    datetime[6] = datetime[7] = 0;
    errormask[0] = 0;
    opentime.fulltime = COMPUTETIMESTAMP (datetime, errormask);

    fdesc = open(file, O_WRONLY);
    fnum = fdtogfn(fdesc);
    result = changelabel(fnum,lasttime.wordtime,opentime.wordtime);
    err = close(fdesc);
    return result;
  }
  return -1;
#else  /* !LICENSED */
  return 0;             /* "no error", to suppress annoying failure messages */
#endif  /* ?LICENSED */
}


/* TANDEM version of chmod() function */

int chmod(file, unix_sec)
  const char *file;
  mode_t unix_sec;
{
  FILE *stream;
  struct nsk_sec_type {
    unsigned progid : 1;
    unsigned clear  : 1;
    unsigned null   : 2;
    unsigned read   : 3;
    unsigned write  : 3;
    unsigned execute: 3;
    unsigned purge  : 3;
  };
  union nsk_sec_ov {
    struct nsk_sec_type bit_ov;
    short int_ov;
  };
  union nsk_sec_ov nsk_sec;
  short fnum, fdes, err, nsk_sec_int;


  nsk_sec.bit_ov.progid = 0;
  nsk_sec.bit_ov.clear  = 0;
  nsk_sec.bit_ov.null   = 0;

  /*  4="N", 5="C", 6="U", 7="-"   */

  if (unix_sec & S_IROTH) nsk_sec.bit_ov.read = 4;
  else if (unix_sec & S_IRGRP) nsk_sec.bit_ov.read = 5;
  else if (unix_sec & S_IRUSR) nsk_sec.bit_ov.read = 6;
  else nsk_sec.bit_ov.read = 7;

  if (unix_sec & S_IWOTH) nsk_sec.bit_ov.write = 4;
  else if (unix_sec & S_IWGRP) nsk_sec.bit_ov.write = 5;
  else if (unix_sec & S_IWUSR) nsk_sec.bit_ov.write = 6;
  else nsk_sec.bit_ov.write = 7;

  if (unix_sec & S_IXOTH) nsk_sec.bit_ov.execute = 4;
  else if (unix_sec & S_IXGRP) nsk_sec.bit_ov.execute = 5;
  else if (unix_sec & S_IXUSR) nsk_sec.bit_ov.execute = 6;
  else nsk_sec.bit_ov.execute = 7;

  nsk_sec.bit_ov.purge = nsk_sec.bit_ov.write;

  nsk_sec_int = nsk_sec.int_ov;

  if ((fdes = open(file, (O_EXCLUSIVE | O_RDONLY))) == -1)
    return -1;
  fnum = fdtogfn(fdes);
  err = SETMODE(fnum, SET_FILE_SECURITY, nsk_sec_int);
  close(fdes);
  return (err != 0 ? -1 : 0);
}


/* TANDEM version of chown() function */

int chown(file, uid, gid)
  const char *file;
  uid_t uid;
  gid_t gid;
{
  FILE *stream;
  struct nsk_own_type {
    unsigned group  : 8;
    unsigned user   : 8;
  };
  union nsk_own_ov {
    struct nsk_own_type bit_ov;
    short int_ov;
  };
  union nsk_own_ov nsk_own;
  short fnum, fdes, err, nsk_own_int;

  nsk_own.bit_ov.group = gid;
  nsk_own.bit_ov.user  = uid;

  nsk_own_int = nsk_own.int_ov;

  if ((fdes = open(file, (O_EXCLUSIVE | O_RDONLY))) == -1)
    return -1;
  fnum = fdtogfn(fdes);
  err = SETMODE(fnum, SET_FILE_OWNER, nsk_own_int);
  close(fdes);
  return (err != 0 ? -1 : 0);
}


/* TANDEM version of stat() function */

time_t gmt_to_time_t (long long *);

time_t gmt_to_time_t (gmt)
  long long *gmt;
{
  #define GMT_TO_LCT 0;
  #define GMT_TO_LST 1;

  struct tm temp_tm;
  short  date_time[8];
  long   julian_dayno;
  long long lct, lst, itime;
  short  err[1], type;

  type = GMT_TO_LCT;
  lct = CONVERTTIMESTAMP(*gmt, type,, err);

  if (!err[0]) {
    type = GMT_TO_LST;
    lst = CONVERTTIMESTAMP(*gmt, type,, err);
  }

  itime = (err[0] ? *gmt : lct);
  temp_tm.tm_isdst = (err[0] ? -1 : ((lct == lst) ? 0 : 1));

  julian_dayno = INTERPRETTIMESTAMP (itime, date_time);

  temp_tm.tm_sec   = date_time[5];
  temp_tm.tm_min   = date_time[4];
  temp_tm.tm_hour  = date_time[3];
  temp_tm.tm_mday  = date_time[2];
  temp_tm.tm_mon   = date_time[1] - 1;     /* C's so sad */
  temp_tm.tm_year  = date_time[0] - 1900;  /* it's almost funny */

  return (mktime(&temp_tm));
}

short parsename( const char *, char *, char * );

short parsename(srce, fname, ext)
  const char *srce;
  char *fname;
  char *ext;
{
  /* As a way of supporting DOS extensions from Tandem we look for a space
     separated extension string after the Guardian filename
     e.g. ZIP ZIPFILE "$DATA4.TESTING.INVOICE TXT"
  */

  char *fstart;
  char *fptr;
  short extension = 0;

  *fname = *ext = '\0';  /* set to null string */

  fstart = (char *) srce;

  if ((fptr = strrchr(fstart, TANDEM_EXTENSION)) != NULL) {
    extension = 1;

    fptr++;
    strncat(ext, fptr, _min(EXTENSION_MAX, strlen(fptr)));

    fptr = strchr(fstart, TANDEM_EXTENSION);  /* End of filename */
    strncat(fname, fstart, _min(FILENAME_MAX, (fptr - fstart)));
  }
  else {
    /* just copy string */
    strncat(fname, srce, _min(FILENAME_MAX, strlen(srce)));
  }

  return extension;
}

int stat(n, s)
const char *n;
struct stat *s;
{
  #define flist_items 14
  #define flist_size 200

  short err, i, extension;
  char fname[FILENAME_MAX + 1];
  short fnamelen;
  char ext[EXTENSION_MAX + 1];
                         /* #0  #1  #2  #3 #4 #5 #6 #7 #8 #9 #10 #11 #12 #13 */
/*short ilist[flist_items]={62,117,145,142,58,41,42,30,31,75, 78, 79, 60,119}*/
  short ilist[flist_items]={62, 56,144,142,58,41,42,30,31,75, 78, 79, 60, 54};
  short ilen[flist_items] ={ 2,  4,  4,  2, 1, 1, 1, 1, 1, 1,  1,  1,  1,  4};
  short ioff[flist_items];
  short flist[flist_size];
  short extra[2];
  short *rlen=&extra[0];
  short *err_item=&extra[1];
  unsigned short *fowner;
  unsigned short *fprogid;
  char *fsec;

  short end, count, kind, level, options, searchid;
  short info[5];

  /* Initialise stat structure */
  s->st_dev = _S_GUARDIANOBJECT;
  s->st_ino = 0;
  s->st_nlink = 0;
  s->st_rdev = 0;
  s->st_uid = s->st_gid = 0;
  s->st_size = 0;
  s->st_atime = s->st_ctime = s->st_mtime = 0;
  s->st_reserved[0] = 0;

  /* Check to see if name contains a (pseudo) file extension */
  extension = parsename (n,fname,ext);

  fnamelen = strlen(fname);

  options = 3; /* Allow Subvols and Templates */
  err = FILENAME_SCAN_( fname,
                        fnamelen,
                        &count,
                        &kind,
                        &level,
                        options
                      );

  /* allow kind == 2 (DEFINE names) */
  if (err != 0) return -1;

  if (kind == 1 || (kind == 0 && level < 2)) {
    /* Pattern, Subvol Name or One part Filename - lets see if it exists */
    err = FILENAME_FINDSTART_ ( &searchid,
                                fname,
                                fnamelen,
                                ,
                                DISK_DEVICE
                              );

    if (err != 0) {
      end = FILENAME_FINDFINISH_ ( searchid );
      return -1;
    }

    err = FILENAME_FINDNEXT_ ( searchid,
                               fname,
                               FILENAME_MAX,
                               &fnamelen,
                               info
                              );
    end = FILENAME_FINDFINISH_ ( searchid );

    if (err != 0)
      return -1;  /* Non existing template, subvol or file */

    if (kind == 1 || info[2] == -1) {
      s->st_mode = S_IFDIR;    /* Its an existing template or directory */
      return 0;
    }

    /* Must be a real file so drop to code below to get info on it */
  }

  err = FILE_GETINFOLISTBYNAME_( fname,
                                 fnamelen,
                                 ilist,
                                 flist_items,
                                 flist,
                                 flist_size,
                                 rlen,
                                 err_item
                               );

  if (err != 0) return -1;

  ioff[0] = 0;

  /*  Build up table of offets into result list */
  for (i=1; i < flist_items; i++)
    ioff[i] = ioff[i-1] + ilen[i-1];


  /* Setup timestamps */
  s->st_atime = gmt_to_time_t ((long long *)&flist[ioff[1]]);
  s->st_mtime = s->st_ctime = gmt_to_time_t ((long long *)&flist[ioff[2]]);
  s->st_reserved[0] = (int64_t) gmt_to_time_t ((long long *)&flist[ioff[13]]);

  s->st_size = *(off_t *)&flist[ioff[3]];

  fowner = (unsigned short *)&flist[ioff[4]];
  s->st_uid = *fowner & 0x00ff;
  s->st_gid = *fowner >> 8;

  /* Note that Purge security (fsec[3]) in NSK has no relevance to stat() */
  fsec = (char *)&flist[ioff[0]];
  fprogid = (unsigned short *)&flist[ioff[12]];

  s->st_mode = S_IFREG |  /* Regular File */
  /*  Parse Read Flag */
               ((fsec[0] & 0x03) == 0x00 ? S_IROTH : 0) |
               ((fsec[0] & 0x02) == 0x00 ? S_IRGRP : 0) |
               ((fsec[0] & 0x03) != 0x03 ? S_IRUSR : 0) |
  /*  Parse Write Flag */
               ((fsec[1] & 0x03) == 0x00 ? S_IWOTH : 0) |
               ((fsec[1] & 0x02) == 0x00 ? S_IWGRP : 0) |
               ((fsec[1] & 0x03) != 0x03 ? S_IWUSR : 0) |
  /*  Parse Execute Flag */
               ((fsec[2] & 0x03) == 0x00 ? S_IXOTH : 0) |
               ((fsec[2] & 0x02) == 0x00 ? S_IXGRP : 0) |
               ((fsec[2] & 0x03) != 0x03 ? S_IXUSR : 0) |
  /*  Parse Progid */
               (*fprogid == 1 ? (S_ISUID | S_ISGID) : 0) ;

  return 0;
}



/* TANDEM Directory processing */

DIR *opendir(const char *dirname)
{
   short i, resolve;
   char sname[FILENAME_MAX + 1];
   short snamelen;
   char fname[FILENAME_MAX + 1];
   short fnamelen;
   char *p;
   short searchid, err, end;
   struct dirent *entry;
   DIR *dirp;
   char ext[EXTENSION_MAX + 1];
   short extension;

   extension = parsename(dirname, sname, ext);
   snamelen = strlen(sname);

   /*  First we work out how detailed the template is...
    *  e.g. If the template is DAVES*.* we want the search result
    *       in the same format
    */

   p = sname;
   i = 0;
   while ((p = strchr(p, TANDEM_DELIMITER)) != NULL){
     i++;
     p++;
   };
   resolve = 2 - i;

   /*  Attempt to start a filename template */
   err = FILENAME_FINDSTART_ ( &searchid,
                               sname,
                               snamelen,
                               resolve,
                               DISK_DEVICE
                             );
   if (err != 0) {
     end = FILENAME_FINDFINISH_(searchid);
     return NULL;
   }

   /* Create DIR structure */
   if ((dirp = malloc(sizeof(DIR))) == NULL ) {
     end = FILENAME_FINDFINISH_(searchid);
     return NULL;
   }
   dirp->D_list = dirp->D_curpos = NULL;
   strcpy(dirp->D_path, dirname);

   while ((err = FILENAME_FINDNEXT_(searchid,
                                    fname,
                                    FILENAME_MAX,
                                    &fnamelen
                                   )
           ) == 0 ){
     /*  Create space for entry */
     if ((entry = malloc (sizeof(struct dirent))) == NULL) {
       end = FILENAME_FINDFINISH_(searchid);
       return NULL;
     }

     /*  Link to last entry */
     if (dirp->D_curpos == NULL)
       dirp->D_list = dirp->D_curpos = entry;  /* First name */
     else {
       dirp->D_curpos->d_next = entry;         /* Link */
       dirp->D_curpos = entry;
     };
     /* Add directory entry */
     *dirp->D_curpos->d_name = '\0';
     strncat(dirp->D_curpos->d_name,fname,fnamelen);
     if (extension) {
       strcat(dirp->D_curpos->d_name,TANDEM_EXTENSION_STR);
       strcat(dirp->D_curpos->d_name,ext);
     };
     dirp->D_curpos->d_next = NULL;
   };

   end = FILENAME_FINDFINISH_(searchid);

   if (err == 1) {  /*  Should return EOF at end of search */
     dirp->D_curpos = dirp->D_list;        /* Set current pos to start */
     return dirp;
   } else
     return NULL;
}

struct dirent *readdir(DIR *dirp)
{
   struct dirent *cur;

   cur = dirp->D_curpos;
   dirp->D_curpos = dirp->D_curpos->d_next;
   return cur;
}

void rewinddir(DIR *dirp)
{
   dirp->D_curpos = dirp->D_list;
}

int closedir(DIR *dirp)
{
   struct dirent *node;

   while (dirp->D_list != NULL) {
      node = dirp->D_list;
      dirp->D_list = dirp->D_list->d_next;
      free( node );
   }
   free( dirp );
   return 0;
}


static int created_dir;        /* used in mapname(), checkdir() */
static int renamed_fullpath;   /* ditto */

/**********************/
/* Function do_wild() */  /* for porting:  dir separator; match(ignore_case) */
/**********************/

char *do_wild(__G__ wildspec)
    __GDEF
    char *wildspec;         /* only used first time on a given dir */
{
    static DIR *dir = (DIR *)NULL;
    static char *dirname, *wildname, matchname[FILNAMSIZ];
    static int firstcall=TRUE, have_dirname, dirnamelen;
    struct dirent *file;
    static char *intname;
    int isdir = 0;
    int pdosflag = 0;

    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (firstcall) {        /* first call:  must initialize everything */
        firstcall = FALSE;

        if (!iswild(wildspec)) {
            strcpy(matchname, wildspec);
            have_dirname = FALSE;
            dir = NULL;
            return matchname;
        }

        dirnamelen = strlen(wildspec);

        if ((dirname = (char *)malloc(dirnamelen+1)) == (char *)NULL) {
            Info(slide, 0x201, ((char *)slide,
              "warning:  cannot allocate wildcard buffers\n"));
             strcpy(matchname, wildspec);
             return matchname;   /* but maybe filespec was not a wildcard */
        }
        strcpy(dirname, wildspec);
        wildname = wildspec;
        have_dirname = FALSE;

        if ((dir = opendir(dirname)) != (DIR *)NULL) {
            while ((file = readdir(dir)) != (struct dirent *)NULL) {
                Trace((stderr, "do_wild:  readdir returns %s\n", file->d_name));
                if (file->d_name[0] == '.' && wildname[0] != '.')
                    continue;  /* Unix:  '*' and '?' do not match leading dot */
                if (match(file->d_name, wildname, 0) &&  /* 0 == case sens. */
                    /* skip "." and ".." directory entries */
                    strcmp(file->d_name, ".") && strcmp(file->d_name, "..")) {
                    Trace((stderr, "do_wild:  match() succeeds\n"));
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

    /* If we've gotten this far, we've read and matched at least one entry
     * successfully (in a previous call), so dirname has been copied into
     * matchname already.
     */
    while ((file = readdir(dir)) != (struct dirent *)NULL) {
        Trace((stderr, "do_wild:  readdir returns %s\n", file->d_name));
        if (file->d_name[0] == '.' && wildname[0] != '.')
            continue;   /* Unix:  '*' and '?' do not match leading dot */
        if (match(file->d_name, wildname, 0)) {   /* 0 == don't ignore case */
            if (have_dirname) {
                /* strcpy(matchname, dirname); */
                strcpy(matchname+dirnamelen, file->d_name);
            } else
                strcpy(matchname, file->d_name);
            return matchname;
        }
    }

    closedir(dir);     /* have read at least one dir entry; nothing left */
    dir = (DIR *)NULL;
    firstcall = TRUE;  /* reset for new wildspec */
    if (have_dirname)
        free(dirname);
    return (char *)NULL;

} /* end function do_wild() */


/**********************/
/* Function mapattr() */
/**********************/

int mapattr(__G)
    __GDEF
{
    ulg tmp = G.crec.external_file_attributes;

    switch (G.pInfo->hostnum) {
        case AMIGA_:
            tmp = (unsigned)(tmp>>17 & 7);   /* Amiga RWE bits */
            G.pInfo->file_attr = (unsigned)(tmp<<6 | tmp<<3 | tmp);
            break;
        case UNIX_:
        case VMS_:
        case ACORN_:
        case ATARI_:
        case BEOS_:
        case QDOS_:
        case TANDEM_:
            G.pInfo->file_attr = (unsigned)(tmp >> 16);
            if (G.pInfo->file_attr != 0 || !G.extra_field) {
                return 0;
            } else {
                /* Some (non-Info-ZIP) implementations of Zip for Unix and
                   VMS (and probably others ??) leave 0 in the upper 16-bit
                   part of the external_file_attributes field. Instead, they
                   store file permission attributes in some extra field.
                   As a work-around, we search for the presence of one of
                   these extra fields and fall back to the MSDOS compatible
                   part of external_file_attributes if one of the known
                   e.f. types has been detected.
                   Later, we might implement extraction of the permission
                   bits from the VMS extra field. But for now, the work-around
                   should be sufficient to provide "readable" extracted files.
                   (For ASI Unix e.f., an experimental remap of the e.f.
                   mode value IS already provided!)
                 */
                ush ebID;
                unsigned ebLen;
                uch *ef = G.extra_field;
                unsigned ef_len = G.crec.extra_field_length;
                int r = FALSE;

                while (!r && ef_len >= EB_HEADSIZE) {
                    ebID = makeword(ef);
                    ebLen = (unsigned)makeword(ef+EB_LEN);
                    if (ebLen > (ef_len - EB_HEADSIZE))
                        /* discoverd some e.f. inconsistency! */
                        break;
                    switch (ebID) {
                      case EF_ASIUNIX:
                        if (ebLen >= (EB_ASI_MODE+2)) {
                            G.pInfo->file_attr =
                              (unsigned)makeword(ef+(EB_HEADSIZE+EB_ASI_MODE));
                            /* force stop of loop: */
                            ef_len = (ebLen + EB_HEADSIZE);
                            break;
                        }
                        /* else: fall through! */
                      case EF_PKVMS:
                        /* "found nondecypherable e.f. with perm. attr" */
                        r = TRUE;
                      default:
                        break;
                    }
                    ef_len -= (ebLen + EB_HEADSIZE);
                    ef += (ebLen + EB_HEADSIZE);
                }
                if (!r)
                    return 0;
            }
            /* fall through! */
        /* all remaining cases:  expand MSDOS read-only bit into write perms */
        case FS_FAT_:
        case FS_HPFS_:
        case FS_NTFS_:
        case MAC_:
        case TOPS20_:
        default:
            tmp = !(tmp & 1) << 1;   /* read-only bit --> write perms bits */
            G.pInfo->file_attr = (unsigned)(0444 | tmp<<6 | tmp<<3 | tmp);
            break;
    } /* end switch (host-OS-created-by) */

    /* for originating systems with no concept of "group," "other," "system": */
    umask( (int)(tmp=umask(0)) );    /* apply mask to expanded r/w(/x) perms */
    G.pInfo->file_attr &= ~tmp;

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
    char pathcomp[FILNAMSIZ];      /* path-component buffer */
    char *pp, *cp;                 /* character pointers */
    char *lastsemi=(char *)NULL;   /* pointer to last semi-colon in pathcomp */
    int quote = FALSE;             /* flags */
    int error = 0;
    register unsigned workch;      /* hold the character being tested */


/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    if (G.pInfo->vollabel)
        return IZ_VOL_LABEL;    /* can't set disk volume labels on Tandem */

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!uO.fflag || renamed);

    created_dir = FALSE;        /* not yet */

    /* user gave full pathname:  don't prepend rootpath */
    renamed_fullpath = (renamed && (*G.filename == '/'));

    if (checkdir(__G__ (char *)NULL, INIT) == 10)
        return 10;              /* initialize path buffer, unless no memory */

    /* TANDEM - call in2ex */
    pp = in2ex(G.filename);
    if (pp == (char *)NULL)
        return 10;
    strcpy(G.filename, pp);
    free(pp);

    *pathcomp = '\0';           /* initialize translation buffer */
    pp = pathcomp;              /* point to translation buffer */
    /* directories have already been junked in in2ex() */
    cp = G.filename;            /* point to internal zipfile-member pathname */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {

        if (quote) {                 /* if character quoted, */
            *pp++ = (char)workch;    /*  include it literally */
            quote = FALSE;
        } else
            switch (workch) {
            case TANDEM_DELIMITER: /* can assume -j flag not given */
                *pp = '\0';
                if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                    return error;
                pp = pathcomp;    /* reset conversion buffer for next piece */
                lastsemi = (char *)NULL; /* leave directory semi-colons alone */
                break;

            case ';':             /* VMS version (or DEC-20 attrib?) */
                lastsemi = pp;
                *pp++ = ';';      /* keep for now; remove VMS ";##" */
                break;            /*  later, if requested */

            case '\026':          /* control-V quote for special chars */
                quote = TRUE;     /* set flag for next character */
                break;

            case ' ':             /* remove spaces for Tandem */
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

    if (G.filename[strlen(G.filename) - 1] == TANDEM_DELIMITER) {
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

        if ((end-buildpath) > FILNAMSIZ-3)  /* need '/', one-char name, '\0' */
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
            if (mkdir(buildpath, 0777) == -1) {   /* create the directory */
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
        *end++ = TANDEM_DELIMITER;
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

/* GRR:  for VMS and TOPS-20, add up to 13 to strlen */

    if (FUNCTION == INIT) {
        Trace((stderr, "initializing buildpath to "));
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+1)) ==
            (char *)NULL)
            return 10;
        if ((rootlen > 0) && !renamed_fullpath) {
            strcpy(buildpath, rootpath);
            end = buildpath + rootlen;
        } else {
            *buildpath = '\0';
            end = buildpath;
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
            if (pathcomp[rootlen-1] == TANDEM_DELIMITER) {
                pathcomp[--rootlen] = '\0';
            }
            if (rootlen > 0 && (stat(pathcomp, &G.statbuf) ||
                !S_ISDIR(G.statbuf.st_mode)))        /* path does not exist */
            {
                if (!G.create_dirs /* || iswild(pathcomp) */ ) {
                    rootlen = 0;
                    return 2;   /* skip (or treat as stored file) */
                }
                /* create the directory (could add loop here to scan pathcomp
                 * and create more than one level, but why really necessary?) */
                if (mkdir(pathcomp, 0777) == -1) {
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
            rootpath[rootlen++] = TANDEM_DELIMITER;
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



/********************/
/* Function mkdir() */
/********************/

int mkdir(path, mode)
const char *path;  /* both    */
mode_t mode;       /* ignored */
/*
 * returns:   0 - successful
 *           -1 - failed (errno not set, however)
 */
{
    return 0;
}


/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{

    /* Pyramid, NeXT have problems with huge macro expansion, too: no Info() */
    sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
      "gcc ", __VERSION__,
#else
#  ifdef __VERSION__
      "cc ", __VERSION__,
#  else
      "cc", "",
#  endif
#endif

      "Unix",

#ifdef TANDEM
      " (Tandem/NSK)",
#else
      "",
#endif /* TANDEM */

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    (*G.message)((zvoid *)&G, slide, (ulg)strlen((char *)slide), 0);

} /* end function version() */




/****************************/
/* Function close_outfile() */
/****************************/

void close_outfile(__G)    /* GRR: change to return PK-style warning level */
    __GDEF
{
    iztimes zt;
    ush z_uidgid[2];
    unsigned eb_izux_flg;

    fclose(G.outfile);

/*---------------------------------------------------------------------------
    Convert from MSDOS-format local time and date to Unix-format 32-bit GMT
    time:  adjust base year from 1980 to 1970, do usual conversions from
    yy/mm/dd hh:mm:ss to elapsed seconds, and account for timezone and day-
    light savings time differences.  If we have a Unix extra field, however,
    we're laughing:  both mtime and atime are ours.  On the other hand, we
    then have to check for restoration of UID/GID.
  ---------------------------------------------------------------------------*/

    eb_izux_flg = (G.extra_field ? ef_scan_for_izux(G.extra_field,
                   G.lrec.extra_field_length, 0, G.lrec.last_mod_dos_datetime,
#ifdef IZ_CHECK_TZ
                   (G.tz_is_valid ? &zt : NULL),
#else
                   &zt,
#endif
                   z_uidgid) : 0);
    if (eb_izux_flg & EB_UT_FL_MTIME) {
        TTrace((stderr, "\nclose_outfile:  Unix e.f. modif. time = %ld\n",
          zt.mtime));
    } else {
        zt.mtime = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }
    if (eb_izux_flg & EB_UT_FL_ATIME) {
        TTrace((stderr, "close_outfile:  Unix e.f. access time = %ld\n",
          zt.atime));
    } else {
        zt.atime = zt.mtime;
        TTrace((stderr, "\nclose_outfile:  modification/access times = %ld\n",
          zt.mtime));
    }

/*---------------------------------------------------------------------------
    Change the file's last modified time to that stored in the zipfile.
    Not sure how (yet) or whether its a good idea to set the last open time
  ---------------------------------------------------------------------------*/

    if (utime(G.filename, (ztimbuf *)&zt))
        if (uO.qflag)
            Info(slide, 0x201, ((char *)slide,
              "warning:  cannot set times for %s\n", G.filename));
        else
            Info(slide, 0x201, ((char *)slide,
              " (warning) cannot set times"));

/*---------------------------------------------------------------------------
    Change the file permissions from default ones to those stored in the
    zipfile.
  ---------------------------------------------------------------------------*/

#ifndef NO_CHMOD
    if (chmod(G.filename, 0xffff & G.pInfo->file_attr))
        perror("chmod (file attributes) error");
#endif

/*---------------------------------------------------------------------------
       if -X option was specified and we have UID/GID info, restore it
       this must come after the file security and modtimes changes - since once
       we have secured the file to somebody else we cannot access it again.
  ---------------------------------------------------------------------------*/

    if (uO.X_flag && eb_izux_flg & EB_UX2_VALID) {
        TTrace((stderr, "close_outfile:  restoring Unix UID/GID info\n"));
        if (chown(G.filename, (uid_t)z_uidgid[0], (gid_t)z_uidgid[1]))
        {
            if (uO.qflag)
                Info(slide, 0x201, ((char *)slide,
                  "warning:  cannot set UID %d and/or GID %d for %s\n",
                  z_uidgid[0], z_uidgid[1], G.filename));
            else
                Info(slide, 0x201, ((char *)slide,
                  " (warning) cannot set UID %d and/or GID %d",
                  z_uidgid[0], z_uidgid[1]));
        }
    }

} /* end function close_outfile() */




/********************/    /* swiped from Zip:  convert the zip file name to */
/* Function in2ex() */    /*  an external file name, returning the malloc'd */
/********************/    /*  string or NULL if not enough memory. */

char *in2ex(n)
    char *n;              /* internal file name */
{
    char *x;              /* external file name */
    char *t;              /* pointer to internal */
    char *p;              /* pointer to internal */
    char *e;              /* pointer to internal */

    if ((x = malloc(strlen(n) + 4)) == NULL)   /* + 4 for safety */
        return NULL;
    *x= '\0';

    /* Junk pathname as requested */
    if (uO.jflag && (t = strrchr(n, INTERNAL_DELIMITER)) != NULL)
        ++t;
    else
        t = n;

    while (*t != '\0') {      /* File part could be sys, vol, subvol or file */
        if (*t == INTERNAL_DELIMITER) {     /* System, Volume or Subvol Name */
            t++;
            if (*t == INTERNAL_DELIMITER) { /* System */
                strcat(x, TANDEM_NODE_STR);
                t++;
            } else
                strcat(x, TANDEM_DELIMITER_STR);
        }
        p = strchr(t, INTERNAL_DELIMITER);
        if (p == NULL)
            break;
        if ((e = strchr(t, DOS_EXTENSION)) == NULL)
            e = p;
        else
            e = (e < p ? e : p);
#ifdef CLIP_MAXFNLEN_NSK
        strncat(x, t, _min(MAXFILEPARTLEN, (e - t)));
#else
        strncat(x, t, (e - t));
#endif
        t = p;
    }

    if ((e = strchr(t, DOS_EXTENSION)) == NULL)
        strcat(x, t);
    else {
#ifdef CLIP_MAXFNLEN_NSK
        strncat(x, t, _min(MAXFILEPARTLEN, (e - t)));
#else
        strncat(x, t, (e - t));
#endif
        strcat(x, TANDEM_EXTENSION_STR);
        strcat(x, e+1);
    }

    return x;
}
