#  Makefile.st	Makefile for UnZip 4.0,
#  using Turbo C 2.0 for the Atari ST and
#  make from the Mark Williams C 3.9 (sorry for that mixture)
#

# UPPERCASE file names facilitate TD's understanding of modules.
.SUFFIXES: .o .O .c .C .s

DISK	= E:
#####################
# MACRO DEFINITIONS #
#####################

CC	= tcc
#CFLAGS	= -DATARI_ST=1 -Y -I=$(DISK)\tc\include
CFLAGS	= -DATARI_ST=1 -I=$(DISK)\tc\include
#LFLAGS	= -L -Y
LFLAGS	=
LIB	= $(DISK)\tc\lib
LD	= tlink

OBJS = unzip.o file_io.o mapname.o match.o misc.o\
       unimplod.o unreduce.o unshrink.o


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

ALL	: unzip.prg
	echo done

unzip.o:      unzip.c unzip.h

file_io.o:    file_io.c unzip.h

mapname.o:    mapname.c unzip.h

match.o:      match.c unzip.h

misc.o:       misc.c unzip.h

unimplod.o:   unimplod.c unzip.h

unreduce.o:   unreduce.c unzip.h

unshrink.o:   unshrink.c unzip.h

unzip.prg:     $(OBJS)
	tlink -O=$@ $(LFLAGS) -S=4096 -C=tlink.opt

.c.o:
	tcc $(CFLAGS) $<

