#  Makefile.DOS    Makefile for UnZip 4.x, using Microsoft C Compiler 5.0
#   [crypt]        and Microsoft MAKE 4.02 (or later versions), or Borland
#   [no inflate]   Turbo C 2.0.  Comment/uncomment the appropriate sections
#                  below and/or edit the Include and Library paths, if 
#                  necessary.  For Borland C++ use the newer project files
#                  which are also included (but read the comments in Contents).
#
#                  Users of MSC 6.0 and NMAKE should use the regular makefile
#                  by typing "nmake msc_dos" or "nmake msc_os2".
#
#  Notes:  (1) Uncomment the appropriate compiler/OS options below.
#          (2) Change the various CFLAGS as appropriate for your environment
#              and libraries.
#              For MSC:  -AS specifies small-model library, and -G2 enables
#              80286 instructions.  QuickC uses the medium memory model, as
#              I recall (-AM and/or -qc).  The "ALL:" target is used by MSC
#              6.0 with old MAKE.  [Bo Kullmar]
#          (3) Rename this file to "unzip" on a DOS system; typing "make
#              unzip" with MSC (or "make -funzip" with TC) then builds
#              unzip.exe.
#
#  Greg Roelofs
#

#####################
# MACRO DEFINITIONS #
#####################

# Borland C++ 2.0 for MS-DOS:
# ----------------------
# bcc is usually configured with -I and -L set appropriately...
# CC = bcc
# CFLAGS = -ms -O -Z -DCRYPT
# INCL = #-Ic:\borland\include
# LD = bcc
# LDFLAGS = -ms #-Lc:\borland\lib
# LDFLAGS2 =

# Turbo C 2.0 for MS-DOS:
# ----------------------
# tcc is usually configured with -I and -L set appropriately...
# CC = tcc
# CFLAGS = -ms -O -Z -DCRYPT
# INCL = #-Ic:\turboc\include
# LD = tcc
# LDFLAGS = -ms #-Lc:\turboc\lib
# LDFLAGS2 =

# MSC for MS-DOS:
# --------------
CC = cl
CFLAGS = -AS -Ox -G2 -DCRYPT  # add -FPi87 if coprocessor installed
INCL =
LD = link
LDFLAGS = /NOI
LDFLAGS2 = ,$*;

# MSC with SDK for OS/2:      # this target no longer supported:  use Makefile
# ---------------------       # (if you MUST use this target, add dosname.obj
# CC = cl                     #   to OBJS and the dependencies line below)
# CFLAGS = -AS -Ox -G2 -DOS2 -DCRYPT
# INCL = -Ic:\m5\include      # for example
# LD = link
# LDFLAGS = /NOI
# RM = del
# LIBC = c:\m5\lib\p\slibce
# LIBD = c:\m5\lib\doscalls.lib
# LIBA = c:\m5\lib\api.lib

OBJS = unzip.obj crypt.obj extract.obj file_io.obj mapname.obj match.obj\
       misc.obj unimplod.obj unreduce.obj unshrink.obj


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

ALL	: unzip.exe

.c.obj:
        $(CC) -c $(CFLAGS) $(INCL) $*.c

unzip.obj:      unzip.c unzip.h

crypt.obj:      crypt.c unzip.h zip.h

extract.obj:    extract.c unzip.h

file_io.obj:    file_io.c unzip.h

mapname.obj:    mapname.c unzip.h

match.obj:      match.c unzip.h

misc.obj:       misc.c unzip.h

unimplod.obj:   unimplod.c unzip.h

unreduce.obj:   unreduce.c unzip.h

unshrink.obj:   unshrink.c unzip.h

# DOS:
# ---
unzip.exe:     $(OBJS)
        $(LD) $(LDFLAGS) $(OBJS) $(LDFLAGS2)

# OS/2:
# ----
# unziptmp.exe:  $(OBJS)
#         $(LD) $(LDFLAGS) $(OBJS), $*.exe,,$(LIBC)+$(LIBD);
#
# unzip.exe:     unziptmp.exe
#         bind unziptmp.exe $(LIBD) $(LIBA) -o unzip.exe
#         $(RM) unziptmp.exe
#         $(RM) *.obj
#         $(RM) *.map
