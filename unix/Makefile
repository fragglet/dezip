#==============================================================================
# Makefile for UnZip, ZipInfo & fUnZip:  Unix and MS-DOS ("real" makes only)
# Version:  5.1                                                 8 February 1994
#==============================================================================


# INSTRUCTIONS (such as they are):
#
# "make vax"	-- makes UnZip on a generic Unix VAX in the current directory
# "make list"	-- lists all supported systems (targets)
# "make help"	-- provides pointers on what targets to try if problems occur
# "make wombat" -- chokes and dies if you haven't added the specifics for your
#		    Wombat 68000 (or whatever) to the systems list
#
# CF are flags for the C compiler.  LF are flags for the loader.  LF2 are more
# flags for the loader, if they need to be at the end of the line instead of at
# the beginning (for example, some libraries).  FC, FL and FL2 are the corre-
# sponding flags for fUnZip.  LOCAL_UNZIP is an environment variable that can
# be used to add default C flags to your compile without editing the Makefile
# (e.g., -DDEBUG_STRUC, or -FPi87 on PCs using Microsoft C).
#
# Some versions of make do not define the macro "$(MAKE)"; this is rare, but
# if things don't work, try using "make" instead of "$(MAKE)" in your system's
# makerule.  Or try adding the following line to your .login file:
#	setenv MAKE "make"
# (That never works--makes which are too stupid to define MAKE are also too
# stupid to look in the environment--but try it anyway for kicks. :-) )
#
# Memcpy and memset are provided for those systems that don't have them; they
# are in file_io.c and will be used if -DZMEM is included in CF.  These days
# almost all systems have them.
#
# Be sure to test your new UnZip (and ZipInfo and fUnZip); successful compila-
# tion does not always imply a working program.


#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use (use LOCAL_UNZIP in environment to add flags, 
# such as -DDOSWILD).

# UnZip flags
CC = cc#	try using "gcc" target rather than changing this (if you do,
LD = $(CC)#	you MUST change LD, too--else "unresolved symbol:  ___main")
LOC = $(LOCAL_UNZIP)
CF = -O -I. $(LOC)
LF = -o unzip
LF2 = -s

# fUnZip flags
FC = -DFUNZIP
FL = -o funzip
FL2 = $(LF2)

# general-purpose stuff
CP = cp
LN = ln
RM = rm -f
E =
O = .o
M = unix
SHELL = /bin/sh

# object files
OBJS1 = unzip$O crypt$O envargs$O explode$O extract$O file_io$O
OBJS2 = inflate$O match$O unreduce$O unshrink$O zipinfo$O
OBJS = $(OBJS1) $(OBJS2) $M$O
LOBJS = $(OBJS)
OBJF = funzip$O crypt_$O inflate_$O
#OBJS_OS2 = $(OBJS1:.o=.obj) $(OBJS2:.o=.obj) os2.obj
#OBJF_OS2 = $(OBJF:.o=.obj)

# installation
INSTALL = cp#	probably can change this to 'install' if you have it
# on some systems, manext=l and MANDIR=/usr/man/man$(manext) may be appropriate
manext = 1
MANDIR = /usr/local/man/man$(manext)#	where to install man pages
BINDIR = /usr/local/bin#		where to install executables
#
UNZIPS = unzip$E funzip$E
MANS = unzip.$(manext) zipinfo.$(manext) funzip.$(manext)
DOCS = unzip.doc zipinfo.doc funzip.doc
# this is a little ugly...
INSTALLED = $(BINDIR)/unzip$E $(BINDIR)/zipinfo$E $(BINDIR)/funzip$E \
	$(MANDIR)/unzip.$(manext) $(MANDIR)/zipinfo.$(manext) \
	$(MANDIR)/funzip.$(manext)

# list of supported systems/targets in this version
SYSTEMS1 = 386i 3Bx 7300 7300_gcc aix amdahl amdahl_eft apollo aviion
SYSTEMS2 = bcc_dos bsd bsd386 bull coherent convex cray cray_scc cray_v3
SYSTEMS3 = cyber_sgi dec dnix encore eta gcc gcc_dos generic generic2
SYSTEMS4 = gould hk68 hp hpux linux minix mips msc_dos next next10
SYSTEMS5 = next2x next3x nextfat osf1 pixel ptx pyramid rs6000 sco
SYSTEMS6 = sco_dos sco_sl sco_x286 sequent sgi stellar sun sysv
SYSTEMS7 = sysv_gcc sysv6300 tahoe ultrix vax v7 wombat xenix xos

####################
# DEFAULT HANDLING #
####################

# By default, print help on which makefile targets to try.  (The SYSTEM
# variable is no longer supported; use "make <target>" instead.)

help:
	@echo ""
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
 "  (Many of them do exactly the same thing, so don't agonize too much over"
	@echo\
 '  which to pick if two or more sound equally likely.)  As a last resort, feel'
	@echo\
 '  feel free to read the numerous comments within the Makefile itself.  Note'
	@echo\
 '  that to compile the decryption version of UnZip, you must obtain the full'
	@echo\
 '  versions of crypt.c and crypt.h (see the "Where" file for ftp and mail-'
	@echo\
 '  server sites).  Have a generally tolerable day.'
	@echo ""

list:
	@echo ""
	@echo\
 'Type "make <system>", where <system> is one of the following:'
	@echo ""
	@echo  "	$(SYSTEMS1)"
	@echo  "	$(SYSTEMS2)"
	@echo  "	$(SYSTEMS3)"
	@echo  "	$(SYSTEMS4)"
	@echo  "	$(SYSTEMS5)"
	@echo  "	$(SYSTEMS6)"
	@echo  "	$(SYSTEMS7)"
	@echo ""
	@echo\
 'Otherwise set the shell variable SYSTEM to one of these and just type "make".'
#	@echo\
# 'Targets for related utilities (ZipInfo and fUnZip) include:'
#	@echo ""
#	@echo  "	$(SYS_UTIL1)"
#	@echo  "	$(SYS_UTIL2)"
	@echo ""
	@echo\
 'For further (very useful) information, please read the comments in Makefile.'
	@echo ""

generic_msg:
	@echo ""
	@echo\
 '  Attempting "make generic" now.  If this fails for some reason, type'
	@echo\
 '  "make help" and/or "make list" for suggestions.'
	@echo ""


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

# this is for GNU make; comment out and notify zip-bugs if it causes errors
.SUFFIXES:	.c $O

.c$O:
	$(CC) -c $(CF) $*.c

.1.doc:
	nroff -Tman -man $< | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@


all:		generic_msg generic
unzips:		$(UNZIPS)
docs:		$(DOCS)
unzipsman:	unzips docs
unzipsdocs:	unzips docs


clean:
	rm -f $(OBJS) $(OBJF) unzip$E funzip$E zipinfo$E

install:	$(UNZIPS) $(MANS)
	$(INSTALL) $(UNZIPS) $(BINDIR)
	$(LN) $(BINDIR)/unzip$E $(BINDIR)/zipinfo$E
	$(INSTALL) unzip.1 $(MANDIR)/unzip.$(manext)
	$(INSTALL) zipinfo.1 $(MANDIR)/zipinfo.$(manext)
	$(INSTALL) funzip.1 $(MANDIR)/funzip.$(manext)

# alternatively, could use zip method:  -cd $(BINDIR); rm -f $(UNZIPS)  [etc.]
uninstall:
	rm -f $(INSTALLED)


# EDIT HERE FOR PARALLEL MAKES on Sequent (and others?)--screws up MS-DOS
# make utilities if default:  change "unzip$E:" to "unzip$E:&"

unzip$E:	$(OBJS)			# add `&' for parallel makes
	$(LD) $(LF) $(LOBJS) $(LF2)

funzip$E:	$(OBJF)			# add `&' for parallel makes
	$(LD) $(FL) $(OBJF) $(FL2)

zipinfo$E:	unzip$E
	@echo\
 '  This is a Unix-specific target.  ZipInfo is not enabled in some MS-DOS'
	@echo\
 '  versions of UnZip; if it is in yours, copy unzip.exe to zipinfo.exe'
	@echo\
 '  or else invoke as "unzip -Z" (in a batch file, for example).'
	$(LN) unzip$E zipinfo$E


crypt$O:	crypt.c unzip.h zip.h crypt.h
envargs$O:	envargs.c unzip.h
explode$O:	explode.c unzip.h
extract$O:	extract.c unzip.h crypt.h
file_io$O:	file_io.c unzip.h crypt.h tables.h
funzip$O:	funzip.c unzip.h crypt.h tables.h
inflate$O:	inflate.c inflate.h unzip.h
match$O:	match.c unzip.h
unreduce$O:	unreduce.c unzip.h
unshrink$O:	unshrink.c unzip.h
unzip$O:	unzip.c unzip.h
zipinfo$O:	zipinfo.c unzip.h

crypt_$O:	crypt.c unzip.h zip.h crypt.h		# funzip only
	$(CP) crypt.c crypt_.c
	$(CC) -c $(CF) $(FC) crypt_.c
	$(RM) crypt_.c

inflate_$O:	inflate.c inflate.h unzip.h crypt.h	# funzip only
	$(CP) inflate.c inflate_.c
	$(CC) -c $(CF) $(FC) inflate_.c
	$(RM) inflate_.c

msdos$O:	msdos/msdos.c unzip.h			# MS-DOS only
	$(CC) -c $(CF) msdos/msdos.c

#os2$O:		os2/os2.c unzip.h			# OS/2 only
#	$(CC) -c $(CF) os2/os2.c

#os2/os2_$O:	os2/os2.c unzip.h			# OS/2 only
#	$(CP) os2/os2.c os2/os2_.c
#	$(CC) -c $(CF) $(ZC) os2/os2_.c
#	$(RM) os2/os2_.c

unix$O:		unix/unix.c unzip.h			# Unix only
	$(CC) -c $(CF) unix/unix.c

unix_make:
#	@echo\
# '(Ignore any errors from `make'"' due to the following command; it's harmless.)"
	-@2>&1 $(LN) unix/Makefile . > /dev/null || echo > /dev/null


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

#####   TABS ARE REQUIRED FOR MANY VERSIONS OF "MAKE"!   #####


#----------------------------------------------------------------------------
#  Generic targets (can't assume make utility groks "$(MAKE)")
#----------------------------------------------------------------------------

generic:	unzips	   # first try if unknown

generic2:	unix_make  # second try if unknown:  hope make is called "make"
	make unzips CF="$(CF) -DBSD"

#----------------------------------------------------------------------------
#  "Normal" group (both big- and little-endian, structure-padding or not):
#----------------------------------------------------------------------------

386i:		unzips	# sun386i, SunOS 4.0.2
3Bx:		unzips	# AT&T 3B2/1000-80; should work on any WE32XXX machine
aix:		unzips	# IBM RT 6150 under AIX 2.2.1; all AIX?
apollo:		unzips	# Apollo Domain/OS machines
bull:		unzips	# Bull DPX/2, BOS 2.00.45 (doesn't require -Xk switch)
coherent:	unzips	# Coherent 3.x/4.x, Mark Williams C
convex:		unzips	# Convex C-120 and C-210 (-O is enough; -ext is default)
cray:		unzips	# Cray-2 and Y-MP, using default (possibly old) compiler
dec:		unzips	# DEC 5820 (MIPS RISC), test version of Ultrix v4.0
encore:		unzips	# Multimax
eta:		unzips	# ETA-10P*, hybrid SysV with BSD 4.3 enhancements
gould:		unzips	# Gould PN9000 running UTX/32 2.1Bu01
hp:		unzips	# HP 9000 series (68020), 4.3BSD or HP-UX A.B3.10 Ver D
hpux:		unzips	# (to match zip's makefile entry)
mips:		unzips	# MIPS M120-5(?), SysV.3 [error in sys/param.h file?]
next10:		unzips	# NeXT (generic; use next2x or next3x for better opt.)
pyr_:		unzips	# [failsafe target for pyramid target below]
pyr_ucb:	unzips	# Pyramids running BSD universe by default (see below)
sco:		unzips	# Xenix/386 (tested on 2.3.1); SCO Unix 3.2.0.
stellar:	unzips	# gs-2000
sun:		unzips	# Sun 3, 4; SunOS 4.x (SOME SYSTEMS ARE SYSTEM V!)
tahoe:		unzips	# tahoe (CCI Power6/32), 4.3BSD
ultrix:		unzips	# VAXen, DEC 58x0 (MIPS guts), DECstation 2100; v4.x
vax:		unzips	# general-purpose VAX target (not counting VMS)
xenix:		unzips	# Xenix/386 (tested on 2.3.1); SCO Unix 3.2.0.

#----------------------------------------------------------------------------
#  BSD group (for timezone structs [struct timeb]):
#----------------------------------------------------------------------------

bsd:		_bsd	# generic BSD (BSD 4.2 & Ultrix handled in unzip.h)
bsd386:		_bsd	# BSDI BSD/386 version 1.0

_bsd:		unix_make
	$(MAKE) unzips CF="$(CF) -DBSD"

#----------------------------------------------------------------------------
#  SysV group (for extern long timezone and ioctl.h instead of sgtty.h):
#----------------------------------------------------------------------------

amdahl:		_sysv	# Amdahl (IBM) mainframe, UTS (SysV) 1.2.4 and 2.0.1
aviion:		_sysv	# Data General AViiONs, DG/UX 4.3x
pyr_att:	_sysv	# Pyramids running AT&T (SysV) universe by default
sgi:		_sysv	# Silicon Graphics Iris 4D, Irix (SysV) 3.3.2, 4.0.x
sysv:		_sysv	# generic System V Unix (Xenix handled in unzip.h)
xos:		_sysv	# Olivetti LSX-3005..3045, X/OS 2.3 and 2.4

_sysv:		unix_make
	$(MAKE) unzips CF="$(CF) -DSYSV"

#----------------------------------------------------------------------------
#  Version 7 group (old/obsolescent):
#----------------------------------------------------------------------------

pixel:		_v7	# Pixel 80, 100 (68000-based, V7/mostly BSD4.1 compat.)
v7:		_v7	# generic Unix Version 7 box (prob. only Pixel...)

_v7:
	make unzips CF="$(CF) -DV7 -DNO_PARAM_H -DSHORT_NAMES -DBSD -DZMEM"

#----------------------------------------------------------------------------
#  "Unique" group (require non-standard options):
#----------------------------------------------------------------------------

# AT&T 7300 (M68000/SysV.3) (add -DSYSV? -DNO_LIMITS?)
7300:		unix_make
	$(MAKE) unzips CF="$(CF) -DNO_DIR -DNO_MKDIR -DNO_STRNICMP"

7300_gcc:	unix_make
	$(MAKE) unzips CC=gcc LD=gcc LF2="" \
	 CF="-O2 -I. -DNO_DIR -DNO_MKDIR -DNO_STDLIB_H -DNO_STRNICMP $(LOC)"
	strip $(UNZIPS)

# Amdahl UTS 2.1.4 with "extended file types" filesystem (aarrrggghhhh...)
amdahl_eft:	unix_make
	$(MAKE) unzips CF="$(CF) -eft -DSYSV"

# MS-DOS:  Borland C++ 3.0 (can change UnZip memory model to small for more
# speed but no ZipInfo support [-ml -> -ms in _bcc_dos], but may run out of
# memory when inflating
bcc_dos:	_bcc_dos fu_bcc

_bcc_dos:	bcc_rsp
	$(MAKE) unzip.exe CF="-w -ml -O2 -I. $(LOC)" CC=bcc LD=bcc E=.exe\
	 O=.obj M=msdos LOBJS="" LF="@bcc_rsp" LF2=""
	del bcc_rsp

bcc_rsp:
	echo $(OBJS1:.o=.obj) > bcc_rsp
	echo msdos.obj $(OBJS2:.o=.obj) >> bcc_rsp

fu_bcc:
	$(MAKE) funzip.exe CF="-w -ms -O2 -I. $(LOC)" CC=bcc LD=bcc E=.exe\
	 O=.obj FL="" FL2="" CP=copy RM=del

# Cray-2 and Y-MP, running Unicos 5.1 to 6.1 (SysV + BSD enhancements)
# and Standard (ANSI) C compiler 1.5, 2.0 or 3.0.
cray_scc:	unix_make
	$(MAKE) unzips CC=scc LD=scc

# Ditto, for Cray Standard C 3.0 or later.
cray_v3:	unix_make
	$(MAKE) unzips CC="scc" LD="scc" CF="$(CF) -h scalar3 -h vector3"

# The unzip41 build on a Cyber 910/SGI running Irix v3.3.3 was successful
# with the following change to Makefile:
cyber_sgi:	unix_make
	$(MAKE) unzips CF="$(CF) -I/usr/include/bsd"\
	 LF="-lbsd $(LF)"

# 680x0, DIAB dnix 5.2/5.3 (a Swedish System V clone)
#
# Options for the dnix cc:
#  -X7 = cc is strict ANSI C
#  -X9 = warnings if a function is used without a declaration
#
dnix:		unix_make
	$(MAKE) unzips CF="$(CF) -X7 -X9 -DDNIX"

# Generic BSDish Unix gcc.  ``The -O2 only works with the latest version of
# gcc; you may have to use -O only for earlier versions.  I have no idea why
# -s causes this bug in gcc.''  [Bug:  "nm: unzip: no name list", "collect:
# /usr/bin/nm returned 1 exit status".]  If you don't have strip, don't
# worry about it (it just makes the executable smaller).
#
gcc:		unix_make
	$(MAKE) unzips CC=gcc LD=gcc CF="-O2 -I. $(LOC)" LF2=""
	strip $(UNZIPS)

# MS-DOS with D.J. Delorie's djgpp 1.11.  Note that go32 doesn't support
# DOS function 0x38, so the default date format is used with the -v option.
#
gcc_dos: _gcc_dos fu_gcc

_gcc_dos: gcc_rsp
	$(MAKE) unzip CC=gcc LD=gcc M=msdos CF="-O2 -Wall -I. $(LOC)" CP=copy\
	 RM=del LOBJS="" LF="-o unzip @gcc_rsp" LF2=""
	aout2exe unzip
	del gcc_rsp
	del unzip

gcc_rsp:
	echo $(OBJS1) > gcc_rsp
	echo msdos.o $(OBJS2) >> gcc_rsp

fu_gcc:
	$(MAKE) funzip CF="-Wall -O2 -I. $(LOC)" CC=gcc LD=gcc FL2="" CP=copy\
	 RM=del
	aout2exe funzip
	del funzip

# Heurikon HK68 (68010), UniPlus+ System V 5.0, Green Hills C-68000
hk68:		unix_make
	$(MAKE) unzips CC="gcc" LD="gcc" LF="-n $(LF)" \
	 CF="-ga -X138 -I. $(LOC) -Dlocaltime=localti -Dtimezone=timezon"

# Rules needed to build the unzip program for an SGI Iris Indigo running
# Irix Version 4.0.1
#indigo:
#	$(MAKE) unzips CF="-cckr $(CF) -DSYSV $(LOC)"

# Linux pre-0.96 + gcc 2.1; also 0.99.13 + gcc 2.4.5.  Linux is Posix
# (almost SysV but not quite).  [apparently don't need -DSHORT_NAMES]
linux:		unix_make
	$(MAKE) unzips CC=gcc LD=gcc
#	$(MAKE) unzips CF="$(CF) -DLINUX" CC=gcc LD=gcc  (linux pre-defined?)

# Minix 1.5 PC for the 386.  Invoke as is to use default cc, or as "make
# minix CC=gcc" to use gcc.  Try "make linux" if you have a working termios.h.
minix:		unix_make
	$(MAKE) unzips CF="$(CF) -DMINIX -DSHORT_NAMES" CC=$(CC) LD=$(CC)

# MS-DOS:  Microsoft C 6.0 and NMAKE.  "nmake msc_dos" works fine, aside from
# an irrelevant message (possibly) about the creation of a temporary file.
# Environment variable LOCAL_UNZIP should be set via "SET LOCAL_UNZIP=-FPi87"
# if you use the 80x87 library; also add -G2 or -G3 if using a 286/386/486.
# With VC++ Pro, -O2 instead of -Oait seems to work OK and makes the
# executables slightly faster. Use at your own risk.
#
msc_dos:	_msc_dos fu_msc

# can change UnZip memory model (-AL) to small (-AS) to disable ZipInfo
# mode in favor of speed, but may still run out of memory...
_msc_dos:	msc_rsp
	$(MAKE) unzip.exe CF="-AL -W3 -Oait -Gs -nologo -I. $(LOC)" CC=cl\
	 LD=link E=.exe O=.obj M=msdos LOBJS="" LF="@msc_rsp" LF2=""
	del msc_rsp

msc_rsp:
	echo $(OBJS1:.o=.obj) + > msc_rsp
	echo msdos.obj $(OBJS2:.o=.obj)/noi/e/st:0x0c00; >> msc_rsp

fu_msc:		# (probably over 128-char limit)
	$(MAKE) funzip.exe CF="-AS -Oait -Gs -nologo -I. $(LOC)" CC=cl LD=link\
	 E=.exe O=.obj FL="/noi/nol/e" FL2=",funzip;" RM=del CP=copy

# $(NOD):  intended to be used as   SET NOD=-link /nod:slibcep   to allow the
#          use of default library names (slibce.lib) instead of protected-mode
#          names (slibcep.lib), but it fails:  MSC adds its own /nod qualifier,
#          and there seems to be no way to override this.  Typical...
#
#    THIS TARGET RETAINED AS AN EXAMPLE ONLY.  USE MAKEFILE.OS2.
#
#msc_os2:		# 16-bit OS/2 (1.x) with MSC 6.00 (use makefile.os2)
#	$(MAKE) -nologo unzips CC=cl LD=cl E=.exe O=.obj\
#	 OBJS="$(OBJS_OS2)" OBJZ="$(OBJZ_OS2)"\
#	 CF="-nologo -AC -Ocegit -G2s -DOS2 -DMSC $(LOC)"\
#	 LF="-nologo -AC $(LOC) -Lp -F 2000"\
#	 LF2="unzip.def -o unzip.exe $(NOD)" CP=copy RM=del\
#	 ZL="-nologo -AC $(LOC) -Lp -Fb" ZL2="zipinfo.def -o zipinfo.exe"

# NeXT info.
next:
	@echo
	@echo\
 '  Please pick a specific NeXT target:  "make next10" will create a generic'
	@echo\
 '  NeXT executable; "make next2x" will create a smaller executable (for'
	@echo\
 '  NeXTstep 2.0 and higher); "make next3x" will create a small executable'
	@echo\
 '  with significantly better optimization (NeXTstep 3.0 and higher only);'
	@echo\
 '  "make nextfat" will create a fat, multi-architecture (NeXT plus Intel)'
	@echo\
 '  executable (NeXTstep 3.1 and higher only).'
	@echo

# 68030 BSD 4.3+Mach.  NeXT 2.x: make the executable smaller.
next2x:		unix_make
	$(MAKE) unzips LF2="-object -s"

# NeXT 3.x: as above, plus better optimization.
next3x:		unix_make
	$(MAKE) unzips CF="-O2 -I. $(LOC)" LF2="-object -s"

# NeXT 3.1+: make the executable fat (multi-architecture binary [MAB],
# for "black" [NeXT] and "white" [x86] hardware, so far).
nextfat:	unix_make
	$(MAKE) unzips CF="-O2 -I. $(LOC) -arch i386 -arch m68k" \
	 LF2="-arch i386 -arch m68k -object -s"

# DecStation (including Alpha-based) running DEC OSF/1 v1.x; hasn't got
# ftime(3) in the standard C library.  (GRR:  use _sysv target instead?)
osf1:		unix_make
	$(MAKE) unzips CF="$(CF) -D_BSD" LF2="$(LF2) -lbsd"

# I successfully compiled and tested the unzip program (v30) for the
# Silicon Graphics environment (Personal Iris 4D20/G with IRIX v3.2.2)
#p_iris:
#	$(MAKE) unzips CF="$(CF) -I/usr/include/bsd -DBSD"\
#	 LF="-lbsd $(LF)"

# Sequent Symmetry running Dynix/ptx (sort of SysV.3):  needs to link
# with libseq to get symlink().
ptx:		unix_make
	$(MAKE) unzips CF="$(CF) -DSYSV -DTERMIO" LF2="$(LF2) -lseq"

# Pyramid 90X (probably all) under >= OSx4.1, either universe.  (This is an
# experimental target!  If it fails, use either pyr_ucb or pyr_att instead.)
# The make in the BSD half is too stupid to understand $(MAKE), sigh...
pyramid:	unix_make
	-make pyr_`universe`

# IBM RS/6000 under AIX 3.2
rs6000:		unix_make
	$(MAKE) unzips CF="$(CF) -DBSD -D_BSD -DUNIX" LF="-lbsd $(LF)"

# SCO cross compile from Unix to DOS. Tested with Xenix/386 and OpenDeskTop.
# Should work with Xenix/286 as well. (davidsen)  Note that you *must* remove
# the Unix objects and executable before doing this!  (Piet Plomp:  gcc won't
# recognize the -M0 flag which forces 8086 code.)  (GRR:  may need to reduce
# stack to 0c00h if using 286/small-model code...?)
sco_dos:	unix_make
	$(MAKE) unzips CF="-O -I. $(LOC) -DNO_ERRNO -dos -M0" M=msdos\
	 LF="-dos -F 2000" LF2="-o unzip.exe" FL="-dos" FL2="-o funzip.exe"

# SCO UNIX with shared libraries and no international support.  If you are
# not using a USA-style keyboard and display, you may want to remove -nointl
# to get support.  It adds quite a bit to the size of the executable.
sco_sl:		unix_make
	$(MAKE) unzips LF="$(LF) -nointl" LF2="$(LF2) -lc_s"

# SCO Xenix/286 2.3.3 with 2.2.1 development system
sco_x286:	unix_make
	$(MAKE) unzips CF="$(CF) -Mel2 -LARGE -DTERMIO" LF="$(LF) -Mel2 -LARGE"

# Sequent Symmetry with Dynix.  (386, but needs -DZMEM)
# This should also work on Balance but I can't test it just yet.
sequent:	unix_make
	$(MAKE) unzips CF="$(CF) -DBSD -DZMEM"

# Generic System V + GNU C
sysv_gcc:	unix_make
	$(MAKE) unzips CC=gcc LD=gcc CF="-O2 -I. -DSYSV $(LOC)" LF2=""
	strip $(UNZIPS)

# AT&T 6300+, System V.2 Unix:  run-time out-of-memory error if don't use -Ml;
# also compile-time error if work arrays dimensioned at HSIZE+2 (>32K)
sysv6300:	unix_make
	$(MAKE) unzips CF="$(CF) -Ml -DSYSV" LF="$(LF) -Ml" FL="$(FL) -Ml"

# Wombat 68000 (or whatever).
# I didn't do this.  I swear.  No, really.
wombat:		unix_make
	@echo
	@echo  '	Ha ha!  Just kidding.'
	@echo
