/* This header file was created by Joe Meadows, and is not copyrighted
   in any way. No guarantee is made as to the accuracy of the contents
   of this header file. This header file was last modified on Sep. 22th,
   1987. (Modified to include this statement) */

#define FJN$M_ONLY_RU 1
#define FJN$M_RUJNL 2
#define FJN$M_BIJNL 4
#define FJN$M_AIJNL 8
#define FJN$M_ATJNL 16
#define FJN$M_NEVER_RU 32
#define FJN$M_JOURNAL_FILE 64
#define FJN$S_FJNDEF 1
struct fjndef  {
  unsigned fjn$v_only_ru : 1;
  unsigned fjn$v_rujnl : 1;
  unsigned fjn$v_bijnl : 1;
  unsigned fjn$v_aijnl : 1;
  unsigned fjn$v_atjnl : 1;
  unsigned fjn$v_never_ru : 1;
  unsigned fjn$v_journal_file:1;
} ;
