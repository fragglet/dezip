$ !
$ !		"Makefile" for VMS versions of unzip and zipinfo
$ !			(version:  crypt + no inflate)
$ !
$ ! Find out current disk and directory
$ !
$ my_name = f$env("procedure")
$ here = f$parse(my_name,,,"device") + f$parse(my_name,,,"directory")
$ set verify	! like "echo on", eh?
$ !
$ ! Do unzip:
$ !
$ gcc /undef=__STDC__ /def=(CRYPT) unzip
$ gcc /undef=__STDC__ /def=(CRYPT) crypt
$ gcc /undef=__STDC__ /def=(CRYPT) extract
$ gcc /undef=__STDC__ /def=(CRYPT) file_io
$ gcc /undef=__STDC__ /def=(CRYPT) 	mapname
$ gcc /undef=__STDC__ /def=(CRYPT) match
$ gcc /undef=__STDC__ /def=(CRYPT) misc
$ gcc /undef=__STDC__ /def=(CRYPT) unimplod
$ gcc /undef=__STDC__ /def=(CRYPT) unreduce
$ gcc /undef=__STDC__ /def=(CRYPT) unshrink
$ gcc /undef=__STDC__ /def=(CRYPT) vms
$ gcc /undef=__STDC__ /def=(CRYPT) VMSmunch
$ link unzip,crypt,extract,file_io,mapname,match,misc,-
	unimplod,unreduce,unshrink,vms,VMSmunch,-
 gnu_cc:[000000]gcclib.olb/lib,-
 sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
! Next line:  put a similar line (full pathname for unzip.exe) in login.com.
! Remember to include leading "$" before disk name.
$ unzip == "$''here'unzip.exe"		! set up symbol to use unzip
$ !
$ ! Do zipinfo:
$ !
$ gcc /undef=__STDC__ zipinfo
$ rename misc.c misc_.c;*
$ gcc /undef=__STDC__ /def=(ZIPINFO) misc_
$ rename misc_.c misc.c;*
$ link zipinfo,match,misc_,VMSmunch,-
 gnu_cc:[000000]gcclib.olb/lib,-
sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
! Next line:  put a similar line (full pathname for unzip.exe) in login.com.
! Remember to include leading "$" before disk name.
$ zipinfo == "$''here'zipinfo.exe"	! set up symbol to use zipinfo
$ set noverify
