$ !
$ !		"Makefile" for VMS versions of unzip and zipinfo
$ !			(version:  no crypt + no inflate)
$ !
$ ! Find out current disk and directory
$ !
$ my_name = f$env("procedure")
$ here = f$parse(my_name,,,"device") + f$parse(my_name,,,"directory")
$ set verify	! like "echo on", eh?
$ !
$ ! Do unzip:
$ !
$ gcc /undef=__STDC__ unzip
$ gcc /undef=__STDC__ extract
$ gcc /undef=__STDC__ file_io
$ gcc /undef=__STDC__ 	mapname
$ gcc /undef=__STDC__ match
$ gcc /undef=__STDC__ misc
$ gcc /undef=__STDC__ unimplod
$ gcc /undef=__STDC__ unreduce
$ gcc /undef=__STDC__ unshrink
$ gcc /undef=__STDC__ vms
$ gcc /undef=__STDC__ VMSmunch
$ link unzip,extract,file_io,mapname,match,misc,-
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
$ gcc /undef=_STDC__ /def=(ZIPINFO) misc_
$ rename misc_.c misc.c;*
$ link zipinfo,match,misc_,VMSmunch,-
gnu_cc:[000000]gcclib.olb/lib,-
sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
! Next line:  put a similar line (full pathname for unzip.exe) in login.com.
! Remember to include leading "$" before disk name.
$ zipinfo == "$''here'zipinfo.exe"	! set up symbol to use zipinfo
$ set noverify
