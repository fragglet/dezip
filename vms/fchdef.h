/* This header file was created by Joe Meadows, and is not copyrighted
   in any way. No guarantee is made as to the accuracy of the contents
   of this header file. This header file was last modified on Sep. 22th,
   1987. (Modified to include this statement) */

#define FCH$V_BADACL 0x00B
#define FCH$M_BADACL (1 << FCH$V_ACL)
#define FCH$V_BADBLOCK 0x00E
#define FCH$M_BADBLOCK (1 << FCH$V_BADBLOCK)
#define FCH$V_CONTIG 0x007
#define FCH$M_CONTIG (1 << FCH$V_CONTIG)
#define FCH$V_CONTIGB 0x005
#define FCH$M_CONTIGB (1 << FCH$V_CONTIGB)
#define FCH$V_DIRECTORY 0x00D
#define FCH$M_DIRECTORY (1 << FCH$V_DIRECTORY)
#define FCH$V_ERASE 0x011
#define FCH$M_ERASE (1 << FCH$V_ERASE)
#define FCH$V_LOCKED 0x006
#define FCH$M_LOCKED (1 << FCH$V_LOCKED)
#define FCH$V_MARKDEL 0x00F
#define FCH$M_MARKDEL (1 << FCH$V_MARKDEL)
#define FCH$V_NOBACKUP 0x001
#define FCH$M_NOBACKUP (1 << FCH$V_NOBACKUP)
#define FCH$V_NOCHARGE 0x010
#define FCH$M_NOCHARGE (1 << FCH$V_NOCHARGE)
#define FCH$V_READCHECK 0x003
#define FCH$M_READCHECK (1 << FCH$V_READCHECK)
#define FCH$V_SPOOL 0x00C
#define FCH$M_SPOOL (1 << FCH$V_SPOOL)
#define FCH$V_WRITCHECK 0x004
#define FCH$M_WRITCHECK (1 << FCH$V_WRITCHECK)
#define FCH$V_WRITEBACK 0x002
#define FCH$M_WRITEBACK (1 << FCH$V_WRITEBACK)

struct fchdef  {
  unsigned : 1;
  unsigned fch$v_nobackup : 1 ;
  unsigned fch$v_writeback : 1;
  unsigned fch$v_readcheck : 1;
  unsigned fch$v_writcheck : 1;
  unsigned fch$v_contigb : 1;
  unsigned fch$v_locked : 1;
  unsigned fch$v_contig : 1;
  unsigned : 3;
  unsigned fch$v_badacl : 1;
  unsigned fch$v_spool : 1;
  unsigned fch$v_directory : 1;
  unsigned fch$v_badblock : 1;
  unsigned fch$v_markdel : 1;
  unsigned fch$v_nocharge : 1;
  unsigned fch$v_erase : 1;
};
