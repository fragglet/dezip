/*---------------------------------------------------------------------------

  pathname.c

  Function dealing with the pathname. Mostly C-string work.

  ---------------------------------------------------------------------------*/

/*****************************************************************************/
/*  Includes                                                                 */
/*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "pathname.h"
#include "helpers.h"
#include "macstuff.h"


/*****************************************************************************/
/*  Global Vars                                                              */
/*****************************************************************************/

const char  ResourceMark[] = "XtraStuf.mac:";  /* see also macos.c */


#include "zip.h"




/*****************************************************************************/
/*  Functions                                                                */
/*****************************************************************************/




/*
**  return volumename from pathname
**
*/

unsigned short GetVolumeFromPath(const char *FullPath, char *VolumeName)
{
const char *VolEnd, *tmpPtr1;
char *tmpPtr2 = VolumeName;

for (VolEnd = FullPath; *VolEnd != '\0' && *VolEnd != ':'; VolEnd++)
      ;
if (*VolEnd == '\0') return 0;

for (tmpPtr1 = FullPath; tmpPtr1 != VolEnd;)
    {
    *tmpPtr2++ = *tmpPtr1++;
    }

*tmpPtr2 = '\0';

return (unsigned short) strlen(VolumeName);
}



/*
**  return the path without the filename
**
*/

char *TruncFilename(char *CompletePath, const char *name, FSSpec *Spec,
                    OSErr *err)
{
char *tmpPtr;
char *dirPtr;
short fullPathLength = 0;

strcpy(CompletePath, name);

for (tmpPtr = CompletePath; *tmpPtr; tmpPtr++)
    if (*tmpPtr == ':') dirPtr = tmpPtr;

*++dirPtr = '\0';

fullPathLength = strlen(CompletePath);
printerr("Warning path length exceeds limit: ", fullPathLength >= NAME_MAX,
         fullPathLength, __LINE__, __FILE__, " chars ");

*err = FSpLocationFromFullPath(fullPathLength, CompletePath, Spec);

return CompletePath;
}



/*
**  return only filename
**
*/

char *GetFilename(char *CompletePath, const char *name)
{
const char *tmpPtr;
const char *dirPtr = NULL;


for (tmpPtr = name; *tmpPtr; tmpPtr++)
    if (*tmpPtr == ':') dirPtr = tmpPtr;

return strcpy(CompletePath, (dirPtr == NULL ? name : ++dirPtr));
}



/*
**  return fullpathname from folder/dir-id
**
*/

char *GetFullPathFromID(char *CompletePath, short vRefNum, long dirID,
                        ConstStr255Param name, OSErr *err)
{
FSSpec      spec;

    *err = FSMakeFSSpecCompat(vRefNum, dirID, name, &spec);
    printerr("FSMakeFSSpecCompat:", (*err != -43) && (*err != 0), *err,
             __LINE__, __FILE__, "");
    if ( (*err == noErr) || (*err == fnfErr) )
    {
        return GetFullPathFromSpec(CompletePath, &spec, err);
    }

return NULL;
}



/*
**  convert real-filename to archive-filename
**
*/

char *Real2RfDfFilen(char *RfDfFilen, const char *RealPath,
                    short CurrentFork, short MacZipMode, Boolean DataForkOnly)
{

if (DataForkOnly) /* make no changes */
    {
    return strcpy(RfDfFilen, RealPath);
    }

switch (MacZipMode)
    {
    case JohnnyLee_EF:
        {
        strcpy(RfDfFilen, RealPath);
        if (CurrentFork == DataFork)            /* data-fork  */
            return strcat(RfDfFilen, "d");
        if (CurrentFork == ResourceFork)        /* resource-fork */
            return strcat(RfDfFilen, "r");
        break;
        }

    case NewZipMode_EF:
        {
        switch (CurrentFork)
            {
            case DataFork:
                {
                strcpy(RfDfFilen, RealPath);
                return RfDfFilen;  /* data-fork  */
                break;
                }
            case ResourceFork:
                {
                strcpy(RfDfFilen, ResourceMark);
                strcat(RfDfFilen, RealPath);  /* resource-fork */
                return RfDfFilen;
                break;
                }
            default:
                {
                printerr("Real2RfDfFilen:", -1, -1,
                         __LINE__, __FILE__, RealPath);
                return NULL;  /* function should never reach this point */
                }
            }
        break;
        }
    default:
        {
        printerr("Real2RfDfFilen:", -1, -1, __LINE__, __FILE__, RealPath);
        return NULL;  /* function should never reach this point */
        }
    }

printerr("Real2RfDfFilen:", -1, -1, __LINE__, __FILE__, RealPath);
return NULL;  /* function should never come reach this point */
}



/*
**  convert archive-filename into a real filename
**
*/

char *RfDfFilen2Real(char *RealFn, const char *RfDfFilen, short MacZipMode,
                     Boolean DataForkOnly, short *CurrentFork)
{
short   length;
int     result;

if (DataForkOnly)
    {
    *CurrentFork = DataFork;
    return strcpy(RealFn,RfDfFilen);
    }

switch (MacZipMode)
    {
    case JohnnyLee_EF:
        {
        strcpy(RealFn, RfDfFilen);
        length = strlen(RealFn);       /* determine Fork type */
        if (RealFn[length-1] == 'd') *CurrentFork = DataFork;
        else *CurrentFork = ResourceFork;
        RealFn[length-1] = '\0';       /* simply cut one char  */
        return RealFn;
        break;
        }

    case NewZipMode_EF:
        {                                   /* determine Fork type */
        result = strncmp(RfDfFilen, ResourceMark, sizeof(ResourceMark)-2);
        if (result != 0)
            {
            *CurrentFork = DataFork;
            strcpy(RealFn, RfDfFilen);
            return RealFn;  /* data-fork  */
            }
        else
            {
            *CurrentFork = ResourceFork;
            if (strlen(RfDfFilen) > (sizeof(ResourceMark) - 1))
                {
                strcpy(RealFn, &RfDfFilen[sizeof(ResourceMark)-1]);
                }
            else RealFn[0] = '\0';
            return RealFn;  /* resource-fork */
            }
        break;
        }
    default:
        {
        printerr("RfDfFilen2Real():", -1, MacZipMode,
                 __LINE__, __FILE__, RfDfFilen);
        return NULL;  /* function should never reach this point */
        }
    }

printerr("RfDfFilen2Real():", -1, MacZipMode, __LINE__, __FILE__, RfDfFilen);
return NULL;  /* function should never reach this point */
}



/*
**  return the applications name (argv[0])
**
*/

char *GetAppName(void)
{
ProcessSerialNumber psn;
static Str255       AppName;
ProcessInfoRec      pinfo;
OSErr               err;

GetCurrentProcess(&psn);
pinfo.processName = AppName;
pinfo.processInfoLength = sizeof(pinfo);
pinfo.processAppSpec = NULL;

err = GetProcessInformation(&psn,&pinfo);
AppName[AppName[0]+1] = 0x00;

return (char *)&AppName[1];
}



/*
**  return fullpathname from FSSpec
**
*/

char *GetFullPathFromSpec(char *CompletePath, FSSpec *Spec, OSErr *err)
{
Handle hFullPath;
short len;

memset(CompletePath, 0, sizeof(CompletePath));
*err = 0;
*err = FSpGetFullPath(Spec, &len, &hFullPath);
printerr("FSpGetFullPath:", (*err != -43) && (*err != 0), *err,
         __LINE__, __FILE__, "");
strncpy(CompletePath, *hFullPath, len);
DisposeHandle(hFullPath);   /* we don't need it any more */

CompletePath[len] = '\0';  /* make c-string */
printerr("Warning path length exceeds limit: ", len >= NAME_MAX, len,
         __LINE__, __FILE__, " chars ");

return CompletePath;
}




/*
* This function expands a given partial path to a complete path.
* Path expansions are relative to the running app.
* This function follows the notation:
*   1. relative path:
*       a: ":subfolder:filename"    -> ":current folder:subfolder:filename"
*       b: "::folder2:filename"     -> folder2 is beside the current
*                                      folder on the same level
*       c: "filename"               -> in current folder
*
* An absolute path will be returned.

The following characteristics of Macintosh pathnames should be noted:

       A full pathname never begins with a colon, but must contain at
       least one colon.
       A partial pathname always begins with a colon separator except in
       the case where the file partial pathname is a simple file or
       directory name.
       Single trailing separator colons in full or partial pathnames are
       ignored except in the case of full pathnames to volumes.
       In full pathnames to volumes, the trailing separator colon is required.
       Consecutive separator colons can be used to ascend a level from a
       directory to its parent directory. Two consecutive separator colons
       will ascend one level, three consecutive separator colons will ascend
       two levels, and so on. Ascending can only occur from a directory;
       not a file.
*/

char *GetCompletePath(char *CompletePath, const char *name, FSSpec *Spec,
                      OSErr *err)
{
Boolean hasDirName = false;
char *tmpPtr;

for (tmpPtr = name; *tmpPtr; tmpPtr++)
    if (*tmpPtr == ':') hasDirName = true;


if (name[0] != ':')   /* case c: path including volume name or only filename */
    {
    if (hasDirName)
        {   /* okey, starts with volume name, so it must be a complete path */
        strcpy(CompletePath, name);
        }
    else
        {   /* only filename: add cwd and return */
        getcwd(CompletePath, NAME_MAX);
        strcat(CompletePath, name);
        }
    }
else if (name[1] == ':')    /* it's case b: */
    {
            /* it's not yet implemented; do we really need this case ?*/
    return NULL;
    }
else                        /* it's case a: */
    {
    getcwd(CompletePath, NAME_MAX);     /* we don't need a second colon */
    CompletePath[strlen(CompletePath)-1] = '\0';
    strcat(CompletePath, name);
    }

*err = FSpLocationFromFullPath((short)strlen(CompletePath),
                               CompletePath, Spec);
return CompletePath;
}
