# Microsoft Developer Studio Project File - Name="unzip32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=unzip32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unzip32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unzip32.mak" CFG="unzip32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unzip32 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "unzip32 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unzip32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "E:\WIZ\UNZIP" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DLL" /D "API" /D "SFX" /D "USE_EF_UT_TIME" /D "REENTRANT" /D "WINDLL" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Release\unzip32sfx.lib"

!ELSEIF  "$(CFG)" == "unzip32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "E:\WIZ\UNZIP" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DLL" /D "API" /D "SFX" /D "USE_EF_UT_TIME" /D "REENTRANT" /D "WINDLL" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Debug\unzip32sfx.lib"

!ENDIF 

# Begin Target

# Name "unzip32 - Win32 Release"
# Name "unzip32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\api.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\crc32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\crctab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\explode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\extract.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\fileio.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\globals.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\inflate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\match.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\win32\nt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\process.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\windll\unzipsfx.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\win32\win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\windll\windll.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Wiz\unzip\windll\windll.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
