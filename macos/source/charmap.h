/*

Conversion table from MacOS Roman to ISO-8859 Latin 1

     Notes on Mac OS Roman:
     ----------------------

       Mac OS Roman character set is used for at least the following Mac OS
       localizations: U.S., British, Canadian French, French, Swiss
       French, German, Swiss German, Italian, Swiss Italian, Dutch,
       Swedish, Norwegian, Danish, Finnish, Spanish, Catalan,
       Portuguese, Brazilian, and the default International system.

       Not every char of the charset MacRoman has their equivalent
       in ISO-8859-1.
       To make the mapping in most cases possible, I choosed
       most similar chars or at least the MIDDLE DOT. Chars that
       do not have a direct match are marked with '***'

In all Mac OS encodings, character codes 0x00-0x7F are identical to ASCII

*/

#ifndef __macos_charmap_h
#define __macos_charmap_h


ZCONST uch MacRoman_to_ISO8859_1[128] = {
            /*  MacRoman        Unicode     # Unicode name  */
    0xC4    ,   /*  0x80        0x00C4      # LATIN CAPITAL LETTER A WITH DIAERESIS */
    0xC5    ,   /*  0x81        0x00C5      # LATIN CAPITAL LETTER A WITH RING ABOVE    */
    0xC7    ,   /*  0x82        0x00C7      # LATIN CAPITAL LETTER C WITH CEDILLA   */
    0xC9    ,   /*  0x83        0x00C9      # LATIN CAPITAL LETTER E WITH ACUTE */
    0xD1    ,   /*  0x84        0x00D1      # LATIN CAPITAL LETTER N WITH TILDE */
    0xD6    ,   /*  0x85        0x00D6      # LATIN CAPITAL LETTER O WITH DIAERESIS */
    0xDC    ,   /*  0x86        0x00DC      # LATIN CAPITAL LETTER U WITH DIAERESIS */
    0xE1    ,   /*  0x87        0x00E1      # LATIN SMALL LETTER A WITH ACUTE   */
    0xE0    ,   /*  0x88        0x00E0      # LATIN SMALL LETTER A WITH GRAVE   */
    0xE2    ,   /*  0x89        0x00E2      # LATIN SMALL LETTER A WITH CIRCUMFLEX  */
    0xE4    ,   /*  0x8A        0x00E4      # LATIN SMALL LETTER A WITH DIAERESIS   */
    0xE3    ,   /*  0x8B        0x00E3      # LATIN SMALL LETTER A WITH TILDE   */
    0xE5    ,   /*  0x8C        0x00E5      # LATIN SMALL LETTER A WITH RING ABOVE  */
    0xE7    ,   /*  0x8D        0x00E7      # LATIN SMALL LETTER C WITH CEDILLA */
    0xE9    ,   /*  0x8E        0x00E9      # LATIN SMALL LETTER E WITH ACUTE   */
    0xE8    ,   /*  0x8F        0x00E8      # LATIN SMALL LETTER E WITH GRAVE   */
    0xEA    ,   /*  0x90        0x00EA      # LATIN SMALL LETTER E WITH CIRCUMFLEX  */
    0xEB    ,   /*  0x91        0x00EB      # LATIN SMALL LETTER E WITH DIAERESIS   */
    0xED    ,   /*  0x92        0x00ED      # LATIN SMALL LETTER I WITH ACUTE   */
    0xEC    ,   /*  0x93        0x00EC      # LATIN SMALL LETTER I WITH GRAVE   */
    0xEE    ,   /*  0x94        0x00EE      # LATIN SMALL LETTER I WITH CIRCUMFLEX  */
    0xEF    ,   /*  0x95        0x00EF      # LATIN SMALL LETTER I WITH DIAERESIS   */
    0xF1    ,   /*  0x96        0x00F1      # LATIN SMALL LETTER N WITH TILDE   */
    0xF3    ,   /*  0x97        0x00F3      # LATIN SMALL LETTER O WITH ACUTE   */
    0xF2    ,   /*  0x98        0x00F2      # LATIN SMALL LETTER O WITH GRAVE   */
    0xF4    ,   /*  0x99        0x00F4      # LATIN SMALL LETTER O WITH CIRCUMFLEX  */
    0xF6    ,   /*  0x9A        0x00F6      # LATIN SMALL LETTER O WITH DIAERESIS   */
    0xF5    ,   /*  0x9B        0x00F5      # LATIN SMALL LETTER O WITH TILDE   */
    0xFA    ,   /*  0x9C        0x00FA      # LATIN SMALL LETTER U WITH ACUTE   */
    0xF9    ,   /*  0x9D        0x00F9      # LATIN SMALL LETTER U WITH GRAVE   */
    0xFB    ,   /*  0x9E        0x00FB      # LATIN SMALL LETTER U WITH CIRCUMFLEX  */
    0xFC    ,   /*  0x9F        0x00FC      # LATIN SMALL LETTER U WITH DIAERESIS   */
    0xB7    ,   /*  0xA0        0x2020  *** # DAGGER    */
    0xB0    ,   /*  0xA1        0x00B0      # DEGREE SIGN   */
    0xA2    ,   /*  0xA2        0x00A2      # CENT SIGN */
    0xA3    ,   /*  0xA3        0x00A3      # POUND SIGN    */
    0xA7    ,   /*  0xA4        0x00A7      # SECTION SIGN  */
    0xB7    ,   /*  0xA5        0x2022  *** # BULLET    */
    0xB6    ,   /*  0xA6        0x00B6      # PILCROW SIGN  */
    0xDF    ,   /*  0xA7        0x00DF      # LATIN SMALL LETTER SHARP S (German)   */
    0xAE    ,   /*  0xA8        0x00AE      # REGISTERED SIGN   */
    0xA9    ,   /*  0xA9        0x00A9      # COPYRIGHT SIGN    */
    0xB7    ,   /*  0xAA        0x2122  *** # TRADE MARK SIGN   */
    0xB4    ,   /*  0xAB        0x00B4      # ACUTE ACCENT  */
    0xA8    ,   /*  0xAC        0x00A8      # DIAERESIS */
    0xB7    ,   /*  0xAD        0x2260  *** # NOT EQUAL TO  */
    0xC6    ,   /*  0xAE        0x00C6      # LATIN CAPITAL LETTER AE   */
    0x4F    ,   /*  0xAF        0x00D8  *** # LATIN CAPITAL LETTER O WITH STROKE    */
    0xB7    ,   /*  0xB0        0x221E  *** # INFINITY  */
    0xB1    ,   /*  0xB1        0x00B1      # PLUS-MINUS SIGN   */
    0x3C    ,   /*  0xB2        0x2264  *** # LESS-THAN OR EQUAL TO */
    0x3E    ,   /*  0xB3        0x2265  *** # GREATER-THAN OR EQUAL TO  */
    0xA5    ,   /*  0xB4        0x00A5      # YEN SIGN  */
    0xB5    ,   /*  0xB5        0x00B5      # MICRO SIGN    */
    0xB7    ,   /*  0xB6        0x2202  *** # PARTIAL DIFFERENTIAL  */
    0xB7    ,   /*  0xB7        0x2211  *** # N-ARY SUMMATION   */
    0xB7    ,   /*  0xB8        0x220F  *** # N-ARY PRODUCT */
    0xB7    ,   /*  0xB9        0x03C0  *** # GREEK SMALL LETTER PI */
    0xB7    ,   /*  0xBA        0x222B  *** # INTEGRAL  */
    0xAA    ,   /*  0xBB        0x00AA      # FEMININE ORDINAL INDICATOR    */
    0xBA    ,   /*  0xBC        0x00BA      # MASCULINE ORDINAL INDICATOR   */
    0xB7    ,   /*  0xBD        0x03A9  *** # GREEK CAPITAL LETTER OMEGA    */
    0xE6    ,   /*  0xBE        0x00E6      # LATIN SMALL LETTER AE */
    0xF8    ,   /*  0xBF        0x00F8      # LATIN SMALL LETTER O WITH STROKE  */
    0xBF    ,   /*  0xC0        0x00BF      # INVERTED QUESTION MARK    */
    0xA1    ,   /*  0xC1        0x00A1      # INVERTED EXCLAMATION MARK */
    0xAC    ,   /*  0xC2        0x00AC      # NOT SIGN  */
    0x56    ,   /*  0xC3        0x221A  *** # SQUARE ROOT   */
    0x66    ,   /*  0xC4        0x0192  *** # LATIN SMALL LETTER F WITH HOOK    */
    0x3D    ,   /*  0xC5        0x2248  *** # ALMOST EQUAL TO   */
    0xB7    ,   /*  0xC6        0x2206  *** # INCREMENT */
    0xAB    ,   /*  0xC7        0x00AB      # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
    0xBB    ,   /*  0xC8        0x00BB      # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK    */
    0xB7    ,   /*  0xC9        0x2026  *** # HORIZONTAL ELLIPSIS   */
    0xA0    ,   /*  0xCA        0x00A0      # NO-BREAK SPACE    */
    0xC0    ,   /*  0xCB        0x00C0      # LATIN CAPITAL LETTER A WITH GRAVE */
    0xC3    ,   /*  0xCC        0x00C3      # LATIN CAPITAL LETTER A WITH TILDE */
    0xD5    ,   /*  0xCD        0x00D5      # LATIN CAPITAL LETTER O WITH TILDE */
    0x6F    ,   /*  0xCE        0x0152  *** # LATIN CAPITAL LIGATURE OE */
    0x6F    ,   /*  0xCF        0x0153  *** # LATIN SMALL LIGATURE OE   */
    0xAD    ,   /*  0xD0        0x2013  *** # EN DASH   */
    0xAD    ,   /*  0xD1        0x2014  *** # EM DASH   */
    0xA8    ,   /*  0xD2        0x201C  *** # LEFT DOUBLE QUOTATION MARK    */
    0xA8    ,   /*  0xD3        0x201D  *** # RIGHT DOUBLE QUOTATION MARK   */
    0xB4    ,   /*  0xD4        0x2018  *** # LEFT SINGLE QUOTATION MARK    */
    0xB4    ,   /*  0xD5        0x2019  *** # RIGHT SINGLE QUOTATION MARK   */
    0xF7    ,   /*  0xD6        0x00F7      # DIVISION SIGN */
    0xB7    ,   /*  0xD7        0x25CA  *** # LOZENGE   */
    0xFF    ,   /*  0xD8        0x00FF      # LATIN SMALL LETTER Y WITH DIAERESIS   */
    0xFF    ,   /*  0xD9        0x0178  *** # LATIN CAPITAL LETTER Y WITH DIAERESIS */
    0xB7    ,   /*  0xDA        0x2044  *** # FRACTION SLASH    */
    0xA4    ,   /*  0xDB        0x00A4      # CURRENCY SIGN */
    0x3C    ,   /*  0xDC        0x2039  *** # SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
    0x3E    ,   /*  0xDD        0x203A  *** # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK    */
    0xB7    ,   /*  0xDE        0xFB01  *** # LATIN SMALL LIGATURE FI   */
    0xB7    ,   /*  0xDF        0xFB02  *** # LATIN SMALL LIGATURE FL   */
    0xB7    ,   /*  0xE0        0x2021  *** # DOUBLE DAGGER */
    0xB7    ,   /*  0xE1        0x00B7      # MIDDLE DOT    */
    0xA8    ,   /*  0xE2        0x201A  *** # SINGLE LOW-9 QUOTATION MARK   */
    0xA8    ,   /*  0xE3        0x201E  *** # DOUBLE LOW-9 QUOTATION MARK   */
    0xB7    ,   /*  0xE4        0x2030  *** # PER MILLE SIGN    */
    0xC2    ,   /*  0xE5        0x00C2      # LATIN CAPITAL LETTER A WITH CIRCUMFLEX    */
    0xCA    ,   /*  0xE6        0x00CA      # LATIN CAPITAL LETTER E WITH CIRCUMFLEX    */
    0xC1    ,   /*  0xE7        0x00C1      # LATIN CAPITAL LETTER A WITH ACUTE */
    0xCB    ,   /*  0xE8        0x00CB      # LATIN CAPITAL LETTER E WITH DIAERESIS */
    0xC8    ,   /*  0xE9        0x00C8      # LATIN CAPITAL LETTER E WITH GRAVE */
    0xCD    ,   /*  0xEA        0x00CD      # LATIN CAPITAL LETTER I WITH ACUTE */
    0xCE    ,   /*  0xEB        0x00CE      # LATIN CAPITAL LETTER I WITH CIRCUMFLEX    */
    0xCF    ,   /*  0xEC        0x00CF      # LATIN CAPITAL LETTER I WITH DIAERESIS */
    0xCC    ,   /*  0xED        0x00CC      # LATIN CAPITAL LETTER I WITH GRAVE */
    0xD3    ,   /*  0xEE        0x00D3      # LATIN CAPITAL LETTER O WITH ACUTE */
    0xD4    ,   /*  0xEF        0x00D4      # LATIN CAPITAL LETTER O WITH CIRCUMFLEX    */
    0xB7    ,   /*  0xF0        0xF8FF  *** # Apple logo    */
    0xD2    ,   /*  0xF1        0x00D2      # LATIN CAPITAL LETTER O WITH GRAVE */
    0xDA    ,   /*  0xF2        0x00DA      # LATIN CAPITAL LETTER U WITH ACUTE */
    0xDB    ,   /*  0xF3        0x00DB      # LATIN CAPITAL LETTER U WITH CIRCUMFLEX    */
    0xD9    ,   /*  0xF4        0x00D9      # LATIN CAPITAL LETTER U WITH GRAVE */
    0x69    ,   /*  0xF5        0x0131  *** # LATIN SMALL LETTER DOTLESS I  */
    0xB7    ,   /*  0xF6        0x02C6  *** # MODIFIER LETTER CIRCUMFLEX ACCENT */
    0x7E    ,   /*  0xF7        0x02DC  *** # SMALL TILDE   */
    0xAF    ,   /*  0xF8        0x00AF      # MACRON    */
    0xB7    ,   /*  0xF9        0x02D8  *** # BREVE */
    0xB7    ,   /*  0xFA        0x02D9  *** # DOT ABOVE */
    0xB0    ,   /*  0xFB        0x02DA  *** # RING ABOVE    */
    0xB8    ,   /*  0xFC        0x00B8      # CEDILLA   */
    0xB4    ,   /*  0xFD        0x02DD  *** # DOUBLE ACUTE ACCENT   */
    0xB8    ,   /*  0xFE        0x02DB  *** # OGONEK    */
    0xB7        /*  0xFF        0x02C7  *** # CARON */
 };



ZCONST uch ISO8859_1_to_MacRoman[128] = {
    0xb7    ,   /*  0x80    */
    0xb7    ,   /*  0x81    */
    0xb7    ,   /*  0x82    */
    0xb7    ,   /*  0x83    */
    0xb7    ,   /*  0x84    */
    0xb7    ,   /*  0x85    */
    0xb7    ,   /*  0x86    */
    0xb7    ,   /*  0x87    */
    0xb7    ,   /*  0x88    */
    0xb7    ,   /*  0x89    */
    0xb7    ,   /*  0x8A    */
    0xb7    ,   /*  0x8B    */
    0xa0    ,   /*  0x8C    */
    0xa1    ,   /*  0x8D    */
    0xa2    ,   /*  0x8E    */
    0xa3    ,   /*  0x8F    */
    0xa4    ,   /*  0x90    */
    0xa5    ,   /*  0x91    */
    0xa7    ,   /*  0x92    */
    0xa8    ,   /*  0x93    */
    0xa8    ,   /*  0x94    */
    0xa8    ,   /*  0x95    */
    0xa8    ,   /*  0x96    */
    0xa8    ,   /*  0x97    */
    0xa9    ,   /*  0x98    */
    0xaa    ,   /*  0x99    */
    0xab    ,   /*  0x9A    */
    0xac    ,   /*  0x9B    */
    0xad    ,   /*  0x9C    */
    0xad    ,   /*  0x9D    */
    0xae    ,   /*  0x9E    */
    0xaf    ,   /*  0x9F    */
    0xb0    ,   /*  0xA0    */
    0xb0    ,   /*  0xA1    */
    0xb1    ,   /*  0xA2    */
    0xb4    ,   /*  0xA3    */
    0xb4    ,   /*  0xA4    */
    0xb4    ,   /*  0xA5    */
    0xb4    ,   /*  0xA6    */
    0xb5    ,   /*  0xA7    */
    0xb6    ,   /*  0xA8    */
    0xb7    ,   /*  0xA9    */
    0xb7    ,   /*  0xAA    */
    0xb7    ,   /*  0xAB    */
    0xb7    ,   /*  0xAC    */
    0xb7    ,   /*  0xAD    */
    0xb7    ,   /*  0xAE    */
    0xb7    ,   /*  0xAF    */
    0xb7    ,   /*  0xB0    */
    0xb7    ,   /*  0xB1    */
    0xb7    ,   /*  0xB2    */
    0xb7    ,   /*  0xB3    */
    0xb7    ,   /*  0xB4    */
    0xb7    ,   /*  0xB5    */
    0xb7    ,   /*  0xB6    */
    0xb7    ,   /*  0xB7    */
    0xb7    ,   /*  0xB8    */
    0xb7    ,   /*  0xB9    */
    0xb7    ,   /*  0xBA    */
    0xb7    ,   /*  0xBB    */
    0xb7    ,   /*  0xBC    */
    0xb7    ,   /*  0xBD    */
    0xb7    ,   /*  0xBE    */
    0xb7    ,   /*  0xBF    */
    0xb7    ,   /*  0xC0    */
    0xb7    ,   /*  0xC1    */
    0xb7    ,   /*  0xC2    */
    0xb8    ,   /*  0xC3    */
    0xb8    ,   /*  0xC4    */
    0xba    ,   /*  0xC5    */
    0xbb    ,   /*  0xC6    */
    0xbf    ,   /*  0xC7    */
    0xc0    ,   /*  0xC8    */
    0xc1    ,   /*  0xC9    */
    0xc2    ,   /*  0xCA    */
    0xc3    ,   /*  0xCB    */
    0xc4    ,   /*  0xCC    */
    0xc5    ,   /*  0xCD    */
    0xc6    ,   /*  0xCE    */
    0xc7    ,   /*  0xCF    */
    0xc8    ,   /*  0xD0    */
    0xc9    ,   /*  0xD1    */
    0xca    ,   /*  0xD2    */
    0xcb    ,   /*  0xD3    */
    0xcc    ,   /*  0xD4    */
    0xcd    ,   /*  0xD5    */
    0xce    ,   /*  0xD6    */
    0xcf    ,   /*  0xD7    */
    0xd1    ,   /*  0xD8    */
    0xd2    ,   /*  0xD9    */
    0xd3    ,   /*  0xDA    */
    0xd4    ,   /*  0xDB    */
    0xd5    ,   /*  0xDC    */
    0xd6    ,   /*  0xDD    */
    0xd9    ,   /*  0xDE    */
    0xda    ,   /*  0xDF    */
    0xdb    ,   /*  0xE0    */
    0xdc    ,   /*  0xE1    */
    0xdf    ,   /*  0xE2    */
    0xe0    ,   /*  0xE3    */
    0xe1    ,   /*  0xE4    */
    0xe2    ,   /*  0xE5    */
    0xe3    ,   /*  0xE6    */
    0xe4    ,   /*  0xE7    */
    0xe5    ,   /*  0xE8    */
    0xe6    ,   /*  0xE9    */
    0xe7    ,   /*  0xEA    */
    0xe8    ,   /*  0xEB    */
    0xe9    ,   /*  0xEC    */
    0xea    ,   /*  0xED    */
    0xeb    ,   /*  0xEE    */
    0xec    ,   /*  0xEF    */
    0xed    ,   /*  0xF0    */
    0xee    ,   /*  0xF1    */
    0xef    ,   /*  0xF2    */
    0xf1    ,   /*  0xF3    */
    0xf2    ,   /*  0xF4    */
    0xf3    ,   /*  0xF5    */
    0xf4    ,   /*  0xF6    */
    0xf5    ,   /*  0xF7    */
    0xf6    ,   /*  0xF8    */
    0xf7    ,   /*  0xF9    */
    0xf8    ,   /*  0xFA    */
    0xf9    ,   /*  0xFB    */
    0xfa    ,   /*  0xFC    */
    0xfb    ,   /*  0xFD    */
    0xfc    ,   /*  0xFE    */
    0xff        /*  0xFF    */
 };

#endif /* !__macos_charmap_h */
