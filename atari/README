Here it is, finally...  the diffs to UnZip 5.1 for the Atari ST/TT/Falcon
platform!

This took me longer than I expected thanks to a *problem* with the MiNT
library (as of pl.41 it's still there)...  symlinks are 0x16000 instead
of 0xa000; I hope this isn't due to compatibilty with DOS filesystems
(bleh).

These diffs require GNU C, and allow you to build an unzip/zipinfo/funzip
that supports long filenames (on appropriate filesystems, thanks to the 
MiNT library), as well as symlinks.  It also does "proper" (ie, DOS-style)
translation of text-file end-of-line characters.  The programs also build 
as shared-text binaries, so you can start unzipping several files at once 
in the background and only a small part of unzip will be duplicated in 
memory.

I build unzip with the MiNT library (it's in much better shape than the
GNU C library and it's updated constantly... not to mention the fact
that it does all sorts of nifty things for you), GNU C 2.4.5 (2.5.x has
rather sever problems on the 68000, including the amazing ability to
produce code that _will_not_ work on a 68000... even in plain 68000
code-generation mode!  Yahoo...).  Oh, and I tested it extensively under
MiNT's minix filesystem as well as the bogus DOS filesystem that "normal"
Atari partitions have.

[...]

The Makefile won't need any editing if you want to built unzip et al. on
a minix filesystem; if you want to install it on a DOS filesystem, use
"cp" instead of "ln" for zipinfo.  [Or, to save disk space, make a script/
command-file which invokes "unzip -Z ...". --CN]

If you want to compile this using Turbo C (aka Pure C in North America)
you're on your own...  That's ok, I'll make a nice binary version of
unzip available as soon as the official 5.1 version is released (give or
take a few days).

Enjoy!  Cave Newt should be given kudos for keeping this monster easy to
port...  ;-)  [Hah!  But thanks anyway. :-) --CN]

-Chris-
cherborth@semprini.waterloo-rdp.on.ca
