Date: Fri, 27 Apr 90 05:15:29 EDT
From: jones@mips1.uqam.ca (Jones*Peter)
Message-Id: <9004270915.AA23290@mips1.uqam.CA>
Apparently-To: info-zip@simtel20.army.mil

         
Here is my compile and test run on a mips machine. It works fine, despite a
certain number of lint message. The procdeure was invoked with;
make -f Makefile.mips > (the file your are reading now) .

        test  -z mips -f Makefile.mips  \    <--- make default
        && make ERROR -f Makefile.mips \
        || make mips -f Makefile.mips
        uname -A
mips1 mips1 4_0 UMIPS mips m120-5 ATT_V3_0    <---- make mips
        ls -l Makefile.mips Makefile
-rw-rw-r--   1 jones    prof        5003 Apr 26 08:52 Makefile
-rw-------   1 jones    prof        5677 Apr 27 05:04 Makefile.mips
2c2,3                   <------ changes to Makefile (only file changed)
< 
---
> # P. Jones UQAM April 27th, 1990
> # Added support for Mips
62a64
> ZMEMC = zmemset.c zmemcpy.c
66c68,69
< SYSTEMS       =xenix386 ultrix sun3 sun4 encore stellar convex vaxbsd next vaxsysV
---
> SYSTEMS       =xenix386 ultrix sun3 sun4 encore stellar convex vaxbsd next vaxsysV mips
> SYSTEM = mips
68c71
< # The below will try to use your shell variable "SYSTEM"
---
> # The code below will try to use your shell variable "SYSTEM"
74,77c77,80
<       if test -z "$(SYSTEM)";\       <---- Causes a syntax error on our system
<       then make ERROR;\
<       else make $(SYSTEM);\
<       fi
---
>       - test  -z $(SYSTEM) -f Makefile.mips  \
>       && make ERROR -f Makefile.mips \
>       || make $(SYSTEM) -f Makefile.mips
>       exit
85a89
>       rm $(OBJS)
158a163,177
> # Mips code is here
> mips: #MIPS System V
> #Print system identification
>       uname -A
> # Show file creation date
>       ls -l Makefile.mips Makefile
> #show differences in Makefile and this file
>       @- diff Makefile Makefile.mips
> #Actual coplilation run
>       $(MAKE) unzip CFLAGS="$(CFLAGS) -DNOTINT16 -DZMEM"      \
>       OBJS="$(OBJS) $(ZMEMS)"  
> #Test on a handy file
>       ./unzip -t levels.zip
> #Run lint to finish up
>       lint $(CFLAGS) -DNOTINT16 -DZMEM $(SRCS) $(ZMEMC)
160c179,182
< 
---
> # added the useful function below....
> clean:
>       - rm $(OBJS) 
>       - rm unzip
