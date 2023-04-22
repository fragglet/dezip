/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* crc32.h -- compute the CRC-32 of a data stream
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef __crc32_h
#define __crc32_h /* identifies this source module */

const ulg *get_crc_table(void);
ulg crc32(ulg crc, const uch *buf, extent len);

#ifndef CRC_32_TAB
#define CRC_32_TAB crc_32_tab
#endif

#ifdef CRC32
#undef CRC32
#endif
#define CRC32UPD(c, crctab) (crctab[((int) (c)) & 0xff] ^ ((c) >> 8))
#define CRC32(c, b, crctab) (crctab[((int) (c) ^ (b)) & 0xff] ^ ((c) >> 8))
#define REV_BE(w)           w

#endif /* !__crc32_h */
