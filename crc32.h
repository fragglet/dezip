/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
*/
/* crc32.h -- compute the CRC-32 of a data stream
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef __crc32_h
#define __crc32_h /* identifies this source module */

const uint32_t *get_crc_table(void);
uint32_t crc32(uint32_t crc, const uint8_t *buf, size_t len);

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
