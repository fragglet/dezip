.AUTODEPEND

#		*Translator Definitions*
CC = bcc +ZIPINFO.CFG
TASM = TASM
TLINK = tlink


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
  zipinfo.obj \
  match.obj \
  misc.obj

#		*Explicit Rules*
zipinfo.exe: zipinfo.cfg $(EXE_dependencies)
  $(TLINK) /x/n/c/d/P-/LC:\BORLAND\LIB @&&|
c0s.obj+
zipinfo.obj+
match.obj+
misc.obj
zipinfo
		# no map file
emu.lib+
maths.lib+
cs.lib
|


#		*Individual File Dependencies*
zipinfo.obj: zipinfo.c 

match.obj: match.c 

misc.obj: misc.c 

#		*Compiler Configuration File*
zipinfo.cfg: zipinfo.mak
  copy &&|
-ff-
-A
-K
-k-
-d
-wamb
-wamp
-wasm
-wpro
-wcln
-wdef
-wsig
-wnod
-wstv
-wucp
-wuse
-IC:\BORLAND\INCLUDE
-LC:\BORLAND\LIB
-DZIPINFO
-P-.C
| zipinfo.cfg


