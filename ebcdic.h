/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  ebcdic.h

  The CECP 1047 (Extended de-facto EBCDIC) <-> ISO 8859-1 conversion tables,
  from ftp://aix1.segi.ulg.ac.be/pub/docs/iso8859/iso8859.networking

  NOTES:
  <Paul_von_Behren@stortek.com> (OS/390 port 12/97)
   These table no longer represent the standard mappings (for example in the
   OS/390 iconv utility).  In order to follow current standards I remapped
     ebcdic x0a to ascii x15    and
     ebcdic x85 to ascii x25    (and vice-versa)
   Without these changes, newlines in auto-convert text files appeared
   as literal \045.
   I'm not sure what effect this remap would have on the MVS and CMS ports, so
   I ifdef'd these changes.  Hopefully these ifdef's can be removed when the
   MVS/CMS folks test the new mappings.

  Christian Spieler <spieler@ikp.tu-darmstadt.de>, 27-Apr-1998
   The problem mentioned by Paul von Behren was already observed previously
   on VM/CMS, during the preparation of the CMS&MVS port of UnZip 5.20 in
   1996. At that point, the ebcdic tables were not changed since they seemed
   to be an adopted standard (to my knowledge, these tables are still used
   as presented in mainfraime KERMIT). Instead, the "end-of-line" conversion
   feature of Zip's and UnZip's "text-translation" mode was used to force
   correct mappings between ASCII and EBCDIC newline markers.
   Before interchanging the ASCII mappings of the EBCDIC control characters
   "NL" 0x25 and "LF" 0x15 according to the OS/390 setting, we have to
   make sure that EBCDIC 0x15 is never used as line termination.

  ---------------------------------------------------------------------------*/

#ifndef __ebcdic_h /* prevent multiple inclusions */
#define __ebcdic_h

#ifndef ZCONST
#define ZCONST const
#endif

/*---------------------------------------------------------------------------

  The following conversion tables translate between IBM PC CP 850
  (OEM codepage) and the "Western Europe & America" Windows codepage 1252.
  The Windows codepage 1252 contains the ISO 8859-1 "Latin 1" codepage,
  with some additional printable characters in the range (0x80 - 0x9F),
  that is reserved to control codes in the ISO 8859-1 character table.

  The ISO <--> OEM conversion tables were constructed with the help
  of the WIN32 (Win16?) API's OemToAnsi() and AnsiToOem() conversion
  functions and have been checked against the CP850 and LATIN1 tables
  provided in the MS-Kermit 3.14 distribution.

  ---------------------------------------------------------------------------*/

#ifdef IZ_ISO2OEM_ARRAY
ZCONST uch Far iso2oem_850[] = {
    0x3F, 0x3F, 0x27, 0x9F, 0x22, 0x2E, 0xC5, 0xCE, /* 80 - 87 */
    0x5E, 0x25, 0x53, 0x3C, 0x4F, 0x3F, 0x3F, 0x3F, /* 88 - 8F */
    0x3F, 0x27, 0x27, 0x22, 0x22, 0x07, 0x2D, 0x2D, /* 90 - 97 */
    0x7E, 0x54, 0x73, 0x3E, 0x6F, 0x3F, 0x3F, 0x59, /* 98 - 9F */
    0xFF, 0xAD, 0xBD, 0x9C, 0xCF, 0xBE, 0xDD, 0xF5, /* A0 - A7 */
    0xF9, 0xB8, 0xA6, 0xAE, 0xAA, 0xF0, 0xA9, 0xEE, /* A8 - AF */
    0xF8, 0xF1, 0xFD, 0xFC, 0xEF, 0xE6, 0xF4, 0xFA, /* B0 - B7 */
    0xF7, 0xFB, 0xA7, 0xAF, 0xAC, 0xAB, 0xF3, 0xA8, /* B8 - BF */
    0xB7, 0xB5, 0xB6, 0xC7, 0x8E, 0x8F, 0x92, 0x80, /* C0 - C7 */
    0xD4, 0x90, 0xD2, 0xD3, 0xDE, 0xD6, 0xD7, 0xD8, /* C8 - CF */
    0xD1, 0xA5, 0xE3, 0xE0, 0xE2, 0xE5, 0x99, 0x9E, /* D0 - D7 */
    0x9D, 0xEB, 0xE9, 0xEA, 0x9A, 0xED, 0xE8, 0xE1, /* D8 - DF */
    0x85, 0xA0, 0x83, 0xC6, 0x84, 0x86, 0x91, 0x87, /* E0 - E7 */
    0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B, /* E8 - EF */
    0xD0, 0xA4, 0x95, 0xA2, 0x93, 0xE4, 0x94, 0xF6, /* F0 - F7 */
    0x9B, 0x97, 0xA3, 0x96, 0x81, 0xEC, 0xE7, 0x98  /* F8 - FF */
};
#endif /* IZ_ISO2OEM_ARRAY */

#ifdef IZ_OEM2ISO_ARRAY
ZCONST uch Far oem2iso_850[] = {
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, /* 80 - 87 */
    0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5, /* 88 - 8F */
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, /* 90 - 97 */
    0xFF, 0xD6, 0xDC, 0xF8, 0xA3, 0xD8, 0xD7, 0x83, /* 98 - 9F */
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, /* A0 - A7 */
    0xBF, 0xAE, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB, /* A8 - AF */
    0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xC1, 0xC2, 0xC0, /* B0 - B7 */
    0xA9, 0xA6, 0xA6, 0x2B, 0x2B, 0xA2, 0xA5, 0x2B, /* B8 - BF */
    0x2B, 0x2D, 0x2D, 0x2B, 0x2D, 0x2B, 0xE3, 0xC3, /* C0 - C7 */
    0x2B, 0x2B, 0x2D, 0x2D, 0xA6, 0x2D, 0x2B, 0xA4, /* C8 - CF */
    0xF0, 0xD0, 0xCA, 0xCB, 0xC8, 0x69, 0xCD, 0xCE, /* D0 - D7 */
    0xCF, 0x2B, 0x2B, 0xA6, 0x5F, 0xA6, 0xCC, 0xAF, /* D8 - DF */
    0xD3, 0xDF, 0xD4, 0xD2, 0xF5, 0xD5, 0xB5, 0xFE, /* E0 - E7 */
    0xDE, 0xDA, 0xDB, 0xD9, 0xFD, 0xDD, 0xAF, 0xB4, /* E8 - EF */
    0xAD, 0xB1, 0x3D, 0xBE, 0xB6, 0xA7, 0xF7, 0xB8, /* F0 - F7 */
    0xB0, 0xA8, 0xB7, 0xB9, 0xB3, 0xB2, 0xA6, 0xA0  /* F8 - FF */
};
#endif /* IZ_OEM2ISO_ARRAY */

/* The following pointers to the OEM<-->ISO translation tables are used
   by the translation code portions.  They may get initialized at program
   startup to point to the matching static translation tables, or to NULL
   to disable OEM-ISO translation.
   The compile-time initialization used here provides the backward compatible
   setting, as can be found in UnZip 5.52 and earlier.
   In case this mechanism will ever get used on a multithreading system that
   allows different codepage setups for concurrently running threads, these
   pointers should get moved into UnZip's thread-safe global data structure.
 */
#ifdef IZ_ISO2OEM_ARRAY
ZCONST uch Far *iso2oem = iso2oem_850; /* backward compatibility default */
#endif                                 /* IZ_ISO2OEM_ARRAY */
#ifdef IZ_OEM2ISO_ARRAY
ZCONST uch Far *oem2iso = oem2iso_850; /* backward compatibility default */
#endif                                 /* IZ_OEM2ISO_ARRAY */

#endif /* __ebcdic_h  */
