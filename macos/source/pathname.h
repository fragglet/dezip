#ifndef PATHNAME_H
#define PATHNAME_H 1


char *StripPartialDir(char *CompletePath,
                      const char *PartialPath, const char *FullPath);

char *Real2RfDfFilen(char *RfDfFilen, const char *RealPath, short CurrentFork,
                     short MacZipMode, Boolean DataForkOnly);
char *RfDfFilen2Real(char *RealFn, const char *RfDfFilen, short MacZipMode,
                     Boolean DataForkOnly, short *CurrentFork);

unsigned short GetVolumeFromPath(const char *FullPath, char *VolumeName);
char *GetCompletePath(char *CompletePath, const char *name, FSSpec *Spec,
                      OSErr *err);
char *TruncFilename(char *CompletePath, const char *name, FSSpec *Spec,
                    OSErr *err);
char *GetFilename(char *CompletePath, const char *name);
char *GetFullPathFromSpec(char *CompletePath, FSSpec *Spec, OSErr *err);
char *GetFullPathFromID(char *CompletePath, short vRefNum, long dirID,
                        ConstStr255Param name, OSErr *err);

char *GetAppName(void);

#define UnKnown_EF      0
#define JohnnyLee_EF    1
#define NewZipMode_EF   2

#define ResourceFork    -1
#define DataFork        1
#define NoFork          0


#ifndef NAME_MAX
#define NAME_MAX    1024
#endif

#endif   /*  PATHNAME_H  */
