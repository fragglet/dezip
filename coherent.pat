[These patches have been installed in version 3.10.
 They actually had to be manually installed in unzip.h since the dif files
 were intended for an earlier unzip.c where all the defines, etc. were
 still in the one file.
 For them to take effect, you must (of course) define COHERENT somewhere
 (like in your Makefile).  Our Makefile has NOT been enabled for Coherent.

 David Kirschbaum
 Toad Hall
]

From @WSMR-SIMTEL20.ARMY.MIL:kirsch@usasoc.soc.mil Wed Aug  8 19:23:51 1990
Received: from usasoc.soc.mil by WSMR-SIMTEL20.ARMY.MIL with TCP; Wed, 8 Aug 90 15:37:55 MDT
Date: Wed, 8 Aug 90 17:37:25 -0400
From: David Kirschbaum <kirsch>
Message-Id: <9008082137.AA03919@usasoc.soc.mil>
To: INFO-ZIP <info-zip@WSMR-Simtel20.Army.Mil>
Cc: Esa T. Aloha <msdc!esa@uunet.UU.NET>, kirsch <kirsch@usasoc.soc.mil>
Subject:  Re:  Unzip30 & Coherent
Status: R

Of COURSE you can distribute it!  We're talking public domain here, folks;
the good old-fashioned, "do anything you want with this sucker" kind...

I'm really kinda amazed at the relatively few diffs that came up in
porting it to Coherent .. really all quite forseeable stuff! Heck, we had
more trouble with flavors of true-blue Unix!

For the rest of the Info-ZIP people: I'll be posting the differences
(eventually), to be distributed as v3.08 (whenever ...  I'm waiting for
some Atari/TurboC patches from Germany).  I may throw in some v3.08
numbers in the diff patches just to help in tracking changes.

Incidentally, how many snarfers of the unzip307.tar-Z file from SIMTEL20
noticed that bug.lst somehow became a tar of all the OTHER files! Yep,
180K or so!  Donno HOW that happened!  Anyway, just uploaded a fixed
version, so SIMTEL archives are up to date per v3.07 anyway.

David Kirschbaum
Info-ZIP Coordinator

----- Forwarded Message Start

Received: from msdc.UUCP by uunet.uu.net (5.61/1.14) with UUCP
        id AA20573; Wed, 8 Aug 90 15:47:41 -0400
Message-Id: <9008081939.AA10689@msdc.msdc.com>
From: msdc!esa@uunet.UU.NET (Esa T. Ahola)
Date: Wed, 8 Aug 90 15:31:45 EDT
X-Mailer: Mail User's Shell (7.0.4 1/31/90)
To: kirsch@usasoc.soc.mil
Subject: Unzip30 & Coherent

I have ported Unzip 3.0 to Coherent (a V7-clone by Mark Williams Co) and
would like permission to distribute it to other Coherent users (we have
an active mailing list).  Diffs below:

Only in unzip: Makefile.mwc
diff -c unzip.orig/unzip.c unzip/unzip.c
*** unzip.orig/unzip.c  Thu May  3 17:02:50 1990
--- unzip/unzip.c       Wed Aug  8 15:05:14 1990
***************
*** 54,59

  #ifdef __STDC__

  #include <stdlib.h>
   /* this include defines various standard library prototypes */


--- 54,60 -----

  #ifdef __STDC__

+ #ifndef COHERENT
  #include <stdlib.h>
  #endif
   /* this include defines various standard library prototypes */
***************
*** 55,60
  #ifdef __STDC__

  #include <stdlib.h>
   /* this include defines various standard library prototypes */

  #else

--- 56,62 -----

  #ifndef COHERENT
  #include <stdlib.h>
+ #endif
   /* this include defines various standard library prototypes */

  #else
***************
*** 351,356
  #include <sys/param.h>
  #define ZSUFX ".zip"
  #ifndef BSIZE
  #define BSIZE DEV_BSIZE     /* v2.0c assume common for all Unix systems */
  #endif
  #ifndef BSD                 /* v2.0b */

--- 353,361 -----
  #include <sys/param.h>
  #define ZSUFX ".zip"
  #ifndef BSIZE
+ #ifdef COHERENT
+ #define BSIZE 512
+ #else
  #define BSIZE DEV_BSIZE     /* v2.0c assume common for all Unix systems */
  #endif
  #endif
***************
*** 353,358
  #ifndef BSIZE
  #define BSIZE DEV_BSIZE     /* v2.0c assume common for all Unix systems */
  #endif
  #ifndef BSD                 /* v2.0b */
  #include <time.h>
  struct tm *gmtime(), *localtime();

--- 358,364 -----
  #else
  #define BSIZE DEV_BSIZE     /* v2.0c assume common for all Unix systems */
  #endif
+ #endif
  #ifndef BSD                 /* v2.0b */
  #include <time.h>
  struct tm *gmtime(), *localtime();
***************
*** 399,404

  #else   /* !V7 */

  #include <fcntl.h>
   /*
    * this include file defines

--- 405,413 -----

  #else   /* !V7 */

+ #ifdef COHERENT
+ #include <sys/fcntl.h>
+ #else
  #include <fcntl.h>
  #endif
   /*
***************
*** 400,405
  #else   /* !V7 */

  #include <fcntl.h>
   /*
    * this include file defines
    *             #define O_BINARY 0x8000  (* no cr-lf translation *)

--- 409,415 -----
  #include <sys/fcntl.h>
  #else
  #include <fcntl.h>
+ #endif
   /*
    * this include file defines
    *             #define O_BINARY 0x8000  (* no cr-lf translation *)

--
Esa Ahola           esa@msdc.com      uunet!msdc!esa        CIS:70012,2753
Medical Systems Development Corporation (MsdC), Atlanta GA  (404) 589-3368


----- End of Forwarded Message

