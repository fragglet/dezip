# Makefile for UnZip 5.40 and later:  Human68k with gcc        NIIMI Satoshi
#
# The original Makefile maybe works fine, but X680x0 is too slow
# to process it.  So I split out needed part.
#
# Last revised:  11 Feb 96
#
# 1999/09/23: Modified by Shimazaki Ryo.

ifeq "$(TARGET)" "X68030"
COPT = -m68020-40
AOPT = -m68020
LDFLAGS = -L/usr/local/lib/lib060
endif

CC = gcc2
CFLAGS = $(COPT) -Wall -O2 -I. -fomit-frame-pointer -fstrength-reduce \
         -DHAVE_TWONCALL_H -D__DOS_INLINE__ -DASM_CRC -DASM_INFLATECODES
#LDFLAGS = -Wl,-x
LIBS = -lhmem -lttyi -lsignal

AS = g2as
ASFLAGS = $(AOPT) -1 -c4 -y

# UnZipSFX flags
XC = -DSFX

# fUnZip flags
FC = -DFUNZIP

# object files
OBJS = unzip.o crc_68.o crctab.o crypt.o envargs.o explode.o extract.o \
	fileio.o globals.o inflate.o flate.o list.o match.o process.o \
	ttyio.o unreduce.o unshrink.o zipinfo.o human68k.o
OBJX = unzipsfx.o crc_68.o crctab.o crypt.o extract_.o fileio.o globals.o \
	inflate.o flate.o match.o process_.o ttyio.o human68k_.o
OBJF = funzip.o crc_68.o crypt_.o globals_.o inflate_.o flate_.o ttyio_.o

UNZIP_H = unzip.h unzpriv.h globals.h

UNZIPS = unzip.x unzipsfx.x funzip.x
DOCS = unzip.doc unzipsfx.doc zipinfo.doc funzip.doc

.c.o:
	$(CC) $(CFLAGS) -I. -c $< -o $@

# for debugging
.c.s:
	$(CC) $(CFLAGS) -c $< -o $@

all:		unzips
unzips:		$(UNZIPS)
docs:		$(DOCS)
unzipsman:	unzips docs
unzipsdocs:	unzips docs

clean:
	rm -f $(OBJS) $(OBJF) $(OBJX) $(UNZIPS)

unzip.x: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

unzipsfx.x: $(OBJX)
	$(CC) $(LDFLAGS) -o $@ $(OBJX) $(LIBS)

funzip.x: $(OBJF)
	$(CC) $(LDFLAGS) -o $@ $(OBJF) $(LIBS)

#crc32.o:	crc32.c $(UNZIP_H) zip.h
crctab.o:	crctab.c $(UNZIP_H) zip.h
crypt.o:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
envargs.o:	envargs.c $(UNZIP_H)
explode.o:	explode.c $(UNZIP_H)
extract.o:	extract.c $(UNZIP_H) crypt.h
fileio.o:	fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
funzip.o:	funzip.c $(UNZIP_H) crypt.h ttyio.h tables.h
globals.o:	globals.c $(UNZIP_H)
inflate.o:	inflate.c inflate.h $(UNZIP_H)
list.o:		list.c $(UNZIP_H)
match.o:	match.c $(UNZIP_H)
process.o:	process.c $(UNZIP_H)
ttyio.o:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
unreduce.o:	unreduce.c $(UNZIP_H)
unshrink.o:	unshrink.c $(UNZIP_H)
unzip.o:	unzip.c $(UNZIP_H) crypt.h version.h consts.h
zipinfo.o:	zipinfo.c $(UNZIP_H)

crypt_.o:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip
	$(CC) $(CFLAGS) $(FC) -c $< -o $@

extract_.o:	extract.c $(UNZIP_H) crypt.h			# unzipsfx
	$(CC) $(CFLAGS) $(XC) -c $< -o $@

globals_.o:	globals.c $(UNZIP_H)				# funzip
	$(CC) $(CFLAGS) $(FC) -c $< -o $@

human68k.o:	human68k/human68k.c $(UNZIP_H)
	$(CC) $(CFLAGS) -I. -c human68k/human68k.c -o $@

human68k_.o:	human68k/human68k.c $(UNZIP_H)			# unzipsfx
	$(CC) $(CFLAGS) $(XC) -I. -c human68k/human68k.c -o $@

inflate_.o:	inflate.c inflate.h $(UNZIP_H) crypt.h		# funzip
	$(CC) $(CFLAGS) $(FC) -c $< -o $@

process_.o:	process.c $(UNZIP_H)				# unzipsfx
	$(CC) $(CFLAGS) $(XC) -c $< -o $@

ttyio_.o:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip
	$(CC) $(CFLAGS) $(FC) -c $< -o $@

unzipsfx.o:	unzip.c $(UNZIP_H) crypt.h version.h consts.h	# unzipsfx
	$(CC) $(CFLAGS) $(XC) -c $< -o $@

crc_68.o:	human68k/crc_68.s
	$(AS) $(ASFLAGS) $< -o $@

flate.o:	human68k/flate.s human68k/g_offs.mac
	$(AS) $(ASFLAGS) $< -o $@

flate_.o:	human68k/flate.s human68k/g_offs_.mac		# funzip
	$(AS) $(ASFLAGS) $< -o $@ -sFUNZIP

human68k/g_offs.mac:	human68k/mkgoff.x
	human68k/mkgoff.x >! $@
	
human68k/g_offs_.mac:	human68k/mkgoff_.x			# funzip
	human68k/mkgoff_.x >! $@

human68k/mkgoff.x:	human68k/mkgoff.c $(UNZIP_H) crypt.h
	$(CC) $(CFLAGS) human68k/mkgoff.c -o $@

human68k/mkgoff_.x:	human68k/mkgoff.c $(UNZIP_H) crypt.h	# funzip
	$(CC) $(CFLAGS) human68k/mkgoff.c -o $@ $(FC)

# the test zipfile
TESTZIP = testmake.zip

# test some basic features of the build
test:	check

check:	unzips
	@if test ! -f $(TESTZIP); then \
	    echo "#####  ERROR:  can't find test file $(TESTZIP)"; exit 1; fi
#
	@echo "#####  testing extraction"
	@./unzip -bo $(TESTZIP) testmake.zipinfo
	@if test ! -f testmake.zipinfo ; then \
	    echo "#####  ERROR:  file extraction from $(TESTZIP) failed"; \
	    exit 1; fi
#
	@echo '#####  testing zipinfo (unzip -Z)'
	@./unzip -Z $(TESTZIP) > testmake.unzip-Z
	@if diff testmake.unzip-Z testmake.zipinfo; then echo "OK."; else \
	    echo "#####  WARNING:  zipinfo output doesn't match stored version"; \
	    echo '#####     (If the only difference is the file times, compare your'; \
	    echo '#####      timezone with the Central European timezone, which is one'; \
	    echo '#####      hour east of Greenwich but effectively 2 hours east'; \
	    echo '#####      during summer Daylight Savings Time.  The upper two'; \
	    echo '#####      lines should correspond to your local time when the'; \
	    echo '#####      files were created, on 19 November 1998 at 10:46pm CET.'; \
	    echo '#####      If the times are consistent, please ignore this warning.)'; \
	    fi
	@rm -f testmake.unzip-Z testmake.zipinfo
#
	@echo '#####  testing unzip -d exdir option'
	@./unzip -bo $(TESTZIP) -d testun notes
	@cat testun/notes
#
	@echo '#####  testing unzip -o and funzip (ignore funzip warning)'
	@./unzip -boq $(TESTZIP) notes -d testun
	@./funzip < $(TESTZIP) > testun/notes2
	@if diff testun/notes testun/notes2; then true; else \
	    echo '#####  ERROR:  funzip output disagrees with unzip'; fi
#
	@echo '#####  testing unzipsfx (self-extractor)'
	@cat unzipsfx.x $(TESTZIP) > testsfx.x
	@chmod 0700 testsfx.x
	@./testsfx -bo notes
	@if diff notes testun/notes; then true; else \
	    echo '#####  ERROR:  unzipsfx file disagrees with unzip'; fi
	@rm -f testsfx.x notes testun/notes testun/notes2
	@rmdir testun
#
	@echo '#####  testing complete.'

# EOF
