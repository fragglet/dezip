#==============================================================================
# Makefile for UnZip, ZipInfo & fUnZip:  Atari ST                Chris Herborth
# Version:  UnZip 5.1, MiNT, GNU C                              6 February 1994
#==============================================================================

# Based on the original unix Makefile and modified by Chris Herborth
# (cherborth@semprini.waterloo-rdp.on.ca), Nov.13/93.
#
# $Id: Makefile 1.7 1993/12/28 18:52:52 root Exp $

# Be sure to test your new UnZip (and ZipInfo and fUnZip); successful compila-
# tion does not always imply a working program.


#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use (use LOCAL_UNZIP in environment to add flags, 
# such as -DDOSWILD).

# UnZip flags
CC = gcc
LD = gcc
LOC = $(LOCAL_UNZIP) -ansi -D__MINT__ -U__STRICT_ANSI__

CF = -mbaserel -mpcrel -O -fomit-frame-pointer -I. $(LOC)
# CF = -O -I. $(LOC)
# CF = -mbaserel -O -I. $(LOC)
LF = -mbaserel -mpcrel -o unzip.ttp
LF2 = -s

# fUnZip flags
FC = -DFUNZIP
FL = -mbaserel -mpcrel -o funzip.ttp
FL2 = $(LF2)

# general-purpose stuff
CP = cp
LN = ln -s
RM = rm -f
E = .ttp
O = .o
# M = unix
M = atari
SHELL = /bin/sh

# object files
OBJS1 = unzip$O crypt$O envargs$O explode$O extract$O file_io$O
OBJS2 = inflate$O match$O unreduce$O unshrink$O zipinfo$O
OBJS = $(OBJS1) $(OBJS2) $M$O
LOBJS = $(OBJS)
OBJF = funzip$O crypt_$O inflate_$O

# installation
INSTALL = cp#	probably can change this to 'install' if you have it
# on some systems, manext=l and MANDIR=/usr/man/man$(manext) may be appropriate
manext = 1
MANDIR = /usr/local/man/man$(manext)#	where to install man pages
BINDIR = /usr/local/bin#		where to install executables
#
UNZIPS = unzip$E funzip$E zipinfo$E
MANS = unzip.$(manext) zipinfo.$(manext) funzip.$(manext)
DOCS = unzip.doc zipinfo.doc funzip.doc
# this is a little ugly...
INSTALLED = $(BINDIR)/unzip$E $(BINDIR)/zipinfo$E $(BINDIR)/funzip$E \
	$(MANDIR)/unzip.$(manext) $(MANDIR)/zipinfo.$(manext) \
	$(MANDIR)/funzip.$(manext)

###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

.c$O:
	$(CC) -c $(CF) $*.c

.1.doc:
	nroff -Tman -man $< | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@


all:		unzips
unzips:		$(UNZIPS)
docs:		$(DOCS)
unzipsman:	unzips docs
unzipsdocs:	unzips docs


clean:
	rm -f $(OBJS) unzip$E $(OBJF) funzip$E zipinfo$E

install:	$(UNZIPS) $(MANS)
	$(INSTALL) $(UNZIPS) $(BINDIR)
	$(LN) $(BINDIR)/unzip$E $(BINDIR)/zipinfo$E
	$(INSTALL) unzip.1 $(MANDIR)/unzip.$(manext)
	$(INSTALL) zipinfo.1 $(MANDIR)/zipinfo.$(manext)
	$(INSTALL) funzip.1 $(MANDIR)/funzip.$(manext)

# alternatively, could use zip method:  -cd $(BINDIR); rm -f $(UNZIPS)  [etc.]
uninstall:
	rm -f $(INSTALLED)


unzip$E:	$(OBJS)			# add `&' if parallel makes supported
	$(LD) $(LF) $(LOBJS) $(LF2)

funzip$E:	$(OBJF)			# add `&' if parallel makes supported
	$(LD) $(FL) $(OBJF) $(FL2)

zipinfo$E:	unzip$E
	@echo\
 '  This is a Unix-inspired target.  If your filesystem does not support'
	@echo\
 '  symbolic links, copy unzip.ttp to zipinfo.ttp rather than linking it,'
	@echo\
 '  or else invoke as "unzip -Z".'
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

crypt_$O:	crypt.c unzip.h zip.h crypt.h			# funzip only
	$(CP) crypt.c crypt_.c
	$(CC) -c $(CF) $(FC) crypt_.c
	$(RM) crypt_.c

inflate_$O:	inflate.c inflate.h unzip.h crypt.h		# funzip only
	$(CP) inflate.c inflate_.c
	$(CC) -c $(CF) $(FC) inflate_.c
	$(RM) inflate_.c

# atari$O:	atari/atari.c unzip.h				# Atari only
#	$(CC) -c $(CF) atari/atari.c

atari$O:	atari.c unzip.h
