# Makefile for UnZip 4.2 using SAS/C 5.10a
# [crypt + no inflate]
#
# Not tested since UnZip 4.1.  May need to change directory names for stat.c
# and utime.c.

#####################
# MACRO DEFINITIONS #
#####################

CC = lc
CFLAGS = -O -DUNIX -DCRYPT -v -m0t -cuaisfr -rr
LD = blink
LDFLAGS = TO unzip FROM LIB:c.o
LDFLAGS2 = LIB LIB:lc.lib LIB:amiga.lib
EXE =
O = .o
OBJS = unzip$O crypt$O extract$O file_io$O mapname$O match$O misc$O\
       unimplod$O unreduce$O unshrink$O utime$O stat$O

###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

.c$O :
        $(CC) -o$@ $(CFLAGS) $*.c

unzip$(EXE):    $(OBJS)
        $(LD) $(LDFLAGS) $(OBJS) $(LDFLAGS2)

unzip$O:        unzip.c unzip.h
crypt$O:        crypt.c unzip.h zip.h	# may or may not be in distribution
extract$O:      extract.c unzip.h
file_io$O:      file_io.c unzip.h
inflate$O:      inflate.c unzip.h	# may or may not be in distribution
mapname$O:      mapname.c unzip.h
match$O:        match.c unzip.h
misc$O:         misc.c unzip.h
unimplod$O:     unimplod.c unzip.h
unreduce$O:     unreduce.c unzip.h
unshrink$O:     unshrink.c unzip.h
stat$O:         amiga/stat.c	# may need to change or remove directory name
utime$O:        amiga/utime.c	# may need to change or remove directory name
