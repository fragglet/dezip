/*---------------------------------------------------------------------------

  helpers.c

  Some useful functions Used by unzip and zip.

  ---------------------------------------------------------------------------*/

/*****************************************************************************/
/*  Includes                                                                 */
/*****************************************************************************/

#include "zip.h"
#include <ctype.h>
#include <time.h>

#include "macstuff.h"
#include "helpers.h"
#include "pathname.h"


/*****************************************************************************/
/*  Global Vars                                                              */
/*****************************************************************************/


extern int noisy;
extern char MacPathEnd;
extern char *zipfile;   /* filename of the Zipfile */
extern char *tempzip;   /* Temporary zip file name */


static char         argStr[1024];
static char         *argv[MAX_ARGS + 1];


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/


/*****************************************************************************/
/*  Macros, typedefs                                                         */
/*****************************************************************************/


  /*
   * ARGH.  Mac times are based on 1904 Jan 1 00:00, not 1970 Jan 1 00:00.
   *  So we have to diddle time_t's appropriately:  add or subtract 66 years'
   *  worth of seconds == number of days times 86400 == (66*365 regular days +
   *  17 leap days ) * 86400 == (24090 + 17) * 86400 == 2082844800L seconds.
   *  We hope time_t is an unsigned long (ulg) on the Macintosh...
   */
/*
This Offset is only used by MacFileDate_to_UTime()
*/

#define NATIVE_TO_STATS(x)  (x) -= (unsigned long)2082844800L


/*****************************************************************************/
/*  Functions                                                                */
/*****************************************************************************/


/*
**  Copy a C string to a Pascal string
**
*/
unsigned char *CToPCpy(unsigned char *pstr, char *cstr)
{
    register char *dptr;
    register unsigned len;

        len=0;
        dptr=(char *)pstr+1;
    while (len<255 && (*dptr++ = *cstr++)!='\0') ++len;
    *pstr= (unsigned char)len;
  return pstr;
}



/*
**  Copy a Pascal string to a C string
**
*/

char *PToCCpy(unsigned char *pstr, char *cstr)
{
strncpy(cstr, (char *) &pstr[1], *pstr);
    cstr[pstr[0]] = '\0';  /* set endmarker for c-string */
return cstr;
}




/*
**  Alloc memory and init it
**
*/
char *StrCalloc(unsigned short size)
{
char *strPtr = NULL;

if ((strPtr = calloc(size, sizeof(char))) == NULL)
    printerr("StrCalloc failed:", -1, size, __LINE__, __FILE__, "");

return strPtr;
}



/*
**  Release only non NULL pointers
**
*/
char *StrFree(char *strPtr)
{
if (strPtr != NULL)
    {
    free(strPtr);
    }

return NULL;
}




/*
**  Return a value in a binary string
**
*/

char *sBit2Str(unsigned short value)
{
short pos = 0;
static char str[sizeof(value)*8];

memset(str, '0', sizeof(value)*8);  /* set string-buffer */

for (pos = sizeof(value)*8; pos != 0; value >>= 1)
    {
    if (value & 01)
        str[pos] = '1';
    else str[pos] = '0';
    pos--;
    }

str[(sizeof(value)*8)+1] = '\0';
return str;
}



/*
**  Parse commandline style arguments
**
*/

int ParseArguments(char *s, char ***arg)
{
    int  n = 1, Quote = 0;
    char *p = s, *p1, c;

    argv[0] = GetAppName();

    *arg = argv;

    p1 = (char *) argStr;
    while ((c = *p++) != 0) {
        if (c==' ') continue;
        argv[n++] = p1;
        if (n > MAX_ARGS)               /* mm 970404 */
            return (n-1);               /* mm 970404 */
        do {
            if (c=='\\' && *p++)
                c = *p++;
            else
                if ((c=='"') || (c == '\'')) {
                    if (!Quote) {
                        Quote = c;
                        continue;
                    }
                    if (c == Quote) {
                        Quote = 0;
                        continue;
                    }
                }
            *p1++ = c;
        } while (*p && ((c = *p++) != ' ' || Quote));
        *p1++ = '\0';
    }
    return n;
}



/*
**  Print commandline style arguments
**
*/

void PrintArguments(int argc, char **argv)
{

printf("\n Arguments:");
printf("\n --------------------------");

while(--argc >= 0)
    printf("\n argc: %d  argv: [%s]", argc, &*argv[argc]);

printf("\n --------------------------\n\n");
return;
}



/*
**  return some error-msg on file-system
**
*/

int PrintUserHFSerr(int cond, int err, char *msg2)
{
char *msg;

if (cond != 0)
    {
    switch (err)
        {
         case -35:
            msg = "No such Volume";
         break;

         case -56:
            msg = "No such Drive";
         break;

         case -37:
            msg = "Bad Volume Name";
         break;

         case -49:
            msg = "File is already open for writing";
         break;

         case -43:
            msg = "Directory/File not found";
         break;

         case -120:
            msg = "Directory/File not found or incomplete pathname";
         break;

        default: return err;
         }
    fprintf(stderr, "\n\n Error: %s ->%s", msg, msg2);
    exit(err);
    }

return 0;
}



/*
**  Check mounted volumes and return number of volumes
**  with the same name.
*/

short CheckMountedVolumes(char *FullPath)
{
FSSpec  volumes[50];        /* 50 Volumes should be enough */
char VolumeName[257], volume[257];
short actVolCount, volIndex = 1, VolCount = 0;
OSErr err;
int i;

GetVolumeFromPath(FullPath, VolumeName);

err = OnLine(volumes, 50, &actVolCount, &volIndex);
printerr("OnLine:", (err != -35) && (err != 0), err, __LINE__, __FILE__, "");

for (i=0; i < actVolCount; i++)
    {
    PToCCpy(volumes[i].name,volume);
    if (stricmp(volume, VolumeName) == 0) VolCount++;
    }
printerr("OnLine: ", (VolCount == 0), VolCount, __LINE__, __FILE__, FullPath);

return VolCount;
}








/*
**  compares strings, ignoring differences in case
**
*/

int stricmp(const char *p1, const char *p2)
{
int diff;

while (*p1 && *p2)
    {
    if (*p1 != *p2)
        {
        if (isalpha(*p1) && isalpha(*p2))
            {
            diff = toupper(*p1) - toupper(*p2);
            if (diff) return diff;
            }
            else break;
        }
        p1++;
        p2++;
    }
return *p1 - *p2;
}



/*
**  creates an archive file name
**
*/

void createArchiveName(char *thePath)
{
char *tmpPtr;
short folderCount = 0;
char name[256];
unsigned short pathlen = strlen(thePath);

for (tmpPtr = thePath; *tmpPtr; tmpPtr++)
    if (*tmpPtr == ':') folderCount++;

if (folderCount > 1) { /* path contains at least one folder */
    if (pathlen <= 30) thePath[pathlen-2] = 0x0;
    else thePath[pathlen-4] = 0x0;
	strcat(thePath,".zip");
} else {  /* path contains no folder */
    strcpy(name, thePath);
    FindDesktopFolder(thePath);
    if (pathlen <= 30) name[pathlen-2] = 0x0;
    else name[pathlen-4] = 0x0;
	strcat(thePath,name);
	strcat(thePath,".zip");
}
}



/*
** finds the desktop-folder on a volume with
** largest amount of free-space.
*/

void FindDesktopFolder(char *Path)
{
FSSpec  volumes[50];        /* 50 Volumes should be enough */
short   actVolCount, volIndex = 1, VolCount = 0;
OSErr   err;
short     i, foundVRefNum;
FSSpec spec;
UnsignedWide freeBytes;
UnsignedWide totalBytes;
UnsignedWide MaxFreeBytes;

err = OnLine(volumes, 50, &actVolCount, &volIndex);
printerr("OnLine:", (err != -35) && (err != 0), err, __LINE__, __FILE__, "");

MaxFreeBytes.hi = 0;
MaxFreeBytes.lo = 0;

for (i=0; i < actVolCount; i++)
    {
    XGetVInfo(volumes[i].vRefNum,
              volumes[i].name,
			  &volumes[i].vRefNum,
			  &freeBytes,
			  &totalBytes);

	if (MaxFreeBytes.hi < freeBytes.hi) {
		MaxFreeBytes.hi = freeBytes.hi;
		MaxFreeBytes.lo = freeBytes.lo;
		foundVRefNum = volumes[i].vRefNum;
	printf("\n1 Path: %s \n", Path);
	}

	if ((freeBytes.hi == 0) && (MaxFreeBytes.lo < freeBytes.lo)) {
		MaxFreeBytes.hi = freeBytes.hi;
		MaxFreeBytes.lo = freeBytes.lo;
		foundVRefNum = volumes[i].vRefNum;
	}

}

 FSpFindFolder(foundVRefNum, kDesktopFolderType,
            kDontCreateFolder,&spec);

 GetFullPathFromSpec(Path, &spec , &err);
}



#if (defined(USE_SIOUX) || defined(MACUNZIP_STANDALONE))

/*
**  checks the condition and returns an error-msg
**  this function is for internal use only
*/

OSErr printerr(const char *msg, int cond, int err, int line, char *file,
              const char *msg2)
{

if (cond != 0)
    {
    fprintf(stderr, "\nint err: %d: %s %d [%d/%s] {%s}\n", clock(), msg, err,
            line, file, msg2);
    }

return cond;
}


/*
fake-functions:
Not Implemented for metrowerks SIOUX
*/

void leftStatusString(char *status)
{
status = status;
}


void rightStatusString(char *status)
{
status = status;
}



void DoWarnUserDupVol( char *FullPath )
{
  char VolName[257];
  GetVolumeFromPath(FullPath,  VolName);

  printf("\n There are more than one volume that has the same name !!\n");

  printf("\n Volume: %s\n",VolName);

  printf("\n This port has one weak point:");
  printf("\n It is based on pathnames. As you may be already know:");
  printf("\n Pathnames are not unique on a Mac !");
  printf("\n MacZip has problems to find the correct location of");
  printf("\n the archive or the files.\n");

  printf("\n My (Big) recommendation:  Name all your volumes with an");
  printf("\n unique name and MacZip will run without any problem.");
}



#endif
