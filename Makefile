# Makefile for unzip
#
# ******** INSTRUCTIONS (such as they are) ********
#
# "make vax"	-- makes unzip on a VAX 11-780 BSD 4.3 in current directory
#		   (or a SysV VAX, or an 8600 running Ultrix, or...)
# "make"	-- uses environment variable SYSTEM to set the type
#		   system to compile for.  This doesn't work for some
#		   particularly brain-damaged versions of make (VAX BSD,
#		   Gould, and SCO Unix are in this group).
# "make wombat" -- Chokes and dies if you haven't added the specifics
#		   for your Wombat 68000 (or whatever) to the systems list.
#
# CFLAGS are flags for the C compiler.  LDFLAGS are flags for the loader.
# LDFLAGS2 are more flags for the loader, if they need to be at the end
# of the line instead of at the beginning.
#
# My host (a VAX 11-780 running BSD 4.3) is hereafter referred to as
# "my host."
#
# My host's /usr/include/sys/param.h defines BSD for me.
# You may have to add "-DBSD" to the list of CFLAGS for your system.
#
# You MAY need to define "-DNOTINT16" if the program produces CRC errors
# during a "-t" run or extraction (this involves structure alignment/padding);
# you MUST define it if your machine is "big-endian" (i.e., it orders bytes
# in Motorola fashion rather than Intel fashion).  The latter case is charac-
# terized by errors of the form "some-kind-of-header signature not found.
# Please report to Info-ZIP." OR a silent do-nothing return with an exit code
# of 51 (EOF).  It should always be safe to define NOTINT16, but it does add
# some overhead, so first try compiling without it and see what happens.
# If death and destruction ensue, it's probably better to go ahead and use
# NOTINT16.
#
# Some versions of make do not define the macro "$(MAKE)" (my host did not).
# The makefile should now handle such systems correctly, more or less; the
# possible exception to this is if you've used a make command-line option
# (for example, the one which displays the commands which WOULD be executed,
# but doesn't actually execute them).  It probably needs some more tinkering.
# If things still don't work, use "make" instead of "$(MAKE)" in your system's
# makerule.  Or try adding the following line to your .login file:
#   setenv MAKE "make"
# (It didn't help on my host.)
#
# memcpy and memset are provided for those systems that don't have them;
# they're found in misc.c and will be used if -DZMEM is included in the list
# of CFLAGS.  These days ALMOST all systems have them (they're mandated by
# ANSI), but older systems might be lacking.  And at least ONE machine's
# version results in some serious performance degradation...
#
# To test your nice new unzip, insure your zip file includes some LARGE
# members.  Many systems ran just fine with zip file members < 512 bytes
# but failed with larger ones.  Members which are "shrunk" rather than
# "imploded" have also caused many problems in the past, so try to test a
# zipfile which contains some of both.  And it's quite possible that this
# program has NEVER been tested on a "reduced" file, so keep your eyes
# open.  (They're probably mythical, though.)

#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use
CC = cc
CFLAGS = -O -DUNIX
LD = cc
LDFLAGS = -o unzip
EXE =
O = .o
OBJS = unzip$O file_io$O mapname$O match$O misc$O\
       unimplod$O unreduce$O unshrink$O

SHELL = /bin/sh

# list of supported systems in this version
SYSTEMS1 = 386i 3Bx amdahl apollo aviion convex cray cray_cc
SYSTEMS2 = dnix encore generic generic2 gould hp mips msc_dos
SYSTEMS3 = msc_os2 next pyramid rtaix sco sco_dos sequent sgi
SYSTEMS4 = stellar sun tahoe ultrix_risc ultrix_vax vax wombat
# SYSTEMS5 =

####################
# DEFAULT HANDLING #
####################

# The below will try to use your shell variable "SYSTEM" as the type system
# to use (e.g., if you type "make" with no parameters at the command line).
# The test for $(MAKE) is necessary for VAX BSD make (and Gould, apparently),
# as is the "goober" (else stupid makes see an "else ;" statement, which they
# don't like).  "goober" must then be made into a valid target for machines
# which DO define MAKE properly (and have SYSTEM set).  Quel kludge, non?
# And to top it all off, it appears that the VAX, at least, can't pick SYSTEM
# out of the environment either (which, I suppose, should not be surprising).
# [Btw, if the empty "goober" target causes someone else's make to barf, just
# add an "@echo > /dev/null" command (or whatever).  Works OK on the Amdahl
# and Crays, though.]

default:
	@if test -z "$(MAKE)"; then\
		if test -z "$(SYSTEM)";\
		then make ERROR;\
		else make $(SYSTEM) MAKE="make";\
		fi;\
	else\
		if test -z "$(SYSTEM)";\
		then $(MAKE) ERROR;\
		else $(MAKE) $(SYSTEM) goober;\
		fi;\
	fi

goober:

ERROR:
	@echo
	@echo\
 "  If you're not sure about the characteristics of your system, try typing"
	@echo\
 '  "make generic".  If the compiler barfs and says something unpleasant about'
	@echo\
 '  "timezone redefined," try typing "make clean" followed by "make generic2".'
	@echo\
 '  One of these actions should produce a working copy of unzip on most Unix'
	@echo\
 '  systems.  If you know a bit more about the machine on which you work, you'
	@echo\
 '  might try "make list" for a list of the specific systems supported herein.'
	@echo\
 '  And as a last resort, feel free to read the numerous comments within the'
	@echo\
 '  Makefile itself.  Have an excruciatingly pleasant day.'
	@echo

list:
	@echo
	@echo\
 'Type "make <system>", where <system> is one of the following:'
	@echo
	@echo  "	$(SYSTEMS1)"
	@echo  "	$(SYSTEMS2)"
	@echo  "	$(SYSTEMS3)"
	@echo  "	$(SYSTEMS4)"
#	@echo  "	$(SYSTEMS5)"
	@echo
	@echo\
 'Otherwise set the shell variable SYSTEM to one of these and just type "make".'
	@echo\
 'For further (very useful) information, please read the comments in Makefile.'
	@echo


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

.c$O :
	$(CC) -c $(CFLAGS) $*.c

unzip$(EXE):	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LDFLAGS2)

file_io$O:      file_io.c unzip.h
mapname$O:      mapname.c unzip.h
match$O:        match.c unzip.h
misc$O:         misc.c unzip.h
unimplod$O:     unimplod.c unzip.h
unreduce$O:     unreduce.c unzip.h
unshrink$O:     unshrink.c unzip.h
unzip$O:        unzip.c unzip.h

clean:
	rm -f $(OBJS) unzip$(EXE)

# Zipinfo section commented out because it's no longer compatible with
# the current unzip.h (I think).  Will be updated one of these days...
#
# zipinfo:	zipinfo.c unzip.h
#	$(CC) $(CFLAGS) -DNOTINT16 zipinfo.c -o zipinfo


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

# these are the makerules for various systems
# TABS ARE REQUIRED FOR SOME VERSIONS OF make!


# ---------------------------------------------------------------------------
#   Generic targets (can't assume make utility groks "$(MAKE)")
# ---------------------------------------------------------------------------

generic:	# first try for unknown systems:  hope make is called "make"...
	make unzip CFLAGS="$(CFLAGS) -DNOTINT16"

generic2:	# second try for unknown systems:  keep hoping...
	make unzip CFLAGS="$(CFLAGS) -DNOTINT16 -DBSD"

# ---------------------------------------------------------------------------
#   "Normal" (i.e., PC-like) group (no #defines):
# ---------------------------------------------------------------------------

386i:		unzip	# sun386i, SunOS 4.0.2 ["sun:" works, too, but bigger]
encore:		unzip	# Multimax
sco:		unzip	# Xenix/386 (tested on 2.3.1); SCO Unix 3.2.0.
ultrix_vax:	unzip	# VAXen running Ultrix (just 4.0?); not RISC machines
vax:		unzip	# general-purpose VAX target (not counting VMS)

# ---------------------------------------------------------------------------
#   NOTINT16 (structure-padding and/or big-endian) group:
# ---------------------------------------------------------------------------

3Bx:		_16	# AT&T 3B2/1000-80; should work on any WE32XXX machine
amdahl:		_16	# Amdahl (IBM) mainframe, UTS (SysV) 1.2.4 and 2.0.1
apollo:		_16	# Apollo Domain/OS machines
aviion:         _16     # Data General AViiONs, DG/UX 4.3x
convex:		_16	# C200/C400
cray_cc:	_16	# Cray-2 and Y-MP, using old-style compiler
#dec5820:	_16	# DEC 5820 (RISC), Test version of Ultrix v4.0
dnix:		_16	# 680X0, DIAB dnix 5.2/5.3 (a Swedish System V clone)
gould:		_16	# Gould PN9000 running UTX/32 2.1Bu01
hp:		_16	# HP 9000 series (68020), 4.3BSD or HP-UX A.B3.10 Ver D
mips:		_16	# MIPS M120-5(?), SysV R3 [error in sys/param.h file?]
#next:		_16	# 68030 BSD 4.3+Mach
rtaix:		_16	# IBM RT 6150 under AIX 2.2.1
stellar:	_16	# gs-2000
tahoe:		_16	# tahoe (CCI Power6/32), 4.3BSD

_16:
	$(MAKE) unzip CFLAGS="$(CFLAGS) -DNOTINT16"

# ---------------------------------------------------------------------------
#   NOTINT16 + BSD (for timezone structs) group:
# ---------------------------------------------------------------------------

sun:		_16bsd	# Sun 4/110, SunOS 4.0.3c; Sun 3 (68020), SunOS 4.0.3
ultrix_risc:	_16bsd	# DEC 58x0 (MIPS guts), DECstation 2100; Ultrix v4.1

_16bsd:
	$(MAKE) unzip CFLAGS="$(CFLAGS) -DNOTINT16 -DBSD"

# ---------------------------------------------------------------------------
#   "Unique" group (require non-standard options):
# ---------------------------------------------------------------------------

# Sequent Symmetry is a 386 but needs -DZMEM
# This should also work on Balance but I can't test it just yet.

sequent:	# Sequent w/Dynix
	$(MAKE) unzip CFLAGS="$(CFLAGS) -DNOTINT16 -DBSD -DZMEM"

# I have finished porting unzip 3.0 to the Pyramid 90X under OSX4.1.
# The biggest problem was the default structure alignment yielding two
# extra bytes.  The compiler has the -q option to pack structures, and
# this was all that was needed.  To avoid needing ZMEMS we could compile in
# the att universe, but it runs slower!

pyramid:	# Pyramid 90X, probably all, under >= OSx4.1, BSD universe
	make unzip CFLAGS="$(CFLAGS) -q -DBSD -DNOTINT16 -DZMEM"

# I successfully compiled and tested the unzip program (v30) for the
# Silicon Graphics environment (Personal Iris 4D20/G with IRIX v3.2.2)
#
#  Valter V. Cavecchia          | Bitnet:       cavecchi@itncisca
#  Centro di Fisica del C.N.R.  | Internet:     root@itnsg1.cineca.it
#  I-38050 Povo (TN) - Italy    | Decnet:       itnvax::cavecchia (37.65)

sgi:		# Silicon Graphics (tested on Personal Iris 4D20)
	$(MAKE) unzip \
	CFLAGS="$(CFLAGS) -I/usr/include/bsd -DBSD -DNOTINT16" \
	LDFLAGS="-lbsd $(LDFLAGS)"

# Cray-2 and Y-MP, running Unicos 5.1.10 or 6.0 (SysV + BSD enhancements)
# and Standard (ANSI) C compiler 1.5 or 2.0.1.

cray:
	$(MAKE) unzip CC="scc" LD="scc" CFLAGS="$(CFLAGS) -DNOTINT16"

# SCO cross compile from unix to DOS. Tested with Xenix/386 and
# OpenDeskTop. Should work with xenix/286 as well. (davidsen)
# Note that you *must* remove the unix objects and executable
# before doing this!

sco_dos:
	$(MAKE) unzip CFLAGS="-O -dos -M0" LDFLAGS="-dos" \
		LDFLAGS2="-o unzip.exe"

# PCs (IBM-type), running MS-DOS, Microsoft C 6.00 and NMAKE.  Can't use the
# SYSTEM environment variable; that requires processing the "default:" target,
# which expands to some 200+ characters--well over DOS's 128-character limit.
# "nmake msc_dos" works fine, aside from an annoying message, "temporary file
# e:\ln023193 has been created."  I have no idea how to suppress this, but it
# appears to be benign (comes from the link phase; the file is always deleted).
# The environment variable FP should be set to something appropriate if your 
# library uses other than the default floating-point routines; for example, 
# SET FP=-FPi87 .  "msc_dos:" does NOT work with Dennis Vadura's dmake (dmake 
# doesn't seem to understand quotes, and it wants SHELL=$(COMSPEC).  NMAKE 
# ignores SHELL altogether.)  This target assumes the small-model library and 
# an 80286 or better.  At present, everything fits within the 128-character 
# command-line limit.  (Barely.)

msc_dos:
	$(MAKE) -nologo unzip.exe CFLAGS="-Ox -nologo $(FP) -G2" CC=cl\
	LD=link EXE=.exe O=.obj LDFLAGS="/noi /nol" LDFLAGS2=",unzip;"

# I do not believe that OS/2 has the 128 character command line limitation, 
# so when I have more time to get to know how nmake really works, I may figure 
# out how to set the makefile so that it works like most of the other things 
# in unix.  The main things I had to change were adding the define -DOS2 and 
# the C flag -Lp.  It still looks for the default named libraries, and not 
# the protected-mode names, but I am sure most people dealing with OS/2 know
# how to type slibcep when it says it can't find slibce.

msc_os2:
	$(MAKE) -nologo unzip.exe CFLAGS="-Ox -nologo $(FP) -G2 -DOS2 -Lp"\
	CC=cl LD=link EXE=.exe O=.obj LDFLAGS="/noi /nol"\
	LDFLAGS2=",unzip,,,unzip.def;"

# NeXT 2.x: make the executable smaller.
next:
	$(MAKE) unzip CFLAGS="$(CFLAGS) -DNOTINT16" LDFLAGS2="-object -s"

# I didn't do this.  I swear.  No, really.

wombat:		# Wombat 68000 (or whatever)
	@echo
	@echo  '	Ha ha!  Just kidding.'
	@echo

################
# ATTRIBUTIONS #
################

# Thanks to the following people for their help in testing and/or porting
# to various machines:
#
#  386i:	Richard Stephen, stephen@corp.telecom.co.nz
#  3Bx:		Bob Kemp, hrrca!bobc@cbnewse.att.com
#  amdahl:	Kim DeVaughn, ked01@juts.ccc.amdahl.com
#  apollo:	Tim Geibelhaus
#  aviion:	Bruce Kahn, bkahn@archive.webo.dg.com
#  cray:	Greg Roelofs, roelofs@amelia.nas.nasa.gov
#  dec5820:	"Moby" Dick O'Connor, djo7613@u.washington.edu
#  dnix:	Bo Kullmar, bk@kullmar.se
#  gould:	Onno van der Linden, linden@fwi.uva.nl
#  hp:		Randy McCaskile, rmccask@seas.gwu.edu (HP-UX)
#   		Gershon Elber, gershon@cs.utah.edu (HP BSD 4.3)
#  mips:	Peter Jones, jones@mips1.uqam.ca
#  msc_dos:	Greg Roelofs
#  msc_os2:	Wim Bonner, wbonner@yoda.eecs.wsu.edu
#  next:	Mark Adler, madler@piglet.caltech.edu
#  pyramid:	James Dugal, jpd@usl.edu
#  rtaix:	Erik-Jan Vens
#  sco:		Onno van der Linden (SCO Unix 3.2.0)
#   		Bill Davidsen, davidsen@crdos1.crd.ge.com (Xenix/386)
#  sco_dos:	Bill Davidsen
#  sequent:	Phil Howard, phil@ux1.cso.uiuc.edu
#  sgi:		Valter V. Cavecchia (see comments for addresses)
#  sun:		Onno van der Linden (Sun 4)
#  tahoe:	Mark Edwards, mce%sdcc10@ucsd.edu
#  ultrix_vax:	Greg Flint, afc@klaatu.cc.purdue.edu
#  ultrix_risc:	Michael Graff, explorer@iastate.edu
#  vax:		Forrest Gehrke, feg@dodger.att.com (SysV)
#		David Kirschbaum, kirsch@usasoc.soc.mil (BSD 4.3)
#		Jim Steiner, steiner@pica.army.mil (8600+Ultrix)
#  wombat:	Joe Isuzu, joe@trustme.isuzu.com

# SCO unix 3.2.0:
# Don't use -Ox with cc (derived from Microsoft 5.1), there is
# a bug in the loop optimization, which causes bad CRC's
#
# Onno van der Linden
