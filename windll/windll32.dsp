# Microsoft Developer Studio Project File - Name="unzip32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=unzip32 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "windll32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "windll32.mak" CFG="unzip32 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unzip32 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "unzip32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unzip32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\unzip32\Release"
# PROP BASE Intermediate_Dir ".\unzip32\Release"
# PROP BASE Target_Dir ".\unzip32"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\msdev\projects\wiz\release\exe"
# PROP Intermediate_Dir "c:\msdev\projects\wiz\release\unzip"
# PROP Target_Dir ".\unzip32"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "c:\unzip" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINDLL" /D "DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "unzip32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\unzip32\Debug"
# PROP BASE Intermediate_Dir ".\unzip32\Debug"
# PROP BASE Target_Dir ".\unzip32"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "c:\msdev\projects\wiz\debug\exe"
# PROP Intermediate_Dir "c:\msdev\projects\wiz\debug\unzip"
# PROP Target_Dir ".\unzip32"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "c:\unzip" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINDLL" /D "DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386

!ENDIF 

# Begin Target

# Name "unzip32 - Win32 Release"
# Name "unzip32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\api.c
# End Source File
# Begin Source File

SOURCE=..\crc32.c
# End Source File
# Begin Source File

SOURCE=..\crctab.c
# End Source File
# Begin Source File

SOURCE=..\crypt.c
# End Source File
# Begin Source File

SOURCE=.\dllsetup.c
# End Source File
# Begin Source File

SOURCE=..\explode.c
# End Source File
# Begin Source File

SOURCE=..\extract.c
# End Source File
# Begin Source File

SOURCE=..\fileio.c
# End Source File
# Begin Source File

SOURCE=..\globals.c
# End Source File
# Begin Source File

SOURCE=..\inflate.c
# End Source File
# Begin Source File

SOURCE=..\list.c
# End Source File
# Begin Source File

SOURCE=..\match.c
# End Source File
# Begin Source File

SOURCE=..\Win32\nt.c
# End Source File
# Begin Source File

SOURCE=..\process.c
# End Source File
# Begin Source File

SOURCE=..\ttyio.c
# End Source File
# Begin Source File

SOURCE=..\unreduce.c
# End Source File
# Begin Source File

SOURCE=..\unshrink.c
# End Source File
# Begin Source File

SOURCE=..\Win32\win32.c
# End Source File
# Begin Source File

SOURCE=.\windll.c
# End Source File
# Begin Source File

SOURCE=.\windll.rc
# End Source File
# Begin Source File

SOURCE=.\windll32.def
# End Source File
# Begin Source File

SOURCE=..\zipinfo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\crypt.h
# End Source File
# Begin Source File

SOURCE=..\globals.h
# End Source File
# Begin Source File

SOURCE=..\inflate.h
# End Source File
# Begin Source File

SOURCE=..\win32\nt.h
# End Source File
# Begin Source File

SOURCE=.\structs.h
# End Source File
# Begin Source File

SOURCE=..\ttyio.h
# End Source File
# Begin Source File

SOURCE=.\windll.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
