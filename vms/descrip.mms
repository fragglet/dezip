!==========================================================================
! MMS description file for UnZip 5.1                              21 Dec 93
!==========================================================================
!
!   Original by Antonio Querubin, Jr., <querubin@uhccvx.uhcc.hawaii.edu>
!     (23 Dec 90)
!   Enhancements by Igor Mandrichenko, <mandrichenko@mx.decnet.ihep.su>
!     (9 Feb 92 -> ...)

! To build UnZip that uses shared libraries,
!	mms
! (One-time users will find it easier to use the MAKE_VAXC.COM command
! file, which generates both UnZip and ZipInfo.  Just type "@MAKE_VAXC";
! or "@MAKE_GCC" if you have GNU C.)

! To build UnZip without shared libraries,
!	mms noshare

! To delete .OBJ, .EXE and .HLP files,
!	mms clean

.IFDEF EXE
.ELSE
EXE = .EXE
OBJ = .OBJ
OLB = .OLB
.ENDIF

.IFDEF __ALPHA__
CC = CC/STANDARD=VAXC/NOWARNINGS
OPTFILE =
OPTIONS =
.ELSE
OPTFILE = ,[.VMS]VMSSHARE.OPT
OPTIONS = $(OPTFILE)/OPTIONS
.ENDIF

.IFDEF __DEBUG__
CFLAGS = $(CFLAGS)/DEBUG/NOOPTIMIZE
LINKFLAGS = $(LINKFLAGS)/DEBUG
.ELSE
LINKFLAGS = $(LINKFLAGS)/NOTRACE
.ENDIF

OBJS =	unzip$(OBJ),-
-!	crypt$(OBJ),-
	envargs$(OBJ),-
	explode$(OBJ),-
	extract$(OBJ),-
	file_io$(OBJ),-
	inflate$(OBJ),-
	match$(OBJ),-
	unreduce$(OBJ),-
	unshrink$(OBJ),-
	zipinfo$(OBJ),-
	[.VMS]vms$(OBJ)

default	:	unzip$(EXE) unzip.hlp
	@	!	Do nothing.

unzip$(EXE) :	UNZIP$(OLB)($(OBJS))$(OPTFILE)
	$(LINK)$(LINKFLAGS) UNZIP$(OLB)/INCLUDE=UNZIP/LIBRARY$(OPTIONS)

noshare :	$(OBJS)
	$(LINK) /EXE=$(MMS$TARGET) $(OBJS),SYS$LIBRARY:VAXCRTL.OLB/LIB

clean :
	delete $(OBJS)	! you may want to change this to 'delete *.obj;*'
	DELETE UNZIP$(EXE);*
	DELETE UNZIP.HLP;*

unzip.hlp	: [.vms]unzip.rnh
crypt$(OBJ) 	: crypt.c unzip.h zip.h crypt.h
envargs$(OBJ)	: envargs.c unzip.h
explode$(OBJ)	: explode.c unzip.h
extract$(OBJ)	: extract.c unzip.h crypt.h
file_io$(OBJ)	: file_io.c unzip.h crypt.h tables.h
inflate$(OBJ)	: inflate.c inflate.h unzip.h
match$(OBJ)	: match.c unzip.h
unreduce$(OBJ)	: unreduce.c unzip.h
unshrink$(OBJ)	: unshrink.c unzip.h
unzip$(OBJ)	: unzip.c unzip.h
zipinfo$(OBJ)	: zipinfo.c unzip.h
[.VMS]vms$(OBJ)	: [.VMS]vms.c [.VMS]vms.h unzip.h
	$(CC) $(CFLAGS) /INCLUDE=SYS$DISK:[] /OBJ=$(MMS$TARGET) [.VMS]VMS.C
