From: mbeck@ai.mit.edu (Mark Becker)
Date: Thu, 5 Apr 90 16:17:27 EDT
To: kirsch@arsocomvax.socom.mil
Subject: BINGO!  unzip20g.tar.Z functional under MINIX

Pulled a copy of 20g from White Sands and tried it out.

Minix C still gets crc32.c wrong .. enclosed is a context diff for
what I did to get it running.  Nothing earthshaking.. but CRC's on
this thing won't come out right unless the unused bits are masked off.
How would you handle the code change?  As a context diff supplied
separately or a #ifdef MINIX ?

Also enclosed is the stripped-down makefile I used for this.  I did it
this way because Minix is a small model system (64K data/64K code) and
running two makes eats a lot of memory.  Feel free to modify as you'd
like; I can pull a new Makefile and test at your convience.

I wonder why zmemcpy.c doesn't used unsigned character pointers?  For
a test I changed from signed chars (the default under Minix) to
unsigned chars and things still ran okay.  Come to think of it, I'm
not sure it really matters...  :-)

HOORAY!

Regards,
Mark
mbeck@ai.mit.edu
-- cut here -- cut here -- cut here -- cut here -- cut here 
*** crc32.c.old	Fri Mar 23 05:51:24 1990
--- crc32.c	Thu Apr  5 16:05:48 1990
***************
*** 125,131 ****

      crcval = crc32val;
          while (len--) {
!         crcval = crc_32_tab[(byte)crcval ^ (byte)(*s++)]
              ^ (crcval >> 8);
          }
      crc32val = crcval;
--- 125,131 ----

      crcval = crc32val;
          while (len--) {
!         crcval = crc_32_tab[(int)((0x0FF & crcval) ^ (*s++))]
              ^ (crcval >> 8);
          }
      crc32val = crcval;
-- cut here -- cut here -- cut here -- cut here -- cut here 
