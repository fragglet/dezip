/*---------------------------------------------------------------------------

  UnZpLib.h

  This header-files is global to the project UnZpLib.

  ---------------------------------------------------------------------------*/

/*****************************************************************************/
/*  Macros, typedefs                                                         */
/*****************************************************************************/

#define MacStaticLib
#define MACUNZIP

#ifndef    TRUE
#define    TRUE 1
#endif  /* TRUE */
#ifndef    FALSE
#define    FALSE 0
#endif  /* FALSE */

/*
#define DEBUG_TIME
 */


#define USE_EF_UT_TIME

/* since we have no TZ environment variable on Macs
   this option must be disabled */
#undef IZ_CHECK_TZ


/*****************************************************************************/
/*  Includes standard headers                                                */
/*****************************************************************************/
#include <ansi_prefix.mac.h>
#include <TextUtils.h>
#include <Folders.h>
#include <Aliases.h>
#include <Resources.h>
#include <Gestalt.h>
#include <Traps.h>
#include <Processes.h>
#include <MacWindows.h>
#include <Fonts.h>
#include <ToolUtils.h>
#include <Dialogs.h>
#include <Devices.h>
#include <StandardFile.h>
