/*---------------------------------------------------------------------------
    FlexOS specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __flxcfg_h
#define __flxcfg_h

#define __16BIT__
#define MED_MEM
#define EXE_EXTENSION ".286"

#ifndef nearmalloc
#  define nearmalloc malloc
#  define nearfree free
#endif

#define CRTL_CP_IS_OEM

#define near
#define far

#endif /* !__flxcfg_h */
