# DMakefile for UnZip 4.2 with Amiga DICE compiler
# Version:  crypt + no inflate   [29 Feb 92]
#
# Not tested for v4.2.  Edit directories as required.
#
#Georg Sassen, D-5100 Aachen,+49-241-875158 subnet: georg@bluemoon.tunix.sub.org
#georg@bluemoon.GUN.de, georg@cip-s01.informatik.rwth-aachen.de, 2:242/7.11@fido

EXE = DH0:bin/unzip
OD  = dtmp:unzip/
SRC1 = unzip.c crypt.c extract.c file_io.c mapname.c match.c misc.c
SRC2 = unimplod.c unreduce.c unshrink.c
SRCS = $(SRC1) $(SRC2)
CFLAGS = -mD -DCRYPT
OBJS = $(SRCS:"*.c":"$(OD)*.o") $(SRCS:"*.a":"$(OD)*.o")

all : $(EXE)

$(OBJS) : $(SRCS) unzip.h zip.h
    dcc $(CFLAGS) -c -o %(left) %(right)

$(EXE) : $(OBJS)
    dcc $(CFLAGS) -o %(left) %(right)

clean:
    rm -v $(OBJS)
