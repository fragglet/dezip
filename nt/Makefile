# NMAKE Makefile for Windows NT
# D. Feinleib 7/92 t-davefe@microsoft.com
# H. Gessau 9/93 <henryg@kullmar.kullmar.se>
#
# Last revised: 25 Jan 1994.
# Tested with NT SDK. Not tested with VC++ for NT.

# Nmake macros for building Windows NT applications
# To build with debug info use 'nmake debug=1'
!IFNDEF debug
NODEBUG=1
!ENDIF
!include <ntwin32.mak>

# object files
OBJS1 = unzip.obj crypt.obj envargs.obj explode.obj extract.obj file_io.obj
OBJS2 = inflate.obj match.obj unreduce.obj unshrink.obj zipinfo.obj
OBJS  = $(OBJS1) $(OBJS2) nt.obj
OBJF  = funzip.obj crypt_.obj inflate_.obj

# cvars = $(cvars) -nologo -DMSDOS -DNO_ASM -J
cvars = $(cvars) -nologo -DNO_ASM -J

# Some optimization (if not debugging)
!IFDEF NODEBUG
cdebug = -Ogityb2
!ENDIF

# How to compile sources
.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $<

# How to link
.obj.exe:
	$(link) $(ldebug) $(conflags) $(conlibs) $** -out:$@


# Default target is all the executables
unzips : unzip.exe funzip.exe	# zipinfo.exe ?


crypt.obj:	crypt.c unzip.h zip.h crypt.h
envargs.obj:	envargs.c unzip.h
explode.obj:	explode.c unzip.h
extract.obj:	extract.c unzip.h crypt.h
file_io.obj:	file_io.c unzip.h crypt.h tables.h
funzip.obj:	funzip.c unzip.h crypt.h tables.h
inflate.obj:	inflate.c inflate.h unzip.h
match.obj:	match.c unzip.h
unreduce.obj:	unreduce.c unzip.h
unshrink.obj:	unshrink.c unzip.h
unzip.obj:	unzip.c unzip.h
zipinfo.obj:	zipinfo.c unzip.h

nt.obj: 	nt\nt.c unzip.h					# NT only
	$(cc) $(cdebug) $(cflags) $(cvars) -I. nt\nt.c
	
crypt_.obj:	crypt_.c					# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP crypt_.c
	del crypt_.c

inflate_.obj:	inflate_.c					# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP inflate_.c
	del inflate_.c

crypt_.c:	crypt.c unzip.h zip.h crypt.h
	copy crypt.c crypt_.c

inflate_.c:	inflate.c inflate.h unzip.h crypt.h
	copy inflate.c inflate_.c


unzip.exe:	$(OBJS)

funzip.exe:	$(OBJF)

zipinfo.exe:	$(OBJI)

clean:
	del /q $(OBJS) unzip.exe $(OBJF) funzip.exe
