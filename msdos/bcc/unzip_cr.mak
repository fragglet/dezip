.AUTODEPEND

#		*Translator Definitions*
CC = bcc +UNZIP_CR.CFG
TASM = TASM
TLINK = tlink


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
  unzip.obj \
  file_io.obj \
  mapname.obj \
  match.obj \
  misc.obj \
  unimplod.obj \
  unreduce.obj \
  unshrink.obj \
  extract.obj \
  crypt.obj

#		*Explicit Rules*
unzip_cr.exe: unzip_cr.cfg $(EXE_dependencies)
  $(TLINK) /x/n/c/d/P-/LC:\BORLAND\LIB @&&|
c0s.obj+
unzip.obj+
file_io.obj+
mapname.obj+
match.obj+
misc.obj+
unimplod.obj+
unreduce.obj+
unshrink.obj+
extract.obj+
crypt.obj
unzip_cr
		# no map file
emu.lib+
maths.lib+
cs.lib
|


#		*Individual File Dependencies*
unzip.obj: unzip.c 

file_io.obj: file_io.c 

mapname.obj: mapname.c 

match.obj: match.c 

misc.obj: misc.c 

unimplod.obj: unimplod.c 

unreduce.obj: unreduce.c 

unshrink.obj: unshrink.c 

extract.obj: extract.c 

crypt.obj: crypt.c 

#		*Compiler Configuration File*
unzip_cr.cfg: unzip_cr.mak
  copy &&|
-ff-
-A
-k-
-wamb
-wamp
-wasm
-wpro
-wdef
-wnod
-wstv
-wucp
-wuse
-IC:\BORLAND\INCLUDE
-LC:\BORLAND\LIB
-DCRYPT
-P-.C
| unzip_cr.cfg


