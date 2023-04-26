#==============================================================================
# Makefile for UnZip, UnZipSFX and fUnZip:  Unix and MS-DOS ("real" makes only)
# Version:  6.0                                                     18 Jan 2009
#==============================================================================


# INSTRUCTIONS (such as they are):
#
# "make sunos"	-- makes UnZip in current directory on a generic SunOS 4.x Sun
# "make list"	-- lists all supported systems (targets)
# "make help"	-- provides pointers on what targets to try if problems occur
# "make wombat" -- chokes and dies if you haven't added the specifics for your
#		    Wombat 68000 (or whatever) to the systems list
#
# CF are flags for the C compiler.  LF are flags for the loader.  LF2 are more
# flags for the loader, if they need to be at the end of the line instead of at
# the beginning (for example, some libraries).  FL and FL2 are the corre-
# sponding flags for fUnZip.  LOCAL_UNZIP is an environment variable that can
# be used to add default C flags to your compile without editing the Makefile
# (e.g., -DDEBUG_STRUC, or -FPi87 on PCs using Microsoft C).
#
# Some versions of make do not define the macro "$(MAKE)"; this is rare, but
# if things don't work, try using "make" instead of "$(MAKE)" in your system's
# makerule.  Or try adding the following line to your .login file:
#	setenv MAKE "make"
# (That never works--makes that are too stupid to define MAKE are also too
# stupid to look in the environment--but try it anyway for kicks. :-) )
#
# Memcpy and memset are provided for those systems that don't have them; they
# are in fileio.c and will be used if -DZMEM is included in CF.  These days
# almost all systems have them.
#
# Be sure to test your new UnZip (and UnZipSFX and fUnZip); successful compila-
# tion does not always imply a working program.


#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use (use LOCAL_UNZIP in environment to add flags,
# such as -DDOSWILD).

# UnZip flags
CC = cc#	try using "gcc" target rather than changing this (CC and LD
LD = $(CC)#	must match, else "unresolved symbol:  ___main" is possible)
AS = as
LOC = $(D_USE_BZ2) $(LOCAL_UNZIP)
AF = $(LOC)
CFLAGS = -O
CF_NOOPT = -I. -I$(IZ_BZIP2) -DUNIX $(LOC)
CF = $(CFLAGS) $(CF_NOOPT)
LFLAGS1 =
LF = -o unzip$E $(LFLAGS1)
LF2 = -s

# general-purpose stuff
#CP = cp
CP = ln
LN = ln
RM = rm -f
CHMOD = chmod
BINPERMS = 755
MANPERMS = 644
STRIP = strip
E =
O = .o
M = unix
SHELL = /bin/sh
MAKEF = -f unix/Makefile

# Version info for unix/unix.c
HOST_VERSINFO=-DIZ_CC_NAME='\"\$$(CC) \"' -DIZ_OS_NAME='\"`uname -a`\"'

# defaults for crc32 stuff and system dependent headers
CRCA_O =
OSDEP_H = unix/unxcfg.h
# default for dependency on auto-configure result, is an empty symbol
# so that the static non-autoconfigure targets continue to work
ACONF_DEP =

# optional inclusion of bzip2 decompression
IZ_OUR_BZIP2_DIR = bzip2
IZ_BZIP2 = $(IZ_OUR_BZIP2_DIR)
## The following symbols definitions need to be set to activate bzip2 support:
#D_USE_BZ2 = -DUSE_BZIP2
#L_BZ2 = -lbz2
#LIBBZ2 = $(IZ_BZIP2)/libbz2.a

# defaults for unzip's "built-in" bzip2 library compilation
CC_BZ = $(CC)
CFLAGS_BZ = $(CFLAGS)

# object files
OBJS1 = unzip$O crc32$O $(CRCA_O) crypt$O envargs$O explode$O
OBJS2 = extract$O fileio$O inflate$O list$O match$O
OBJS3 = process$O ttyio$O unshrink$O
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $M$O
LOBJS = $(OBJS)
OBJSDLL = $(OBJS:.o=.pic.o) api.pic.o
OBJX = crc32_$O $(CRCA_O) crypt_$O extract_$O fileio_$O \
	inflate_$O match_$O process_$O ttyio_$O $M_$O
LOBJX = $(OBJX)
OBJF = crc32$O $(CRCA_O) cryptf$O inflatef$O ttyiof$O
#OBJS_OS2 = $(OBJS1:.o=.obj) $(OBJS2:.o=.obj) os2.obj
#OBJF_OS2 = $(OBJF:.o=.obj)
UNZIP_H = unzip.h unzpriv.h globals.h $(OSDEP_H) $(ACONF_DEP)

# installation
# (probably can change next two to `install' and `install -d' if you have it)
INSTALL = cp
INSTALL_PROGRAM = $(INSTALL)
INSTALL_D = mkdir -p
# on some systems, manext=l and MANDIR=/usr/man/man$(manext) may be appropriate
manext = 1
prefix = /usr/local
BINDIR = $(prefix)/bin#			where to install executables
MANDIR = $(prefix)/man/man$(manext)#	where to install man pages
INSTALLEDBIN = $(BINDIR)/unzip$E
INSTALLEDMAN = $(MANDIR)/unzip.$(manext)

# Solaris 2.x stuff:
PKGDIR = IZunzip
VERSION = Version 6.0

UNZIPS = unzip$E
# this is a little ugly...well, OK, it's a lot ugly:
MANS = man/unzip.1

# list of supported systems/targets in this version
SYSTEMG1 = generic generic_gcc  generic_pkg generic_gccpkg
SYSTEMG2 = generic1 generic2 generic3 generic_bz2 generic_zlib generic_shlib
SYSTEMS1 = 386i 3Bx 7300 7300_gcc aix aix_rt amdahl amdahl_eft apollo aviion
SYSTEMS2 = bsd bsdi bsdi_noasm bull coherent convex cray cray_opt cyber_sgi
SYSTEMS3 = cygwin dec dnix encore eta freebsd gcc gould hk68 hp hpux
SYSTEMS4 = isc isc_gcc isi linux linux_dos linux_noasm linux_shlib linux_shlibz
SYSTEMS5 = lynx macosx macosx_gcc minix mips mpeix next next10 next2x next3x
SYSTEMS6 = nextfat osf1 pixel ptx pyramid qnxnto realix regulus rs6000 sco
SYSTEMS7 = sco_dos sco_sl sco_x286 sequent sgi solaris solaris_pkg stardent
SYSTEMS8 = stellar sunos3 sunos4 sysv sysv_gcc sysv6300 tahoe ti_sysv ultrix
SYSTEMS9 = vax v7 wombat xenix xos


####################
# DEFAULT HANDLING #
####################

# By default, print help on which makefile targets to try.  (The SYSTEM
# variable is no longer supported; use "make <target>" instead.)

help:
	@echo ""
	@echo "  If you're not sure about the characteristics of your system, try typing"
	@echo "  \"make generic\".  This is new and uses the configure script, though it is"
	@echo "  still being worked on."
	@echo ""
	@echo "  If that does not do it, try the original generic which is \"make generic1\"."
	@echo ""
	@echo "  If the compiler barfs and says something unpleasant about \"timezone redefined\","
	@echo "  try typing \"make clean\" followed by \"make generic2\".  If, on the other"
	@echo "  hand, it complains about an undefined symbol _ftime, try typing \"make clean\""
	@echo "  followed by \"make generic3\"."
	@echo ""
	@echo "  One of these actions should produce a working copy of unzip on most Unix"
	@echo "  systems.  If you know a bit more about the machine on which you work, you"
	@echo "  might try \"make list\" for a list of the specific systems supported herein."
	@echo "  (Many of them do exactly the same thing, so don't agonize too much over"
	@echo "  which to pick if two or more sound equally likely.)  Also check out the"
	@echo "  INSTALL file for notes on compiling various targets.  As a last resort,"
	@echo "  feel free to read the numerous comments within the Makefile itself."
	@echo ""
	@echo "  Have a mostly pretty good day."
	@echo ""

list:
	@echo ""
	@echo\
 'Type "make <system>", where <system> is one of the following:'
	@echo ""
	@echo  "	$(SYSTEMG1)"
	@echo  "	$(SYSTEMG2)"
	@echo ""
	@echo  "	$(SYSTEMS1)"
	@echo  "	$(SYSTEMS2)"
	@echo  "	$(SYSTEMS3)"
	@echo  "	$(SYSTEMS4)"
	@echo  "	$(SYSTEMS5)"
	@echo  "	$(SYSTEMS6)"
	@echo  "	$(SYSTEMS7)"
	@echo  "	$(SYSTEMS8)"
	@echo  "	$(SYSTEMS9)"
#	@echo ""
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
.SUFFIXES:	.c .o .obj .pic.o

# yes, we should be able to use the $O macro to combine these two, but it
# fails on some brain-damaged makes (e.g., AIX's)...no big deal
.c.o:
	$(CC) -c $(CF) $*.c

.c.obj:
	$(CC) -c $(CF) $*.c

.c.pic.o:
	$(CC) -c $(CF) -o $@ $*.c

# this doesn't work...directories are always a pain with implicit rules
#.1.txt:		man/$<
#	nroff -Tman -man $< | col -b | uniq | \
#	 sed 's/Sun Release ..../Info-ZIP        /' > $@


# these rules may be specific to Linux (or at least the GNU groff package)
# and are really intended only for the authors' use in creating non-Unix
# documentation files (which are provided with both source and binary
# distributions).  We should probably add a ".1.txt" rule for more generic
# systems...

unzip.txt:	man/unzip.1
	nroff -Tascii -man man/unzip.1 | col -bx | uniq | expand > $@

all:		generic_msg generic
unzips:		$(UNZIPS)
objs:		$(OBJS)
objsdll:	$(OBJSDLL)
docs:		$(DOCS)
unzipsman:	unzips docs
unzipsdocs:	unzips docs


# EDIT HERE FOR PARALLEL MAKES on Sequent (and others?)--screws up MS-DOS
# make utilities if default:  change "unzip$E:" to "unzip$E:&"

unzip$E:	$(OBJS) $(LIBBZ2)	# add `&' for parallel makes
	$(LD) $(LF) -L$(IZ_BZIP2) $(LOBJS) $(L_BZ2) $(LF2)

# when the optional bzip2 support is provided (as recommended) by sources
# in the 'bzip2' subdirectory, create/update the library:
$(IZ_OUR_BZIP2_DIR)/libbz2.a:
	@echo "Building/updating bzip2 object library..."
	( cd $(IZ_OUR_BZIP2_DIR) ; $(MAKE) -f Makebz2.iz CC="$(CC_BZ)"\
	 CFLAGS="$(CFLAGS_BZ)" RM="rm -f" )


crc32$O:	crc32.c $(UNZIP_H) crc32.h
crypt$O:	crypt.c $(UNZIP_H) crypt.h crc32.h ttyio.h
envargs$O:	envargs.c $(UNZIP_H)
explode$O:	explode.c $(UNZIP_H)
extract$O:	extract.c $(UNZIP_H) crc32.h crypt.h
fileio$O:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
inflate$O:	inflate.c $(UNZIP_H)
list$O:		list.c $(UNZIP_H)
match$O:	match.c $(UNZIP_H)
process$O:	process.c $(UNZIP_H) crc32.h
ttyio$O:	ttyio.c $(UNZIP_H) crypt.h ttyio.h
unshrink$O:	unshrink.c $(UNZIP_H)
unzip$O:	unzip.c $(UNZIP_H) crypt.h

# optional assembler replacements
crc_i86$O:	msdos/crc_i86.asm				# 16bit only
	$(AS) $(AF) msdos/crc_i86.asm $(ASEOL)

crc_gcc$O:	crc_i386.S $(ACONF_DEP)				# 32bit, GNU AS
	$(AS) $(AF) -x assembler-with-cpp -c -o $@ crc_i386.S

crc_gcc.pic.o:	crc_i386.S $(ACONF_DEP)				# 32bit, GNU AS
	$(AS) $(AF) -x assembler-with-cpp -c -o $@ crc_i386.S

crc_sysv$O:	crc_i386.S $(ACONF_DEP)				# 32bit, SysV AS
	$(CC) -E $(AF) crc_i386.S > crc_i386s.s
	$(AS) -o $@ crc_i386s.s
	$(RM) crc_i386s.s

unix$O:		unix/unix.c $(UNZIP_H) # Unix only
	$(CC) -c $(CF) unix/unix.c

unix.pic.o:	unix/unix.c $(UNZIP_H) # Unix shlib
	$(CC) -c $(CF) -o $@ unix/unix.c


unix_make:
#	@echo\
# '(Ignore any errors from `make'"' due to the following command; it's harmless.)"
	-@2>&1 $(LN) unix/Makefile . > /dev/null || echo > /dev/null

# this really only works for Unix targets, unless E and O specified on cmd line
clean:
	@echo ""
	@echo '         This is a Unix-specific target.  (Just so you know.)'
	@echo ""
	-( cd $(IZ_OUR_BZIP2_DIR); $(MAKE) -f Makebz2.iz RM="rm -f" clean )
	rm -f $(UNZIPS) $(OBJS) $(OBJF) $(OBJX) api$O apihelp$O crc_gcc$O \
	  crc_sysv$O unzipstb$O crypt_.c extract_.c inflate_.c \
	  ttyio_.c crc_i386s.s msdos_.c process_.c unix_.c unzipsfx.c
	rm -f flags
	rm -rf ./$(PKGDIR)

# Package generation interface (by J.Bush).  Originally tested under Sun
# Solaris 2.x.  Other SVr4s may be very similar and could possibly use this.
# Note:  expects version info to be stored in VERSION macro variable.
# See "README" under ./unix/Packaging
#
svr4package:	unzips
	@echo "Creating SVR4 package for Unix ..."
	-@rm -rf ./$(PKGDIR) ./$(PKGDIR)_`uname -p`.pkg
	-@sed -e "s/.VERSION./$(VERSION)/g" \
	      -e "s/.PSTAMP./$(LOGNAME)_`date | tr  ' ' '_'`/g" \
	      -e "s/.ARCH./Solaris_`uname -rp | tr ' ' ','`/g" \
	      ./unix/Packaging/pkginfo.in > ./unix/Packaging/pkginfo
	-@sed -e "s/.ARCH./`uname -p`/g" \
	      ./unix/Packaging/preinstall.in > ./unix/Packaging/preinstall
	/usr/bin/pkgmk -d . -b . -r . -f ./unix/Packaging/prototype $(PKGDIR)
	/usr/bin/pkgtrans -o -s . $(PKGDIR)_`uname -p`.pkg $(PKGDIR)
	@echo " "
	@echo "To install, copy $(PKGDIR)_`uname -p`.pkg to the target system, and"
	@echo "issue the command (as root):  pkgadd -d $(PKGDIR)_`uname -p`.pkg"
	@echo " "

install:	$(MANS)
	-$(INSTALL_D) $(BINDIR)
	$(INSTALL_PROGRAM) $(UNZIPS) $(BINDIR)
	-$(INSTALL_D) $(MANDIR)
	$(INSTALL) man/unzip.1 $(MANDIR)/unzip.$(manext)
	$(CHMOD) $(BINPERMS) $(INSTALLEDBIN)
	$(CHMOD) $(MANPERMS) $(INSTALLEDMAN)

uninstall:
	$(RM) $(INSTALLEDBIN) $(INSTALLEDMAN)

# added 10/28/04 EG
flags:  unix/configure
	sh unix/configure "${CC}" "${CF_NOOPT}" "${IZ_BZIP2}"

# the test zipfile
TESTZIP = testmake.zip

# test some basic features of the build
test:		check

check:
	@echo '#####  This is a Unix-specific target.  (Just so you know.)'
	@echo '#####     Make sure unzip, is compiled and in this directory.'
	@if test ! -f ./unzip; then \
	    echo "#####  ERROR:  can't find ./unzip"; exit 1; fi
#
	@if test ! -f $(TESTZIP); then \
	    echo "#####  ERROR:  can't find test file $(TESTZIP)"; exit 1; fi
#
	@echo "#####  testing extraction"
	@./unzip -bo $(TESTZIP) testmake.zipinfo
	@if test ! -f testmake.zipinfo ; then \
	    echo "#####  ERROR:  file extraction from $(TESTZIP) failed"; \
	    exit 1; fi
#
	@$(RM) testmake.unzip-Z testmake.zipinfo
#
	@echo '#####  testing unzip -d exdir option'
	@./unzip -bo $(TESTZIP) -d testun notes
	@cat testun/notes
#
	@echo '#####  testing unzip'
	@./unzip -boq $(TESTZIP) notes -d testun
#
	@rmdir testun
#
	@echo '#####  testing complete.'


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

#----------------------------------------------------------------------------
#  Generic targets using the configure script to determine configuration.
#----------------------------------------------------------------------------

# Well, try MAKE and see.  By now everyone may be happy.  10/28/04 EG
generic:	flags	   # now try autoconfigure first
	eval $(MAKE) $(MAKEF) unzips ACONF_DEP=flags `cat flags`
#	make $(MAKEF) unzips CF="${CF} `cat flags`"

generic_gcc:
	$(MAKE) $(MAKEF) generic CC=gcc IZ_BZIP2="$(IZ_BZIP2)"

# extensions to perform SVR4 package-creation after compilation
generic_pkg:	generic svr4package
generic_gccpkg:	generic_gcc svr4package

#----------------------------------------------------------------------------
#  Old static generic targets (can't assume make utility groks "$(MAKE)")
#----------------------------------------------------------------------------

generic1:	unzips	   # first try if unknown

generic2:	unix_make  # second try if unknown:  hope make is called "make"
	make $(MAKEF) unzips CF="$(CF) -DBSD"

generic3:	unix_make  # third try if unknown:  hope make is called "make"
	make $(MAKEF) unzips CF="$(CF) -DSYSV"

# Generic build including bzip2 decompression support for unzip.
# Requires presence of the bzip2 sources in subdirectory bzip2.

generic_bz2:	unix_make
	@echo\
 "This target assumes bzip2 sources are available in subfolder bzip2/."
	$(MAKE) $(MAKEF) unzips D_USE_BZ2="-DUSE_BZIP2"\
	 L_BZ2="-lbz2" LIBBZ2="$(IZ_OUR_BZIP2_DIR)/libbz2.a" \
	 CC_BZ="$(CC)" CFLAGS_BZ="$(CFLAGS)"

# Generic unzip and funzip target using either shared or static zlib for
# inflate rather than the original UnZip version.  (libz was libgz prior
# to 0.94)  Need to figure out how to force unzipsfx to use static libz.

generic_zlib:	unix_make
	@echo\
 "This target assumes zlib (libz.a or libz.so.*) is already installed."
	$(MAKE) unzip CF="$(CF) -DUSE_ZLIB" LF2="-lz $(LF2)"

# Generic GNU C shared library.  This is an example of how to compile UnZip as
# a shared library.  (Doing so as a static library would be similar.)  See also
# the linux_shlib target.

generic_shlib:	unix_make
	@echo\
 'This target requires GNU C.  When done, do "setenv LD_LIBRARY_PATH `pwd`"'
	@echo\
 'or similar in order to test the shared library in place (with ./unzip_shlib ,'
	@echo\
 'which is UnZip linked with the DLL).  This target is an example only.'
	@echo ""
	$(MAKE) objsdll CC=gcc CFLAGS="-O3 -Wall -fPIC -DDLL"
	gcc -shared -Wl,-soname,libunzip.so.0 -o libunzip.so.0.4 $(OBJSDLL)
	$(RM) libunzip.so.0 libunzip.so
	$(LN) -s libunzip.so.0.4 libunzip.so.0
	$(LN) -s libunzip.so.0 libunzip.so
	gcc -c -O unzipstb.c
	gcc -o unzip_shlib unzipstb.o -L. -lunzip

# Generic BSDish Unix gcc.  ``The -O3 only works with later versions of gcc;
# you may have to use -O2 or -O for earlier versions.  I have no idea why
# -s causes this bug in gcc.''  [Bug:  "nm: unzip: no name list", "collect:
# /usr/bin/nm returned 1 exit status".]  If you don't have strip, don't
# worry about it (it just makes the executable smaller and can be replaced
# with "echo" instead).
#
gcc:		unix_make
	$(MAKE) unzips CC=gcc LD=gcc CFLAGS="-O3" LF2=""
	$(STRIP) $(UNZIPS)

