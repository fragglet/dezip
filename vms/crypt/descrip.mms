! =========================================================================
! MMS description file for UnZip 4.2.
! Version:  decrypt + no inflate
! =========================================================================
!
!   Original by Antonio Querubin, Jr., <querubin@uhccvx.uhcc.hawaii.edu>
!     (23 Dec 90)
!   Enhancements by Igor Mandrichenko, <mandrichenko@mx.decnet.ihep.su>
!     (9 Feb 92)

! To build unzip that uses shared libraries,
!	mms
! (One-time users will find it easier to use the MAKE_UNZIP_VAXC.COM command
! file, which generates both unzip and zipinfo.  Just type "@MAKE_UNZIP_VAXC";
! or "@MAKE_UNZIP_GCC" if you have GNU C.)

! To build unzip without shared libraries,
!	mms noshare

! To delete unnecessary OBJ files,
!	mms clean

CC = cc
CFLAGS = /def=(CRYPT)
LD = link
LDFLAGS =
EXE =
O = .obj;
OBJS = unzip$(O), crypt$(O), extract$(O), file_io$(O), mapname$(O), match$(O),\
      misc$(O), unimplod$(O), unreduce$(O), unshrink$(O), VMSmunch$(O), vms$(O)
OBJI = zipinfo$(O), misc.obj_, match$(O), VMSmunch$(O)

LDFLAGS2 =

default	:	unzip.exe, zipinfo.exe
	@	!	Do nothing.

unzip.exe :	$(OBJS), vmsshare.opt
	$(LD) $(LDFLAGS) $(OBJS), \
		vmsshare.opt/options

zipinfo.exe :	$(OBJI), vmsshare.opt
	$(LD) $(LDFLAGS) $(OBJI), \
		vmsshare.opt/options


noshare :	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS), \
		sys$library:vaxcrtl.olb/library $(LDFLAGS2)

clean :
	delete $(OBJS)	! you may want to change this to 'delete *.obj;*'

VMSmunch$(O) :	VMSmunch.h fatdef.h fchdef.h fjndef.h
crypt$(O) :	crypt.c unzip.h zip.h	! may or may not be included in distrib
extract$(O) :	extract.c unzip.h
file_io$(O) :	file_io.c unzip.h
! inflate$(O) :	inflate.c unzip.h	! may or may not be included in distrib
mapname$(O) :	mapname.c unzip.h
match$(O) :	match.c unzip.h
misc$(O) :	misc.c unzip.h
unimplod$(O) :	unimplod.c unzip.h
unreduce$(O) :	unreduce.c unzip.h
unshrink$(O) :	unshrink.c unzip.h
unzip$(O) :	unzip.c unzip.h fatdef.h
vms$(O)	  :	vms.c unzip.h
misc.obj_ :	misc.c unzip.h
	$(CC)/object=misc.obj_/define="ZIPINFO" misc.c
