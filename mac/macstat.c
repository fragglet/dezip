#ifdef THINK_C
#define MACOS
#include    <FileMgr.h>
#include    <HFS.h>
#include    <pascal.h>
#endif
#ifdef MPW
#define MACOS
#include    <Files.h>
#include    <Errors.h>
#define FSFCBLen    (*(short *)0x3F6)
#define hFileInfo   hfileInfo
#define CtoPstr c2pstr
#define PtoCstr p2cstr
#endif

#ifdef MACOS
#include    <string.h>
#include    "macstat.h"
int stat(char *path, struct stat *buf);

/* assume that the path will contain a Mac-type pathname, i.e. ':'s, etc. */
int stat(path, buf)
char *path;
struct stat *buf;
{
    char    temp[256];
    short   curVolume;
    long    curDir;
    short   fIsHFS = false;
    OSErr   err;

    if (buf == (struct stat *)0L || path == (char *)0L) {
        SysBeep(1);
        return -1;
    }
    
    if (path[0] == '\0' || strlen(path)>255) {
        return -1;
    }

    if (GetVol((StringPtr)&temp[0], &curVolume) != noErr) {
        SysBeep(1);
        return -1;
    }
    
    /* get info about the specified volume */
    if (FSFCBLen > 0)   /* HFS Disk? */
    {
        WDPBRec wdpb;
        HParamBlockRec    hpbr;
        Str255 st;

        wdpb.ioCompletion = 0;
        wdpb.ioNamePtr = temp;
        err = PBHGetVol(&wdpb, 0);
        if (err == noErr)
        {
            hpbr.volumeParam.ioCompletion = 0;
            hpbr.volumeParam.ioNamePtr = st;
            hpbr.volumeParam.ioVRefNum = wdpb.ioVRefNum;
            hpbr.volumeParam.ioVolIndex = 0;
            err = PBHGetVInfo(&hpbr, 0);

            if (err == noErr && hpbr.volumeParam.ioVFSID == 0
                && hpbr.volumeParam.ioVSigWord == 0x4244) {
                    fIsHFS = true;
            }
        }
    }


    /* number of links, at least in System 6.0x, 0 */
    buf->st_nlink = 0;
    /* user id */
    buf->st_uid = 0;
    /* group id */
    buf->st_gid = 0;

    if (fIsHFS == true)   /* HFS? */
    {
        CInfoPBRec  cPB;
        HParamBlockRec  wPB;
        
        wPB.wdParam.ioCompletion = (ProcPtr)0L;
        wPB.wdParam.ioNamePtr = (StringPtr)temp;
        err = PBHGetVol((WDPBPtr)&wPB, false);
        if (err != noErr) {
            SysBeep(1);
            return -1;
        }

        curVolume = wPB.wdParam.ioWDVRefNum;
        buf->st_dev = curDir = wPB.wdParam.ioWDDirID;
        
        /* get information about file */
        cPB.hFileInfo.ioCompletion = (ProcPtr)0L;
        CtoPstr(path);
        strncpy(temp,path, path[0]+1);
        PtoCstr(path);
        cPB.hFileInfo.ioNamePtr = (StringPtr)temp;
        cPB.hFileInfo.ioVRefNum = 0;
        cPB.hFileInfo.ioDirID = 0;
        cPB.hFileInfo.ioFDirIndex = 0;
        
        err = PBGetCatInfo(&cPB, false); 
        
        if (err != noErr) {
            if (err != fnfErr) {
                SysBeep(1);
            }
            return -1;
        }
        
        /* Type of file: directory or regular file */
        buf->st_mode = (cPB.hFileInfo.ioFlAttrib & ioDirMask) ? S_IFDIR : S_IFREG;
        
        /* last access time, modification time and creation time(?) */
        buf->st_atime = buf->st_mtime = cPB.hFileInfo.ioFlMdDat;
        buf->st_ctime = cPB.hFileInfo.ioFlCrDat;
        /* inode number */
        buf->st_ino = cPB.hFileInfo.ioDirID + ((long)curVolume)<<16;
        /* size of file - use only the data fork */
        buf->st_size = cPB.hFileInfo.ioFlLgLen;
        /* size of disk block */
        if (cPB.hFileInfo.ioFlClpSiz == 0) {
            HParamBlockRec hPB;
            
            hPB.volumeParam.ioCompletion = (ProcPtr)0L;
            hPB.volumeParam.ioNamePtr = (StringPtr)temp;
            hPB.volumeParam.ioVRefNum = 0;
            hPB.volumeParam.ioVolIndex = 0;
            
            err = PBHGetVInfo(&hPB, false);
            
            if (err != noErr) {
                SysBeep(1);
                return -1;
            }
            
            buf->st_blksize = hPB.volumeParam.ioVClpSiz;
        }
        else
            buf->st_blksize = cPB.hFileInfo.ioFlClpSiz;
    
        buf->st_blocks = cPB.hFileInfo.ioFlPyLen/ buf->st_blksize;
    }
    else    /* MFS? */
    {
        ParamBlockRec   pPB;
        
        buf->st_dev = 0;
        
        CtoPstr(path);
        strncpy(temp, path, path[0]+1);
        PtoCstr(path);
        pPB.fileParam.ioCompletion = (ProcPtr)0;
        pPB.fileParam.ioNamePtr = (StringPtr)temp;
        pPB.fileParam.ioVRefNum = curVolume;
        pPB.fileParam.ioFVersNum = 0;
        pPB.fileParam.ioFDirIndex = 0;
        
        err = PBGetFInfo(&pPB, false);   
        
        if (err != noErr) {
            SysBeep(1);
            return -1;
        }
        
        /* Type of file: either directory or regular file */
        buf->st_mode = (pPB.fileParam.ioFlAttrib & ioDirMask) ? S_IFDIR : S_IFREG;
        
        /* last access time, modification time and creation time(?) */
        buf->st_atime = buf->st_mtime = pPB.fileParam.ioFlMdDat;
        buf->st_ctime = pPB.fileParam.ioFlCrDat;
        /* inode number */
        buf->st_ino = pPB.fileParam.ioFlNum + ((long)curVolume)<<16;
        /* size of file - use only the data fork */
        buf->st_size = pPB.fileParam.ioFlLgLen;
        /* size of disk block */
        {
            ParamBlockRec hPB;
            
            hPB.volumeParam.ioCompletion = (ProcPtr)0;
            hPB.volumeParam.ioNamePtr = (StringPtr)temp;
            hPB.volumeParam.ioVRefNum = curVolume;
            hPB.volumeParam.ioVolIndex = 0;
            
            err = PBGetVInfo(&hPB, false);
            
            if (err != noErr) {
                SysBeep(1);
                return -1;
            }
            
            buf->st_blksize = hPB.volumeParam.ioVClpSiz;
        }
    
        /* number of disk blocks used by file - includes resource fork */
        buf->st_blocks = pPB.fileParam.ioFlPyLen/ buf->st_blksize +
                            pPB.fileParam.ioFlRPyLen/buf->st_blksize;
    }

    return 0;
}
#else
#error 1
#endif