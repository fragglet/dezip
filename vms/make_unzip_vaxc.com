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
$ cc unzip,extract,file_io,-
	mapname,match,misc,unimplod,unreduce,unshrink,vms,VMSmunch
$ link unzip,extract,file_io,mapname,match,misc,-
	unimplod,unreduce,unshrink,vms,VMSmunch, sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
! Next line:  put a similar line (full pathname for unzip.exe) in login.com.
! Remember to include leading "$" before disk name.
$ unzip == "$''here'unzip.exe"		! set up symbol to use unzip
$ !
$ ! Do zipinfo:
$ !
$ cc zipinfo
$ rename misc.c misc_.c;*
$ cc /def=(ZIPINFO) misc_
$ rename misc_.c misc.c;*
$ link zipinfo,match,misc_,VMSmunch,sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
! Next line:  put a similar line (full pathname for unzip.exe) in login.com.
! Remember to include leading "$" before disk name.
$ zipinfo == "$''here'zipinfo.exe"	! set up symbol to use zipinfo
$ set noverify
