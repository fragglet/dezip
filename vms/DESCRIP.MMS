! MMS description file for unzip.
!
!   From: AntonioQuerubin, Jr. <QUERUBIN@uhccvx.uhcc.hawaii.edu>
!   Date: Sun 23 Dec 90 13:49:55-HST
!
!   "The following is an MMS description file that works for unzip.  The UNIX 
!   makefile is much too different to be converted to work also with mms.  MMS 
!   will search for a file called DESCRIP.MMS before looking for MAKEFILE so 
!   you should be able to include this in the distribution as descrip.mms to 
!   avoid confusion."

! To build unzip that uses shared libraries,
!	mms
! [For this to work, however, you presently need a separate file called
!   VMSSHARE.OPT, which is no longer included as a normal part of this pack-
!   age.  It's trivial to reconstruct, however; it contains the single line:
!	sys$share:vaxcrtl.exe/shareable
!   (not including the "! ").  One-time users will find it easier to use
!   the VMS_MAKE.COM command file; real MMS hacker-types can probably figure
!   out how to include the sys$share line within this file.  GRR, 1991.2.19]

! To build unzip without shared libraries,
!	mms noshare

! To delete unnecessary OBJ files,
!	mms clean

! Toad Hall Note:  Jumping in where mortals fear to tread,
! I'm attempting to add the vms_attr.c stuff Cave Newt gleefully
! threw at me (and ran giggling away).
! Not having the *foggiest* idea about ANYTHING VMS-related ....
! Just look for the "vms_attr" stuff.
! David Kirschbaum
! Toad Hall

CC = cc
CFLAGS =
LD = link
LDFLAGS =
EXE =
O = .obj;
OBJS = unzip$(O), file_io$(O), mapname$(O), match$(O), misc$(O), \
       unimplod$(O), unreduce$(O), unshrink$(O), vms_attr$(O)
LDFLAGS2 =

default :	$(OBJS), vmsshare.opt
	$(LD) $(LDFLAGS) $(OBJS), \
		vmsshare.opt/options, \
		sys$library:vaxcrtl.olb/library $(LDFLAGS2)

noshare :	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS), \
		sys$library:vaxcrtl.olb/library $(LDFLAGS2)

clean :
	delete $(OBJS)	! you may want to change this to 'delete *.obj;*'

vms_attr$(O):   fatdef.h fchdef.h fjndef.h
file_io$(O) :	file_io.c unzip.h
mapname$(O) :	mapname.c unzip.h
match$(O) :	match.c unzip.h
misc$(O) :	misc.c unzip.h
unimplod$(O) :	unimplod.c unzip.h
unreduce$(O) :	unreduce.c unzip.h
unshrink$(O) :	unshrink.c unzip.h
unzip$(O) :	unzip.c unzip.h
