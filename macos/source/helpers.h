#ifndef HELPERS_H
#define HELPERS_H       1

 /* Convert a C string to a Pascal string */
unsigned char *CToPCpy(unsigned char *pstr, char *cstr);

 /* Convert a Pascal string to a C string */
char *PToCCpy(unsigned char *pstr, char *cstr);


char *StrCalloc(unsigned short size);
char *StrFree(char *strPtr);

char *sBit2Str(unsigned short value);

void print_extra_info(void);

int ParseArguments(char *s, char ***arg);
void PrintArguments(int argc, char **argv);

Boolean IsZipFile(char *name);
OSErr printerr(const char *msg, int cond, int err, int line, char *file,
               const char *msg2);
int PrintUserHFSerr(int cond, int err, char *msg2);

short CheckMountedVolumes(char *FullPath);
void DoWarnUserDupVol(char *path);

void PrintFileInfo(void);

int stricmp(const char *p1, const char *p2);
void leftStatusString(char *status);
void rightStatusString(char *status);

Boolean isZipFile(FSSpec *fileToOpen);
void createArchiveName(char *Path);
void FindDesktopFolder(char *Path);

unsigned long MacFileDate_to_UTime(unsigned long mactime);

#define     MAX_ARGS    25

#endif   /*  HELPERS_H   */
