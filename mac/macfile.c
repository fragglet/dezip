/*---------------------------------------------------------------------------

  mac.c

  This source file is used by the mac port to support commands not available
  directly on the Mac, i.e. mkdir().
  It also helps determine if we're running on a Mac with HFS and a disk
  formatted for HFS (HFS - Hierarchical File System; compared to its predecessor,
  MFS - Macintosh File System).
  
  ---------------------------------------------------------------------------*/

#include "unzip.h"

#ifdef MACOS
#ifndef THINK_C
#define FSFCBLen    (*(short *)0x3F6)
#define CtoPstr     c2pstr
#define PtoCstr     p2cstr
#endif

static short wAppVRefNum;
static long lAppDirID;
int hfsflag;            /* set if disk has hierarchical file system */

static int IsHFSDisk(short wRefNum)
{
    /* get info about the specified volume */
    if (hfsflag == true) {
        HParamBlockRec    hpbr;
        Str255 temp;
        short wErr;
        
        hpbr.volumeParam.ioCompletion = 0;
        hpbr.volumeParam.ioNamePtr = temp;
        hpbr.volumeParam.ioVRefNum = wRefNum;
        hpbr.volumeParam.ioVolIndex = 0;
        wErr = PBHGetVInfo(&hpbr, 0);

        if (wErr == noErr && hpbr.volumeParam.ioVFSID == 0
            && hpbr.volumeParam.ioVSigWord == 0x4244) {
                return true;
        }
    }

    return false;
} /* IsHFSDisk */

void macfstest(int vrefnum)
{
    Str255 st;

    /* is this machine running HFS file system? */
    if (FSFCBLen <= 0) {
        hfsflag = false;
    }
    else
    {
        hfsflag = true;
    }

    /* get the file's volume reference number and directory ID */
    if (hfsflag == true) {
        WDPBRec    wdpb;
        OSErr err = noErr;

        if (vrefnum != 0) {
            wdpb.ioCompletion = false;
            wdpb.ioNamePtr = st;
            wdpb.ioWDIndex = 0;
            wdpb.ioVRefNum = vrefnum;
            err = PBHGetVol(&wdpb, false);
        
            if (err == noErr) {
                wAppVRefNum = wdpb.ioWDVRefNum;
                lAppDirID = wdpb.ioWDDirID;
            }
        }

        /* is the disk we're using formatted for HFS? */
        hfsflag = IsHFSDisk(wAppVRefNum);
    }
} /* mactest */

int mkdir(char *path)
{
    OSErr    err = -1;

    if (path != 0 && strlen(path)<256 && hfsflag == true) {
        HParamBlockRec    hpbr;
        Str255    st;
        short     wVol;
        long      lDirID;

        CtoPstr(path);
        hpbr.fileParam.ioNamePtr = st;
        hpbr.fileParam.ioCompletion = NULL;
        err = PBHGetVol((WDPBPtr)&hpbr, false);
        if (err == noErr) {
            wVol = hpbr.wdParam.ioWDVRefNum;
            lDirID = hpbr.wdParam.ioWDDirID;
            hpbr.fileParam.ioCompletion = NULL;
            hpbr.fileParam.ioVRefNum = wVol;
            hpbr.fileParam.ioDirID = lDirID;
            hpbr.fileParam.ioNamePtr = (StringPtr)path;
            err = PBDirCreate(&hpbr, false);
        }    
        PtoCstr(path);
    }

    return (int)err;
} /* mkdir */

void SetMacVol(char *pch, short wVRefNum)
{
    OSErr err = -1;

    if (hfsflag == true) {
        HParamBlockRec  hpbr;
        Str255  st;

        hpbr.wdParam.ioCompletion = NULL;
        hpbr.wdParam.ioNamePtr = st;
        hpbr.wdParam.ioVRefNum = wVRefNum;
        hpbr.wdParam.ioWDIndex = 0;
        hpbr.wdParam.ioWDProcID = 0;
        hpbr.wdParam.ioWDVRefNum = 0;
        err = PBGetWDInfo((WDPBPtr)&hpbr, false);
        if (err == noErr) {
            hpbr.wdParam.ioCompletion = NULL;
            hpbr.wdParam.ioNamePtr = NULL;
            err = PBHSetVol((WDPBPtr)&hpbr, false);
        }
    } else {
        err = SetVol((StringPtr)pch, wVRefNum);
    }
} /* SetMacVol */
#endif /* MACOS */
