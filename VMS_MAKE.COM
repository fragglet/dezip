$ set verify	! like "echo on", eh?
$ define lnk$library sys$library:vaxcrtl.olb
$ cc nunzip,file_io,mapname,match,misc,unimplod,unreduce,unshrink
$ link nunzip,file_io,mapname,match,misc,unimplod,unreduce,unshrink,vmsshare/opt
$ set noverify
