# From: Richard A Seay <g0rick@cdf.toronto.edu>, 4 Apr 92
#
# I successfully compiled both Zip and UnZip for MSDOS using the Microsoft
# Quick C 2.0 compiler and linker.  Here are the NMAKE files that I used.
#
# Last revised:  14 Jan 94  (still no funzip)

PROJ	= UNZIP
CC	= qcl
CFLAGS  = /AL /W1 /Za /Olt /DMSC
LFLAGS  = /noi /e
OBJS_EXT =
LIBS_EXT =

all:	$(PROJ).exe

unzip.obj:      unzip.c unzip.h

crypt.obj:      crypt.c unzip.h zip.h crypt.h

envargs.obj:    envargs.c unzip.h

explode.obj:    explode.c unzip.h

extract.obj:    extract.c unzip.h crypt.h

file_io.obj:    file_io.c unzip.h crypt.h tables.h

inflate.obj:    inflate.c unzip.h

match.obj:      match.c unzip.h

msdos.obj:      msdos/msdos.c unzip.h
	$(CC) -c $(CFLAGS) /I. msdos/msdos.c

unreduce.obj:   unreduce.c unzip.h

unshrink.obj:   unshrink.c unzip.h

zipinfo.obj:    zipinfo.c unzip.h

$(PROJ).exe:	unzip.obj crypt.obj envargs.obj explode.obj extract.obj\
	file_io.obj inflate.obj match.obj unreduce.obj\
	unshrink.obj zipinfo.obj msdos.obj $(OBJS_EXT)
	echo >NUL @<<$(PROJ).rsp
unzip.obj +
crypt.obj +
envargs.obj +
explode.obj +
extract.obj +
file_io.obj +
inflate.obj +
match.obj +
msdos.obj +
unreduce.obj +
unshrink.obj +
zipinfo.obj +
$(OBJS_EXT)
$(PROJ).exe
$(LIBS_EXT);
<<
	link $(LFLAGS) @$(PROJ).rsp
	del $(PROJ).rsp
