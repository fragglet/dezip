; crc_i386.asm, optimized CRC calculation function for Zip and UnZip, not
; copyrighted by Paul Kienitz and Christian Spieler.  Last revised 10 Nov 96.
;
; Revised 06-Oct-96, Scott Field (sfield@microsoft.com)
;   fixed to assemble with masm by not using .model directive which makes
;   assumptions about segment alignment.  Also,
;   avoid using loop, and j[e]cxz where possible.  Use mov + inc, rather
;   than lodsb, and other misc. changes resulting in the following performance
;   increases:
;
;      unrolled loops                NO_UNROLLED_LOOPS
;      *8    >8      <8              *8      >8      <8
;
;      +54%  +42%    +35%            +82%    +52%    +25%
;
;   first item in each table is input buffer length, even multiple of 8
;   second item in each table is input buffer length, > 8
;   third item in each table is input buffer length, < 8
;
;
; FLAT memory model assumed.
;
; The loop unroolling can be disabled by defining the macro NO_UNROLLED_LOOPS.
; This results in shorter code at the expense of reduced performance.
;
;==============================================================================
;
; Do NOT assemble this source if external crc32 routine from zlib gets used.
;
    IFNDEF USE_ZLIB
;
        .386p
        name    crc_i386
; don't make assumptions about segment align        .model flat

extrn   _get_crc_table:near    ; ulg near *get_crc_table(void);

;
    IFNDEF NO_STD_STACKFRAME
        ; Use a `standard' stack frame setup on routine entry and exit.
        ; Actually, this option is set as default, because it results
        ; in smaller code !!
STD_ENTRY       MACRO
                push    ebp
                mov     ebp,esp
        ENDM

        Arg1    EQU     08H[ebp]
        Arg2    EQU     0CH[ebp]
        Arg3    EQU     10H[ebp]

STD_LEAVE       MACRO
                pop     ebp
        ENDM

    ELSE  ; NO_STD_STACKFRAME

STD_ENTRY       MACRO
        ENDM

        Arg1    EQU     18H[esp]
        Arg2    EQU     1CH[esp]
        Arg3    EQU     20H[esp]

STD_LEAVE       MACRO
        ENDM

    ENDIF ; ?NO_STD_STACKFRAME

; This is the loop body of the CRC32 cruncher.
; registers modified:
;   ebx  : crc value "c"
;   esi  : pointer to next data byte "buf++"
; registers read:
;   edi  : pointer to base of crc_table array
; scratch registers:
;   eax  : requires upper three bytes of eax = 0, uses al
Do_CRC  MACRO
                mov     al, byte ptr [esi]   ; al <-- *buf
                inc     esi                  ; buf++
                xor     al,bl                ; (c ^ *buf++) & 0xFF
                shr     ebx,8                ; c = (c >> 8)
                xor     ebx,[edi+eax*4]      ;  ^ table[(c ^ *buf++) & 0xFF]
        ENDM

_TEXT   segment para

        public  _crc32
_crc32          proc    near  ; ulg crc32(ulg crc, ZCONST uch *buf, extent len)
                STD_ENTRY
                push    edi
                push    esi
                push    ebx
                push    edx
                push    ecx

                mov     esi,Arg2             ; 2nd arg: uch *buf
                sub     eax,eax              ;> if (!buf)
                test    esi,esi              ;>   return 0;
                jz      fine                 ;> else {

                call    _get_crc_table
                mov     edi,eax
                mov     ebx,Arg1             ; 1st arg: ulg crc
                sub     eax,eax              ; eax=0; make al usable as a dword
                mov     ecx,Arg3             ; 3rd arg: extent len
                not     ebx                  ;>   c = ~crc;

    IFNDEF  NO_UNROLLED_LOOPS
                mov     edx,ecx              ; save len in edx
                and     edx,000000007H       ; edx = len % 8
                shr     ecx,3                ; ecx = len / 8
                jz      No_Eights
; align loop head at start of 486 internal cache line !!
                align   16
Next_Eight:
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                dec     ecx
                jnz     Next_Eight
No_Eights:
                mov     ecx,edx

    ENDIF ; NO_UNROLLED_LOOPS
                jecxz   bail                 ;>   if (len)
; align loop head at start of 486 internal cache line !!
                align   16
loupe:                                       ;>     do {
                Do_CRC                       ;        c = CRC32(c, *buf++);
                dec     ecx                  ;>     } while (--len);
                jnz     loupe

bail:                                        ;> }
                mov     eax,ebx
                not     eax                  ;> return ~c;
fine:
                pop     ecx
                pop     edx
                pop     ebx
                pop     esi
                pop     edi
                STD_LEAVE
                ret
_crc32          endp

_TEXT   ends
;
    ENDIF ;!USE_ZLIB
;
end
