/* This header file was created by Joe Meadows, and is not copyrighted
   in any way. No guarantee is made as to the accuracy of the contents
   of this header file. This header file was last modified on Sep. 22th,
   1987. (Modified to include this statement) */
#define FAT$K_LENGTH 32
#define FAT$C_LENGTH 32
#define FAT$S_FATDEF 32

struct fatdef {
  union  {
    unsigned char fat$b_rtype;
    struct  {
      unsigned fat$v_rtype : 4;
      unsigned fat$v_fileorg : 4;
    } fat$r_rtype_bits;
  } fat$r_rtype_overlay;
# define FAT$S_RTYPE 4
# define FAT$V_RTYPE 0
#   define FAT$C_UNDEFINED 0
#   define FAT$C_FIXED 1
#   define FAT$C_VARIABLE 2
#   define FAT$C_VFC 3
#   define FAT$C_STREAM 4
#   define FAT$C_STREAMLF 5
#   define FAT$C_STREAMCR 6
# define FAT$S_FILEORG 4
# define FAT$V_FILEORG 4
#   define FAT$C_SEQUENTIAL 0
#   define FAT$C_RELATIVE 1
#   define FAT$C_INDEXED 2
#   define FAT$C_DIRECT 3
  union  {
    unsigned char fat$b_rattrib;
    struct  {
      unsigned fat$v_fortrancc : 1;
      unsigned fat$v_impliedcc : 1;
      unsigned fat$v_printcc : 1;
      unsigned fat$v_nospan : 1;
    } fat$r_rattrib_bits;
  } fat$r_rattrib_overlay;
#   define FAT$V_FORTRANCC 0
#   define FAT$M_FORTRANCC 1
#   define FAT$V_IMPLIEDCC 1
#   define FAT$M_IMPLIEDCC 2
#   define FAT$V_PRINTCC 2
#   define FAT$M_PRINTCC 4
#   define FAT$V_NOSPAN 3
#   define FAT$M_NOSPAN 8
  unsigned short int fat$w_rsize;
  union
  {
    unsigned long int fat$l_hiblk;
    struct
    {
      unsigned short int fat$w_hiblkh;
      unsigned short int fat$w_hiblkl;
    } fat$r_hiblk_fields;
  } fat$r_hiblk_overlay;
  union
  {
    unsigned long int fat$l_efblk;
    struct
    {
      unsigned short int fat$w_efblkh;
      unsigned short int fat$w_efblkl;
    } fat$r_efblk_fields;
  } fat$r_efblk_overlay;
  unsigned short int fat$w_ffbyte;
  unsigned char fat$b_bktsize;
  unsigned char fat$b_vfcsize;
  unsigned short int fat$w_maxrec;
  unsigned short int fat$w_defext;
  unsigned short int fat$w_gbc;
  char fat$fill[8];
  unsigned short int fat$w_versions;
};
