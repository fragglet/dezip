;===========================================================================
; Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.
;
; See the accompanying file LICENSE, version 2000-Apr-09 or later
; (the contents of which are also included in unzip.h) for terms of use.
; If, for some reason, all these files are missing, the Info-ZIP license
; also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
;===========================================================================
; flate.a created by Paul Kienitz, 20 June 94.  Last modified 21 Feb 96.
;
; 68000 assembly language version of inflate_codes(), for Amiga.  Prototype:
;
;   int inflate_codes(__GPRO__ struct huft *tl, struct huft *td,
;                     int bl, int bd);
;
; Where __GPRO__ expands to "Uz_Globs *G," if REENTRANT is defined,
; otherwise to nothing.  In the latter case G is a global variable.
;
; Define the symbol FUNZIP if this is for fUnZip.  It overrides REENTRANT.
;
; Define AZTEC to use the Aztec C macro version of getc() instead of the
; library getc() with FUNZIP.  AZTEC is ignored if FUNZIP is not defined.
;
; Define NO_CHECK_EOF to not use the fancy paranoid version of NEEDBITS --
; this is equivalent to removing the #define CHECK_EOF from inflate.c.
;
; Define INT16 if ints are short, otherwise it assumes ints are long.
;
; *DO NOT* define WSIZE -- this only works with the default value of 32K.
;
; 1999/09/23: for Human68k: Modified by Shimazaki Ryo.

X:              EQU     $7ffe

                IFDEF   INT16
MOVINT           MACRO  _1,_2
        move.w          _1,_2
                 ENDM
INTSIZE equ     2
                ELSE    ; !INT16
MOVINT:          MACRO  _1,_2
        move.l          _1,_2
                 ENDM
INTSIZE equ     4
                ENDC

                IFDEF   REENTRANT
                 IFNDEF FUNZIP
REENT_G equ     1
                 ENDC
                ENDC

; struct huft is defined as follows:
;
;   struct huft {
;     uch e;                /* number of extra bits or operation */
;     uch b;                /* number of bits in this code or subcode */
;     union {
;       ush n;              /* literal, length base, or distance base */
;       struct huft *t;     /* pointer to next level of table */
;     } v;
;   };                      /* sizeof(struct huft) == 6 */
;
; so here we define the offsets of the various members of this struct:

h_e             equ     0
h_b             equ     1
h_n             equ     2
h_t             equ     2
SIZEOF_HUFT     equ     6

; The following include file is generated from globals.h, and gives us equates
; that give the offsets in Uz_Globs of the fields we use, which are:
;       ulg bb
;       unsigned int bk, wp
;       (either array of or pointer to unsigned char) slide
; For fUnZip:
;       FILE *in
; For regular UnZip but not fUnZip:
;       int incnt, mem_mode
;       long csize
;       uch *inptr
; It also defines a value SIZEOF_slide, which tells us whether the appropriate
; slide field in G (either area.Slide or redirect_pointer) is a pointer or an
; array instance.  It is 4 in the former case and a large value in the latter.
; Lastly, this include will define CRYPT as 1 if appropriate.

                IFDEF   FUNZIP
        INCLUDE  human68k/g_offs_.mac
                ELSE
        INCLUDE  human68k/g_offs.mac
                ENDC

; G.bb is the global buffer that holds bits from the huffman code stream, which
; we cache in the register variable b.  G.bk is the number of valid bits in it,
; which we cache in k.  The macros NEEDBITS(n) and DUMPBITS(n) have side effects
; on b and k.

                IFDEF   REENT_G
G_SIZE  equ     4
G_PUSH           MACRO          ; this macro passes "__G__" to functions
        move.l          G,-(sp)
                 ENDM
                ELSE
        xref    _G              ; Uz_Globs
G_SIZE  equ     0
G_PUSH           MACRO
        ds.b            0       ; does nothing; the assembler dislikes MACRO ENDM
                 ENDM
                ENDC    ; REENT_G

        xref    _mask_bits      ; const ush near mask_bits[17];
                IFDEF   FUNZIP
                 IF     CRYPT
        xref    _encrypted      ; int -- boolean flag
        xref    _update_keys    ; int update_keys(__GPRO__ int)
        xref    _decrypt_byte   ; int decrypt_byte(__GPRO)
                 ENDC   ; CRYPT
                ELSE    ; !FUNZIP
        xref    _memflush       ; int memflush(__GPRO__ uch *, ulg)
        xref    _readbyte       ; int readbyte(__GPRO)
                ENDC    ; FUNZIP

        xref    _fgetc          ; int fgetc(FILE *)
        xref    _flush          ; if FUNZIP:  int flush(__GPRO__ ulg)
                                ; else:  int flush(__GPRO__ uch *, ulg *, int)

; Here are our register variables.

b       reg     d2              ; ulg
k       reg     d3              ; ush <= 32
e       reg     d4              ; ush < 256 for most use
w       reg     d5              ; unsigned int
n       reg     d6              ; ush
d       reg     d7              ; unsigned int

; We always maintain w and d as valid unsigned longs, though they may be short.

mask    reg     a3              ; ush *
t       reg     a4              ; struct huft *
*       reg     a5              ; stack frame
G       reg     a6              ; Uz_Globs *

; Couple other items we need:

savregs reg     d2-d7/a3/a4/a6

WSIZE   equ     $8000           ; 32k... be careful not to treat as negative!
EOF     equ     -1

                IFDEF   FUNZIP
; This does fgetc(in).  LIBC version is based on #define getc(fp) in stdio.h
; break d2

GETC              MACRO
        move.l          in-X(G),-(sp)
        jsr             _fgetc
        addq.l          #4,sp
                  ENDM
                ENDC    ; FUNZIP

; Input depends on the NEXTBYTE macro.  This exists in three different forms.
; The first two are for fUnZip, with and without decryption.  The last is for
; regular UnZip with or without decryption.  The resulting byte is returned
; in d0 as a longword, and d1, a0, and a1 are clobbered.

; FLUSH also has different forms for UnZip and fUnZip.  Arg must be a longword.
; The same scratch registers are trashed.

                IFDEF   FUNZIP

NEXTBYTE         MACRO
        move.l   d2,-(sp)
        GETC
                  IF    CRYPT
        tst.w           _encrypted+INTSIZE-2    ; test low word if long
        beq.s           @nbe
        MOVINT          d0,-(sp)                ; save thru next call
        G_PUSH
        jsr             _decrypt_byte
        eor.w           d0,G_SIZE+INTSIZE-2(sp) ; becomes arg to update_keys
        jsr             _update_keys
        addq            #INTSIZE+G_SIZE,sp
@nbe:
                   IFEQ INTSIZE-2
        ext.l           d0              ; assert -1 <= d0 <= 255
                   ENDC
                  ENDC  ; !CRYPT
        move.l   (sp)+,d2
                 ENDM

FLUSH            MACRO  _1
        move.l          d2,-(sp)
        move.l          _1,-(sp)
        G_PUSH
        jsr             _flush
        addq            #4+G_SIZE,sp
        move.l          (sp)+,d2
                 ENDM

                ELSE    ; !FUNZIP

NEXTBYTE         MACRO
;;        subq.l          #1,csize-X(G)
;;        bge.s           @nbg
;;        moveq           #EOF,d0
;;        bra.s           @nbe
@nbg:   subq.w          #1,incnt+INTSIZE-2-X(G)   ; treat as short
        bge.s           @nbs
                IFNE INTSIZE-2
        subq.w          #1,incnt-X(G)
        bge.s           @nbs
                ENDIF
        move.l          d2,-(sp)
        G_PUSH
        jsr             _readbyte
                IFNE    G_SIZE
        addq            #G_SIZE,sp
                ENDC
        move.l          (sp)+,d2
        bra.s           @nbe
@nbs:   moveq           #0,d0
        move.l          inptr-X(G),a0
        move.b          (a0)+,d0
        move.l          a0,inptr-X(G)
@nbe:
                 ENDM

FLUSH            MACRO  _1
        move.l          d2,-(sp)
        clr.l           -(sp)                   ; unshrink flag: always false
        move.l          _1,-(sp)                ; length
                  IF    SIZEOF_slide>4
        pea             slide-X(G)                ; buffer to flush
                  ELSE
        move.l          slide-X(G),-(sp)
                  ENDC
        G_PUSH
        tst.w           mem_mode+INTSIZE-2-X(G)   ; test lower word if long
        beq.s           @fm
        jsr             _memflush               ; ignores the unshrink flag
        bra.s           @fe
@fm:    jsr             _flush
@fe:    lea             8+INTSIZE+G_SIZE(sp),sp
        move.l          (sp)+,d2
                 ENDM

                ENDC    ; ?FUNZIP

; Here are the two bit-grabbing macros, defined in their NO_CHECK_EOF form:
;
;   #define NEEDBITS(n) {while(k<(n)){b|=((ulg)NEXTBYTE)<<k;k+=8;}}
;   #define DUMPBITS(n) {b>>=(n);k-=(n);}
;
; Without NO_CHECK_EOF, NEEDBITS reads like this:
;
;   {while(k<(n)){int c=NEXTBYTE;if(c==EOF)return 1;b|=((ulg)c)<<k;k+=8;}}
;
; NEEDBITS clobbers d0, d1, a0, and a1, none of which can be used as the arg
; to the macro specifying the number of bits.  The arg can be a shortword memory
; address, or d2-d7.  The result is copied into d1 as a word ready for masking.
; DUMPBITS has no side effects; the arg must be a d-register (or immediate in the
; range 1-8?) and only the lower byte is significant.

NEEDBITS        MACRO   _1
@nb:    cmp.w           _1,k            ; assert 0 < k <= 32 ... arg may be 0
        bhs.s           @ne
@loop:
        NEXTBYTE                        ; returns in d0.l
                 IFNDEF NO_CHECK_EOF
        cmp.w           #EOF,d0
        beq             error_return
                 ENDC   ; !NO_CHECK_EOF
@nok:   lsl.l           k,d0
        or.l            d0,b
        addq.w          #8,k
        cmp.w           _1,k            ;bra.s @nb
        bcs             @loop           ;
@ne:    move.w          b,d1
                ENDM

DUMPBITS        MACRO   _1
        lsr.l           _1,b            ; upper bits of _1 are ignored??
        sub.b           _1,k
                ENDM


; ******************************************************************************
; Here we go, finally:

        xdef    _inflate_codes

_inflate_codes:
        link            a5,#-4
        movem.l         savregs,-(sp)
; 8(a5) = tl, 12(a5) = td, 16(a5) = bl, 18|20(a5) = bd... add 4 for REENT_G
; -2(a5) = ml, -4(a5) = md.  Here we cache some globals and args:
                IFDEF   REENT_G
        move.l          8(a5),G
                ELSE
;;      move.l          _G,G            ; old global pointer version
        lea             _G,G            ; G is now a global instance
                IFDEF   X
        lea             (X,G),G
                ENDIF
                ENDC
        lea             _mask_bits,mask
        move.l          bb-X(G),b
        MOVINT          bk-X(G),k
                IFDEF   INT16
        moveq           #0,w            ; keep this usable as longword
                ENDC
        MOVINT          wp-X(G),w
        moveq           #0,e            ; keep this usable as longword too
        MOVINT          16+G_SIZE(a5),d0
        add.w           d0,d0
        move.w          (mask,d0.w),-2(a5)      ; ml = mask_bits[bl]
        MOVINT          16+INTSIZE+G_SIZE(a5),d0
        add.w           d0,d0
        move.w          (mask,d0.w),-4(a5)      ; md = mask_bits[bd]

main_loop:
        NEEDBITS        14+INTSIZE+G_SIZE(a5)   ; bl, lower word if long
        and.w           -2(a5),d1               ; ml
        mulu            #SIZEOF_HUFT,d1
        move.l          8+G_SIZE(a5),a0         ; tl
        lea             (a0,d1.l),t
        move.b          h_e(t),e
        cmp.w           #16,e
        bls.s           topdmp
intop:   cmp.w          #99,e
         beq            error_return    ; error in zipfile
         move.b         h_b(t),d0
         DUMPBITS       d0
         sub.w          #16,e
         NEEDBITS       e
         move.w         e,d0
         add.w          d0,d0
         and.w          (mask,d0.w),d1
         mulu           #SIZEOF_HUFT,d1
         move.l         h_t(t),a0
         lea            (a0,d1.l),t
         move.b         h_e(t),e
         cmp.w          #16,e
         bgt.s          intop
topdmp: move.b          h_b(t),d0
        DUMPBITS        d0

        cmp.w           #16,e           ; is this huffman code a literal?
        bne             lenchk          ; no
        move.w          h_n(t),d0       ; yes
                IF      SIZEOF_slide>4
        lea             slide-X(G),a0
                ELSE
        move.l          slide-X(G),a0
                ENDC
        move.b          d0,(a0,w.l)     ; stick in the decoded byte
        addq.w          #1,w
        bpl             main_loop       ; w < WSIZE(=$8000)
        FLUSH           w
        moveq           #0,w
        bra             main_loop       ; do some more

lenchk: cmp.w           #15,e           ; is it an end-of-block code?
        beq             finish          ; if yes, we're done
        NEEDBITS        e               ; no: we have a duplicate string
        move.w          e,d0
        add.w           d0,d0
        and.w           (mask,d0.w),d1
        move.w          h_n(t),n
        add.w           d1,n            ; length of block to copy
        DUMPBITS        e
        NEEDBITS        14+(2*INTSIZE)+G_SIZE(a5)       ; bd, lower word if long
        and.w           -4(a5),d1                       ; md
        mulu            #SIZEOF_HUFT,d1
        move.l          12+G_SIZE(a5),a0                ; td
        lea             (a0,d1.l),t
        move.b          h_e(t),e
        cmp.w           #16,e
        bls.s           middmp
inmid:   cmp.w          #99,e
         beq            return          ; error in zipfile
         move.b         h_b(t),d0
         DUMPBITS       d0
         sub.w          #16,e
         NEEDBITS       e
         move.w         e,d0
         add.w          d0,d0
         and.w          (mask,d0.w),d1
         mulu           #SIZEOF_HUFT,d1
         move.l         h_t(t),a0
         lea            (a0,d1.l),t
         move.b         h_e(t),e
         cmp.w          #16,e
         bgt.s          inmid
middmp: move.b          h_b(t),d0
        DUMPBITS        d0
        NEEDBITS        e
        move.w          e,d0
        add.w           d0,d0
        and.w           (mask,d0.w),d1
        move.l          w,d
        sub.w           h_n(t),d
        sub.w           d1,d            ; distance back to block to copy
        DUMPBITS        e

indup:   move.w         #WSIZE,e        ; violate the e < 256 rule
         and.w          #WSIZE-1,d
         cmp.w          d,w
         blo.s          ddgw
          sub.w         w,e
         bra.s          dadw
ddgw:     sub.w         d,e
dadw:    cmp.w          n,e
         bls.s          delen
          move.w        n,e
delen:
                IF      SIZEOF_slide>4
         lea            slide-X(G),a0
                ELSE
         move.l         slide-X(G),a0
                ENDC
         move.l         a0,a1
         add.l          w,a0            ; w and d are valid longwords
         add.l          d,a1
         move.w         e,d0
         subq           #1,d0           ; assert >= 0 if sign extended
dspin:    move.b        (a1)+,(a0)+     ; string is probably short, so
          dbra          d0,dspin        ; don't use any fancier copy method
         add.w          e,d
         add.w          e,w
         bpl            dnfl            ; w < WSIZE(=$8000)
         FLUSH          w
         moveq          #0,w
dnfl:    sub.w          e,n
         bne            indup           ; need to do more sub-blocks
        moveq           #0,e            ; restore zeroness in upper bytes
        bra             main_loop       ; do some more

finish: MOVINT          w,wp-X(G)         ; restore cached globals
        MOVINT          k,bk-X(G)
        move.l          b,bb-X(G)
        moveq           #0,d0           ; return "no error"
return: movem.l         (sp)+,savregs
        unlk            a5
        rts

error_return:
        moveq           #1,d0           ; PK_WARN?
        bra             return
