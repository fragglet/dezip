
(*
 * Copyright 1987, 1989 Samuel H. Smith;  All rights reserved
 *
 * This is a component of the ProDoor System.
 * Do not distribute modified versions without my permission.
 * Do not remove or alter this notice or any other copyright notice.
 * If you use this in your own program you must distribute source code.
 * Do not use any of this in a commercial product.
 *
 *)

(*
 * UnZip - A simple zipfile extract utility
 *
 *)

{$I+}    {I/O checking}
{$N-}    {Numeric coprocessor}
{$V-}    {Relaxes string typing}
{$B-}    {Boolean complete evaluation}
{$S-}    {Stack checking}
{$R-}    {Range checking}
{$D+}    {Global debug information}
{$L+}    {Local debug information}

{$M 5000,0,0} {minstack,minheap,maxheap}

program UnZip;

Uses
   Dos, Mdosio;

const
   version = 'UnZ:  Zipfile Extract v2.0 (PAS) of 09-09-89;  (C) 1989 S.H.Smith';



(*
 * Data declarations for the archive text-view functions.
 *
 *)

(* ----------------------------------------------------------- *)
(*
 * ZIPfile layout declarations
 *
 *)

type
   signature_type = longint;

const
   local_file_header_signature = $04034b50;

type
   local_file_header = record
      version_needed_to_extract:    word;
      general_purpose_bit_flag:     word;
      compression_method:           word;
      last_mod_file_time:           word;
      last_mod_file_date:           word;
      crc32:                        longint;
      compressed_size:              longint;
      uncompressed_size:            longint;
      filename_length:              word;
      extra_field_length:           word;
   end;

const
   central_file_header_signature = $02014b50;

type
   central_directory_file_header = record
      version_made_by:                 word;
      version_needed_to_extract:       word;
      general_purpose_bit_flag:        word;
      compression_method:              word;
      last_mod_file_time:              word;
      last_mod_file_date:              word;
      crc32:                           longint;
      compressed_size:                 longint;
      uncompressed_size:               longint;
      filename_length:                 word;
      extra_field_length:              word;
      file_comment_length:             word;
      disk_number_start:               word;
      internal_file_attributes:        word;
      external_file_attributes:        longint;
      relative_offset_local_header:    longint;
   end;

const
   end_central_dir_signature = $06054b50;

type
   end_central_dir_record = record
      number_this_disk:                         word;
      number_disk_with_start_central_directory: word;
      total_entries_central_dir_on_this_disk:   word;
      total_entries_central_dir:                word;
      size_central_directory:                   longint;
      offset_start_central_directory:           longint;
      zipfile_comment_length:                   word;
   end;



(* ----------------------------------------------------------- *)
(*
 * input file variables
 *
 *)

const
   uinbufsize = 512;    {input buffer size}
var
   zipeof:      boolean;
   csize:       longint;
   cusize:      longint;
   cmethod:     integer;
   cflags:      word;

   ctime:       word;
   cdate:       word;
   inbuf:       array[1..uinbufsize] of byte;
   inpos:       integer;
   incnt:       integer;
   pc:          byte;
   pcbits:      byte;
   pcbitv:      byte;
   zipfd:       dos_handle;
   zipfn:       dos_filename;



(* ----------------------------------------------------------- *)
(*
 * output stream variables
 *
 *)

var
   outbuf:      array[0..8192] of byte; {8192 or more for rle look-back}
   outpos:      longint;                {absolute position in outfile}
   outcnt:      integer;
   outfd:       dos_handle;
   filename:    string;
   extra:       string;



(* ----------------------------------------------------------- *)

type
   Sarray = array[0..255] of string[64];

var
   factor:     integer;
   followers:  Sarray;
   ExState:    integer;
   C:          integer;
   V:          integer;
   Len:        integer;

const
   hsize =     8192;

type
   hsize_array_integer = array[0..hsize] of integer;
   hsize_array_byte    = array[0..hsize] of byte;

var
   prefix_of:  hsize_array_integer;
   suffix_of:  hsize_array_byte;
   stack:      hsize_array_byte;
   stackp:     integer;




(*
 * Zipfile input/output handlers
 *
 *)


(* ------------------------------------------------------------- *)
procedure skip_csize;
begin
   dos_lseek(zipfd,csize,seek_cur);
   zipeof := true;
   csize := 0;
   incnt := 0;
end;


(* ------------------------------------------------------------- *)
procedure ReadByte(var x: byte);
begin
   if incnt = 0 then
   begin
      if csize = 0 then
      begin
         zipeof := true;
         exit;
      end;

      inpos := sizeof(inbuf);
      if inpos > csize then
         inpos := csize;
      incnt := dos_read(zipfd,inbuf,inpos);

      inpos := 1;
      dec(csize,incnt);
   end;

   x := inbuf[inpos];
   inc(inpos);
   dec(incnt);
end;


(*
 * Copyright 1987, 1989 Samuel H. Smith;  All rights reserved
 *
 * This is a component of the ProDoor System.
 * Do not distribute modified versions without my permission.
 * Do not remove or alter this notice or any other copyright notice.
 * If you use this in your own program you must distribute source code.
 * Do not use any of this in a commercial product.
 *
 *)

(******************************************************
 *
 * Procedure:  itoh
 *
 * Purpose:    converts an integer into a string of hex digits
 *
 * Example:    s := itoh(i);
 *
 *)

function itoh(i: longint): string;   {integer to hex conversion}
var
   h:   string;
   w:   word;

   procedure digit(ix: integer; ii: word);
   begin
      ii := ii and 15;
      if ii > 9 then 
         ii := ii + 7 + ord('a') - ord('A');
      h[ix] := chr(ii + ord('0'));
   end;

begin
   w := i and $FFFF;
   h[0] := chr(4);
   digit(1,w shr 12);
   digit(2,w shr 8);
   digit(3,w shr 4);
   digit(4,w);
   itoh := h;   
end;


(* ------------------------------------------------------------- *)
procedure ReadBits(bits: integer; var result: integer);
   {read the specified number of bits}
const
   bit:     integer = 0;
   bitv:    integer = 0;
   x:       integer = 0;
begin
   x := 0;
   bitv := 1;

   for bit := 0 to bits-1 do
   begin

      if pcbits > 0 then
      begin
         dec(pcbits);
         pcbitv := pcbitv shl 1;
      end
      else

      begin
         ReadByte(pc);
         pcbits := 7;
         pcbitv := 1;
      end;

      if (pc and pcbitv) <> 0 then
         x := x or bitv;

      bitv := bitv shl 1;
   end;

(* writeln(bits,'-',itoh(x)); *)
   result := x;
end;


(* ---------------------------------------------------------- *)
procedure get_string(ln: word; var s: string);
var
   n: word;
begin
   if ln > 255 then
      ln := 255;
   n := dos_read(zipfd,s[1],ln);
   s[0] := chr(ln);
end;


(* ------------------------------------------------------------- *)
procedure OutByte (c: integer);
   (* output each character from archive to screen *)
begin
   outbuf[outcnt {outpos mod sizeof(outbuf)} ] := c;
   inc(outpos);
   inc(outcnt);

   if outcnt = sizeof(outbuf) then
   begin
      dos_write(outfd,outbuf,outcnt);
      outcnt := 0;
      write('.');
   end;
end;


(*
 * expand 'reduced' members of a zipfile
 *
 *)

(*
 * The Reducing algorithm is actually a combination of two
 * distinct algorithms.  The first algorithm compresses repeated
 * byte sequences, and the second algorithm takes the compressed
 * stream from the first algorithm and applies a probabilistic
 * compression method.
 *
 *)

function reduce_L(x: byte): byte;
begin
   case factor of
      1: reduce_L := x and $7f;
      2: reduce_L := x and $3f;
      3: reduce_L := x and $1f;
      4: reduce_L := x and $0f;
   end;
end;

function reduce_F(x: byte): byte;
begin
   case factor of
      1: if x = 127 then reduce_F := 2 else reduce_F := 3;
      2: if x = 63  then reduce_F := 2 else reduce_F := 3;
      3: if x = 31  then reduce_F := 2 else reduce_F := 3;
      4: if x = 15  then reduce_F := 2 else reduce_F := 3;
   end;
end;

function reduce_D(x,y: byte): word;
begin
   case factor of
      1: reduce_D := ((x shr 7) and $01) * 256 + Y + 1;
      2: reduce_D := ((x shr 6) and $03) * 256 + Y + 1;
      3: reduce_D := ((x shr 5) and $07) * 256 + Y + 1;
      4: reduce_D := ((x shr 4) and $0f) * 256 + Y + 1;
   end;
end;

function reduce_B(x: byte): word;
   {number of bits needed to encode the specified number}
begin
   case x-1 of
      0..1:    reduce_B := 1;
      2..3:    reduce_B := 2;
      4..7:    reduce_B := 3;
      8..15:   reduce_B := 4;
     16..31:   reduce_B := 5;
     32..63:   reduce_B := 6;
     64..127:  reduce_B := 7;
   else        reduce_B := 8;
   end;
end;

procedure Expand(c: byte);
const
   DLE = 144;
var
   op:   longint;
   i:    integer;

begin

   case ExState of
        0:  if C <> DLE then
                outbyte(C)
            else
                ExState := 1;

        1:  if C <> 0 then
            begin
                V := C;
                Len := reduce_L(V);
                ExState := reduce_F(Len);
            end
            else
            begin
                outbyte(DLE);
                ExState := 0;
            end;

        2:  begin
               Len := Len + C;
               ExState := 3;
            end;

        3:  begin
               op := outpos-reduce_D(V,C);
               for i := 0 to Len+2 do
               begin
                  if op < 0 then
                     outbyte(0)
                  else
                     outbyte(outbuf[op mod sizeof(outbuf)]);
                  inc(op);
               end;

               ExState := 0;
            end;
   end;
end;


procedure LoadFollowers;
var
   x: integer;
   i: integer;
   b: integer;
begin
   for x := 255 downto 0 do
   begin
      ReadBits(6,b);
      followers[x][0] := chr(b);

      for i := 1 to length(followers[x]) do
      begin
         ReadBits(8,b);
         followers[x][i] := chr(b);
      end;
   end;
end;


(* ----------------------------------------------------------- *)
procedure unReduce;
   {expand probablisticly reduced data}

var
   lchar:   integer;
   lout:    integer;
   I:       integer;

begin
   factor := cmethod - 1;
   if (factor < 1) or (factor > 4) then
   begin
      skip_csize;
      exit;
   end;

   ExState := 0;
   LoadFollowers;
   lchar := 0;

   while (not zipeof) and (outpos < cusize) do
   begin

      if followers[lchar] = '' then
         ReadBits( 8,lout )
      else

      begin
         ReadBits(1,lout);
         if lout <> 0 then
            ReadBits( 8,lout )
         else
         begin
            ReadBits( reduce_B(length(followers[lchar])), I );
            lout := ord( followers[lchar][I+1] );
         end;
      end;

      if zipeof then
         exit;

      Expand( lout );
      lchar := lout;
   end;

end;



(*
 * expand 'shrunk' members of a zipfile
 *
 *)

(*
 * UnShrinking
 * -----------
 *
 * Shrinking is a Dynamic Ziv-Lempel-Welch compression algorithm
 * with partial clearing.  The initial code size is 9 bits, and
 * the maximum code size is 13 bits.  Shrinking differs from
 * conventional Dynamic Ziv-lempel-Welch implementations in several
 * respects:
 *
 * 1)  The code size is controlled by the compressor, and is not
 *     automatically increased when codes larger than the current
 *     code size are created (but not necessarily used).  When
 *     the decompressor encounters the code sequence 256
 *     (decimal) followed by 1, it should increase the code size
 *     read from the input stream to the next bit size.  No
 *     blocking of the codes is performed, so the next code at
 *     the increased size should be read from the input stream
 *     immediately after where the previous code at the smaller
 *     bit size was read.  Again, the decompressor should not
 *     increase the code size used until the sequence 256,1 is
 *     encountered.
 *
 * 2)  When the table becomes full, total clearing is not
 *     performed.  Rather, when the compresser emits the code
 *     sequence 256,2 (decimal), the decompressor should clear
 *     all leaf nodes from the Ziv-Lempel tree, and continue to
 *     use the current code size.  The nodes that are cleared
 *     from the Ziv-Lempel tree are then re-used, with the lowest
 *     code value re-used first, and the highest code value
 *     re-used last.  The compressor can emit the sequence 256,2
 *     at any time.
 *
 *)

procedure unShrink;

const
   max_bits =  13;
   init_bits = 9;
   first_ent = 257;
   clear =     256;
   
var
   cbits:      integer;
   maxcode:    integer;
   free_ent:   integer;
   maxcodemax: integer;
   offset:     integer;
   sizex:      integer;
   finchar:    integer;
   code:       integer;
   oldcode:    integer;
   incode:     integer;


(* ------------------------------------------------------------- *)
procedure partial_clear;
var
   pr:   integer;
   cd:   integer;

begin
   {mark all nodes as potentially unused}
   for cd := first_ent to free_ent-1 do
      word(prefix_of[cd]) := prefix_of[cd] or $8000;


   {unmark those that are used by other nodes}
   for cd := first_ent to free_ent-1 do
   begin
      pr := prefix_of[cd] and $7fff;    {reference to another node?}
      if pr >= first_ent then           {flag node as referenced}
         prefix_of[pr] := prefix_of[pr] and $7fff;
   end;


   {clear the ones that are still marked}
   for cd := first_ent to free_ent-1 do
      if (prefix_of[cd] and $8000) <> 0 then
         prefix_of[cd] := -1;


   {find first cleared node as next free_ent}
   free_ent := first_ent;
   while (free_ent < maxcodemax) and (prefix_of[free_ent] <> -1) do
      inc(free_ent);
end;


(* ------------------------------------------------------------- *)
begin
   (* decompress the file *)
   maxcodemax := 1 shl max_bits;
   cbits := init_bits;
   maxcode := (1 shl cbits)- 1;
   free_ent := first_ent;
   offset := 0;
   sizex := 0;

   fillchar(prefix_of,sizeof(prefix_of),$FF);
   for code := 255 downto 0 do
   begin
      prefix_of[code] := 0;
      suffix_of[code] := code;
   end;

   ReadBits(cbits,oldcode);
   if zipeof then
      exit;
   finchar := oldcode;

   OutByte(finchar);

   stackp := 0;

   while (not zipeof) do
   begin
      ReadBits(cbits,code);
      if zipeof then
         exit;

      while (code = clear) do
      begin
         ReadBits(cbits,code);

         case code of
            1: begin
                  inc(cbits);
                  if cbits = max_bits then
                     maxcode := maxcodemax
                  else
                     maxcode := (1 shl cbits) - 1;
               end;

            2: partial_clear;
         end;

         ReadBits(cbits,code);
         if zipeof then
            exit;
      end;


      {special case for KwKwK string}
      incode := code;
      if prefix_of[code] = -1 then
      begin
         stack[stackp] := finchar;
         inc(stackp);
         code := oldcode;
      end;


      {generate output characters in reverse order}
      while (code >= first_ent) do
      begin
         stack[stackp] := suffix_of[code];
         inc(stackp);
         code := prefix_of[code];
      end;

      finchar := suffix_of[code];
      stack[stackp] := finchar;
      inc(stackp);


      {and put them out in forward order}
      while (stackp > 0) do
      begin
         dec(stackp);
         OutByte(stack[stackp]);
      end;


      {generate new entry}
      code := free_ent;
      if code < maxcodemax then
      begin
         prefix_of[code] := oldcode;
         suffix_of[code] := finchar;
         while (free_ent < maxcodemax) and (prefix_of[free_ent] <> -1) do
            inc(free_ent);
      end;


      {remember previous code}
      oldcode := incode;
   end;

end;



(* ------------------------------------------------------------- *)
(*
 * Imploding
 * ---------
 *
 * The Imploding algorithm is actually a combination of two distinct
 * algorithms.  The first algorithm compresses repeated byte sequences
 * using a sliding dictionary.  The second algorithm is used to compress
 * the encoding of the sliding dictionary ouput, using multiple
 * Shannon-Fano trees.
 *
 *)

const
   maxSF = 256;

type
   sf_entry = record
                 Code:       word;
                 Value:      byte;
                 BitLength:  byte;
              end;

   sf_tree = record  {a shannon-fano tree}
      entry:         array[0..maxSF] of sf_entry;
      entries:       integer;
      MaxLength:     integer;
   end;

   sf_treep = ^sf_tree;

var
   lit_tree:               sf_tree;
   length_tree:            sf_tree;
   distance_tree:          sf_tree;
   lit_tree_present:       boolean;
   eightK_dictionary:      boolean;
   minimum_match_length:   integer;
   dict_bits:              integer;


procedure SortLengths(var tree: sf_tree);
   {Sort the Bit Lengths in ascending order, while retaining the order
    of the original lengths stored in the file}
var
   x:       integer;
   gap:     integer;
   t:       sf_entry;
   noswaps: boolean;
   a,b:     integer;

begin
   gap := tree.entries div 2;

   repeat
      repeat
         noswaps := true;
         for x := 0 to (tree.entries-1)-gap do
         begin
            a := tree.entry[x].BitLength;
            b := tree.entry[x+gap].BitLength;
            if (a > b) or
               ((a = b) and (tree.entry[x].Value > tree.entry[x+gap].Value)) then
            begin
               t := tree.entry[x];
               tree.entry[x] := tree.entry[x+gap];
               tree.entry[x+gap] := t;
               noswaps := false;
            end;
         end;
      until noswaps;

      gap := gap div 2;
   until gap < 1;
end;


(* ----------------------------------------------------------- *)
procedure ReadLengths(var tree: sf_tree);
var
   treeBytes:  integer;
   i:          integer;
   num,len:    integer;

begin
   {get number of bytes in compressed tree}
   ReadBits(8,treeBytes);
   inc(treeBytes);
   i := 0;

   begin
      tree.MaxLength := 0;

      {High 4 bits: Number of values at this bit length + 1. (1 - 16)
       Low  4 bits: Bit Length needed to represent value + 1. (1 - 16)}
      while treeBytes > 0 do
      begin
         ReadBits(4,len);  inc(len);
         ReadBits(4,num);  inc(num);

         while num > 0 do
         begin
            if len > tree.MaxLength then
               tree.MaxLength := len;
            tree.entry[i].BitLength := len;
            tree.entry[i].Value := i;
            inc(i);
            dec(num);
         end;

         dec(treeBytes);
      end;
   end;
end;


(* ----------------------------------------------------------- *)
procedure GenerateTrees(var tree: sf_tree);
   {Generate the Shannon-Fano trees}
var
   Code:          word;
   CodeIncrement: integer;
   LastBitLength: integer;
   i:             integer;

begin
   Code := 0;
   CodeIncrement := 0;
   LastBitLength := 0;

   i := tree.entries - 1;   {either 255 or 63}
   while i >= 0 do
   begin
      inc(Code,CodeIncrement);
      if tree.entry[i].BitLength <> LastBitLength then
      begin
         LastBitLength := tree.entry[i].BitLength;
         CodeIncrement := 1 shl (16 - LastBitLength);
      end;

      tree.entry[i].Code := Code;
      dec(i);
   end;
end;


(* ----------------------------------------------------------- *)
procedure ReverseBits(var tree: sf_tree);
   {Reverse the order of all the bits in the above ShannonCode[]
    vector, so that the most significant bit becomes the least
    significant bit. For example, the value 0x1234 (hex) would become
    0x2C48 (hex).}
var
   i:    integer;
   mask: word;
   revb: word;
   v:    word;
   o:    word;
   b:    integer;

begin
   for i := 0 to tree.entries-1 do
   begin
      {get original code}
      o := tree.entry[i].Code;

      {reverse each bit}
      mask := $0001;
      revb := $8000;
      v := 0;
      for b := 0 to 15 do
      begin
         {if bit set in mask, then substitute reversed bit}
         if (o and mask) <> 0 then
            v := v or revb;

         {advance to next bit}
         revb := revb shr 1;
         mask := mask shl 1;
      end;

      {store reversed bits}
      tree.entry[i].Code := v;
   end;
end;


(* ----------------------------------------------------------- *)
procedure LoadTree(var tree: sf_tree;
                   treesize: integer);
   {allocate and load a shannon-fano tree from the compressed file}
begin
   tree.entries := treesize;
   ReadLengths(tree);
   SortLengths(tree);
   GenerateTrees(tree);
   ReverseBits(tree);
end;


(* ----------------------------------------------------------- *)
procedure LoadTrees;
begin
   eightK_dictionary := (cflags and $02) <> 0; {bit 1}
   lit_tree_present := (cflags and $04) <> 0; {bit 2}

   if eightK_dictionary then
      dict_bits := 7
   else
      dict_bits := 6;

   if lit_tree_present then
   begin
      minimum_match_length := 3;
      LoadTree(lit_tree,256);
   end
   else
      minimum_match_length := 2;

   LoadTree(length_tree,64);
   LoadTree(distance_tree,64);
end;


(* ----------------------------------------------------------- *)
procedure ReadTree(var tree: sf_tree;
                   var dest: integer);
   {read next byte using a shannon-fano tree}
var
   bits: integer;
   cv:   word;
   b:    integer;
   cur:  integer;

begin
   bits := 0;
   cv := 0;
   cur := 0;
   dest := -1; {in case of error}

   while true do
   begin
      ReadBits(1,b);
      cv := cv or (b shl bits);
      inc(bits);

      (* this is a very poor way of decoding shannon-fano.  two quicker
      methods come to mind:
         a) arrange the tree as a huffman-style binary tree with
            a "leaf" indicator at each node,
      and
         b) take advantage of the fact that s-f codes are at most 8
            bits long and alias unused codes for all bits following
            the "leaf" bit.
      *)

      while tree.entry[cur].BitLength < bits do
      begin
         inc(cur);
         if cur >= tree.entries then
            exit;
      end;

      while tree.entry[cur].BitLength = bits do
      begin
         if tree.entry[cur].Code = cv then
         begin
            dest := tree.entry[cur].Value;
            exit;
         end;

         inc(cur);
         if cur >= tree.entries then
            exit;
      end;
   end;
end;


(* ----------------------------------------------------------- *)
procedure unImplode;
   {expand imploded data}

var
   lout:       integer;
   op:         longint;
   Length:     integer;
   Distance:   integer;
   i:          integer;

begin
   LoadTrees;

   while (not zipeof) and (outpos < cusize) do
   begin
      ReadBits(1,lout);

      if lout <> 0 then    {encoded data is literal data}
      begin
         if lit_tree_present then
            ReadTree(lit_tree,lout)   {use Literal Shannon-Fano tree}
         else
            ReadBits(8,lout);

         OutByte(lout);
      end
      else

      begin          {encoded data is sliding dictionary match}
         readBits(dict_bits,lout);
         Distance := lout;

         ReadTree(distance_tree,lout);
         Distance := Distance or (lout shl dict_bits);
         {using the Distance Shannon-Fano tree, read and decode the
            upper 6 bits of the Distance value}

         ReadTree(length_tree,Length);
         {using the Length Shannon-Fano tree, read and decode the Length value}

         inc(Length,Minimum_Match_Length);
         if Length = (63 + Minimum_Match_Length) then
         begin
            ReadBits(8,lout);
            inc(Length,lout);
         end;

         {move backwards Distance+1 bytes in the output stream, and copy
          Length characters from this position to the output stream.
          (if this position is before the start of the output stream,
          then assume that all the data before the start of the output
          stream is filled with zeros)}

         op := outpos - Distance - 1;
         for i := 1 to Length do
         begin
            if op < 0 then
               OutByte(0)
            else
               OutByte(outbuf[op mod sizeof(outbuf)]);
            inc(op);
         end;
      end;
   end;
end;



(*
 * This procedure displays the text contents of a specified archive
 * file.  The filename must be fully specified and verified.
 *
 *)


(* ---------------------------------------------------------- *)
procedure extract_member;
var
   b: byte;

begin
   pcbits := 0;
   incnt := 0;
   outpos := 0;
   outcnt := 0;
   zipeof := false;

   outfd := dos_create(filename);
   if outfd = dos_error then
   begin
      writeln('Can''t create output: ', filename);
      halt;
   end;

   case cmethod of
      0:    {stored}
            begin
               write(' Extract: ',filename,' ...');
               while (not zipeof) do
               begin
                  ReadByte(b);
                  OutByte(b);
               end;
            end;

      1:    begin
               write('UnShrink: ',filename,' ...');
               UnShrink;
            end;

      2..5: begin
               write('  Expand: ',filename,' ...');
               UnReduce;
            end;

      6:    begin
               write(' Explode: ',filename,' ...');
               unImplode;
            end;

      else  write('Unknown compression method.');
   end;

   if outcnt > 0 then
      dos_write(outfd,outbuf,outcnt);

   dos_file_times(outfd,time_set,ctime,cdate);
   dos_close(outfd);

   writeln('  done.');
end;


(* ---------------------------------------------------------- *)
procedure process_local_file_header;
var
   n:             word;
   rec:           local_file_header;

begin
   n := dos_read(zipfd,rec,sizeof(rec));
   get_string(rec.filename_length,filename);
   get_string(rec.extra_field_length,extra);
   csize := rec.compressed_size;
   cusize := rec.uncompressed_size;
   cmethod := rec.compression_method;
   cflags := rec.general_purpose_bit_flag;
   ctime := rec.last_mod_file_time;
   cdate := rec.last_mod_file_date;
   extract_member;
end;


(* ---------------------------------------------------------- *)
procedure process_central_file_header;
var
   n:             word;
   rec:           central_directory_file_header;
   filename:      string;
   extra:         string;
   comment:       string;

begin
   n := dos_read(zipfd,rec,sizeof(rec));
   get_string(rec.filename_length,filename);
   get_string(rec.extra_field_length,extra);
   get_string(rec.file_comment_length,comment);
end;


(* ---------------------------------------------------------- *)
procedure process_end_central_dir;
var
   n:             word;
   rec:           end_central_dir_record;
   comment:       string;

begin
   n := dos_read(zipfd,rec,sizeof(rec));
   get_string(rec.zipfile_comment_length,comment);
end;


(* ---------------------------------------------------------- *)
procedure process_headers;
var
   sig:  longint;

begin
   dos_lseek(zipfd,0,seek_start);

   while true do
   begin
      if dos_read(zipfd,sig,sizeof(sig)) <> sizeof(sig) then
         exit
      else

      if sig = local_file_header_signature then
         process_local_file_header
      else

      if sig = central_file_header_signature then
         process_central_file_header
      else

      if sig = end_central_dir_signature then
      begin
         process_end_central_dir;
         exit;
      end

      else
      begin
         writeln('Invalid Zipfile Header');
         exit;
      end;
   end;

end;


(* ---------------------------------------------------------- *)
procedure extract_zipfile;
begin
   zipfd := dos_open(zipfn,open_read);
   if zipfd = dos_error then
      exit;

   process_headers;

   dos_close(zipfd);
end;


(*
 * main program
 *
 *)

begin
   if paramcount <> 1 then
   begin
      writeln;
      writeln(version);
      writeln('Courtesy of:  S.H.Smith  and  The Tool Shop BBS,  (602) 279-2673.');
      writeln;
      writeln('You may copy and distribute this program freely, provided that:');
      writeln('    1)   No fee is charged for such copying and distribution, and');
      writeln('    2)   It is distributed ONLY in its original, unmodified state.');
      writeln('If you wish to distribute a modified version of this program, you MUST');
      writeln('include the source code.');
      writeln;
      writeln('If you modify this program, I would appreciate a copy of the  new source');
      writeln('code.   I am holding the copyright on the source code, so please don''t');
      writeln('delete my name from the program files or from the documentation.');
      writeln('IN NO EVENT WILL I BE LIABLE TO YOU FOR ANY DAMAGES, INCLUDING ANY LOST');
      writeln('PROFITS, LOST SAVINGS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES');
      writeln('ARISING OUT OF YOUR USE OR INABILITY TO USE THE PROGRAM, OR FOR ANY');
      writeln('CLAIM BY ANY OTHER PARTY.');
      writeln;
      writeln('Usage:  UnZip FILE[.zip]');
      halt;
   end;

   zipfn := paramstr(1);
   if pos('.',zipfn) = 0 then
      zipfn := zipfn + '.ZIP';

   extract_zipfile;
end.


