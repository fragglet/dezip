#============================================================================
# Makefile for UnZip 5.1 and later                               Greg Roelofs
# Version:  MAKE/VMS 3.4 [Todd Aven]                               7 May 1992
#
# Most recent revisions:  21 Dec 93


#####################
# MACRO DEFINITIONS #
#####################

LOCAL_UNZIP =

CC = cc
CFLAGS = $(LOCAL_UNZIP)			# use "cc $(CFLAGS)" this way
# CFLAGS := /def=(FLAG1,FLAG2)		# this also works, if use "cc 'CFLAGS'"
LD = link
LDFLAGS =

LN = copy
RM = delete
E = .exe
O = .obj

OBJS = unzip$O, crypt.obj, envargs$O, explode$O, extract$O, file_io$O,-
   inflate$O, match$O, unreduce$O, unshrink$O, zipinfo$O, vms$O


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

default:	all
all:		unzip.exe unzip.hlp

*.obj:	*.c				# `*.c' necessary?
	$(CC) $(CFLAGS) $<

unzip$E:	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS), sys$input:/opt      # next line is data line
	sys$share:vaxcrtl.exe/shareable $

crypt$O:        crypt.c unzip.h zip.h crypt.h
envargs$O:      envargs.c unzip.h
explode$O:      explode.c unzip.h
extract$O:      extract.c unzip.h crypt.h
file_io$O:      file_io.c unzip.h crypt.h tables.h
inflate$O:      inflate.c inflate.h unzip.h
match$O:        match.c unzip.h
unreduce$O:     unreduce.c unzip.h
unshrink$O:     unshrink.c unzip.h
unzip$O:        unzip.c unzip.h
zipinfo$O:	zipinfo.c unzip.h

vms$O:          [.vms]vms.c [.vms]vms.h unzip.h
	$(CC) $(CFLAGS) /include=sys$disk:[] [.vms]vms.c

unzip.hlp:	[.vms]unzip.rnh
	runoff /out=unzip.hlp [.vms]unzip.rnh

# system: ${SYSDIR}make.exe ${SYSDIR}make.ini
# debug:
# 	cflags := /debug/noopt
# 	lflags := /debug
# ${DISTDIR}unzip.exe: unzip.exe
#	copy unzip.exe ${DISTDIR}/log

clean:
	purge/log
#	purge/log ${DISTDIR}


# the backslash '\' is the continuation character if it occurs as
# the last non-white character on the line.
# the hyphen '-' is the DCL continuation character, so if it occurs
# as the last non-white character on the line, the next line will
# not have the dollar sign '$' prepended.


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

generic:	default		# first try if unknown
generic2:	default		# second try if unknown
vax:		default
vms:		default

both:		unzip.exe
unzip:		unzip.exe
