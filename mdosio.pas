
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
 * mdosio - Mini library for interface to DOS v3 file access functions
 *
 *)

{$i prodef.inc}

unit MDosIO;

interface

   uses Dos;

   type
      dos_filename = string[64];
      dos_handle   = word;

      long_integer = record
         lsw: word;
         msw: word;
      end;

      seek_modes = (seek_start {0},
                    seek_cur   {1},
                    seek_end   {2});

      open_modes = (open_read  {h40},     {deny_nothing, allow_read}
                    open_write {h41},     {deny_nothing, allow_write}
                    open_update{h42});    {deny_nothing, allow_read+write}

      dos_time_functions = (time_get,
                            time_set);

   const
      dos_error    = $FFFF; {file handle after an error}

   var
      dos_regs:     registers;
      dos_name:     dos_filename;


   procedure dos_call;

   function dos_open(name:      dos_filename;
                     mode:      open_modes):  dos_handle;

   function dos_create(name:    dos_filename): dos_handle;

   function dos_read( handle:   dos_handle;
                      var       buffer;
                      bytes:    word): word;

   procedure dos_write(handle:  dos_handle;
                       var      buffer;
                       bytes:   word);

   function dos_write_failed:   boolean;

   procedure dos_lseek(handle:  dos_handle;
                       offset:  longint;
                       method:  seek_modes);

   procedure dos_rseek(handle:  dos_handle;
                       recnum:  word;
                       recsiz:  word;
                       method:  seek_modes);

   function dos_tell: longint;

   procedure dos_find_eof(fd:   dos_handle);

   procedure dos_close(handle:  dos_handle);

   procedure dos_unlink(name:   dos_filename);

   procedure dos_file_times(fd:       dos_handle;
                            func:     dos_time_functions;
                            var time: word;
                            var date: word);

   function dos_jdate(time,date: word): longint;

   function dos_exists(name: dos_filename): boolean;


implementation

(* -------------------------------------------------------- *)
procedure dos_call;
var
   msg:  string;
begin
   msdos(dos_regs);

   if (dos_regs.flags and Fcarry) <> 0 then
   begin
      case dos_regs.ax of
         2:   msg := 'file not found';
         3:   msg := 'dir not found';
         4:   msg := 'too many open files';
         5:   msg := 'access denied';
         else str(dos_regs.ax,msg);
      end;
{$I-}
      writeln(' DOS error [',msg,'] on file [',dos_name,'] ');
{$i+}
      dos_regs.ax := dos_error;
   end;
end;


(* -------------------------------------------------------- *)
procedure prepare_dos_name(name: dos_filename);
begin
   while (name <> '') and (name[length(name)] <= ' ') do
      dec(name[0]);
   if name = '' then
      name := 'Nul';
   dos_name := name;
   dos_name[length(dos_name)+1] := #0;
   dos_regs.ds := seg(dos_name);
   dos_regs.dx := ofs(dos_name)+1;
end;


(* -------------------------------------------------------- *)
function dos_open(name:    dos_filename;
                  mode:    open_modes):  dos_handle;
var
   try: integer;
const
   retry_count = 3;

begin
   for try := 1 to retry_count do
   begin
      dos_regs.ax := $3d40 + ord(mode);
      prepare_dos_name(name);
      msdos(dos_regs);

      if (dos_regs.flags and Fcarry) = 0 then
      begin
         dos_open := dos_regs.ax;
         exit;
      end;
   end;

   dos_open := dos_error;
end;


(* -------------------------------------------------------- *)
function dos_create(name:    dos_filename): dos_handle;
begin
   dos_regs.ax := $3c00;
   prepare_dos_name(name);
   dos_regs.cx := 0;   {attrib}
   dos_call;
   dos_create := dos_regs.ax;
end;


(* -------------------------------------------------------- *)
function dos_read( handle:  dos_handle;
                   var      buffer;
                   bytes:   word): word;
begin
   dos_regs.ax := $3f00;
   dos_regs.bx := handle;
   dos_regs.cx := bytes;
   dos_regs.ds := seg(buffer);
   dos_regs.dx := ofs(buffer);
   dos_call;
   dos_read := dos_regs.ax;
end;


(* -------------------------------------------------------- *)
procedure dos_write(handle:  dos_handle;
                    var      buffer;
                    bytes:   word);
begin
   dos_regs.ax := $4000;
   dos_regs.bx := handle;
   dos_regs.cx := bytes;
   dos_regs.ds := seg(buffer);
   dos_regs.dx := ofs(buffer);
   dos_call;
   dos_regs.cx := bytes;
end;

function dos_write_failed: boolean;
begin
   dos_write_failed := dos_regs.ax <> dos_regs.cx;
end;


(* -------------------------------------------------------- *)
procedure dos_lseek(handle:  dos_handle;
                    offset:  longint;
                    method:  seek_modes);
var
   pos:  long_integer absolute offset;

begin
   dos_regs.ax := $4200 + ord(method);
   dos_regs.bx := handle;
   dos_regs.cx := pos.msw;
   dos_regs.dx := pos.lsw;
   dos_call;
end;


(* -------------------------------------------------------- *)
procedure dos_rseek(handle:  dos_handle;
                    recnum:  word;
                    recsiz:  word;
                    method:  seek_modes);
var
   offset: longint;
   pos:    long_integer absolute offset;

begin
   offset := longint(recnum) * longint(recsiz);
   dos_regs.ax := $4200 + ord(method);
   dos_regs.bx := handle;
   dos_regs.cx := pos.msw;
   dos_regs.dx := pos.lsw;
   dos_call;
end;


(* -------------------------------------------------------- *)
function dos_tell: longint;
  {call immediately after dos_lseek or dos_rseek}
var
   pos:  long_integer;
   li:   longint absolute pos;
begin
   pos.lsw := dos_regs.ax;
   pos.msw := dos_regs.dx;
   dos_tell := li;
end;


(* -------------------------------------------------------- *)
procedure dos_find_eof(fd: dos_handle);
   {find end of file, skip backward over ^Z eof markers}
var
   b: char;
   n: word;
   i: word;
   p: longint;
   temp: array[1..128] of char;

begin
   dos_lseek(fd,0,seek_end);
   p := dos_tell-1;
   if p < 0 then
      exit;

   p := p and $FFFF80;   {round to last 'sector'}
   {search forward for the eof marker}
   dos_lseek(fd,p,seek_start);
   n := dos_read(fd,temp,sizeof(temp));
   i := 1;

   while (i <= n) and (temp[i] <> ^Z) do
   begin
      inc(i);
      inc(p);
   end;

   {backup to overwrite the eof marker}
   dos_lseek(fd,p,seek_start);
end;


(* -------------------------------------------------------- *)
procedure dos_close(handle:  dos_handle);
begin
   dos_regs.ax := $3e00;
   dos_regs.bx := handle;
   msdos(dos_regs);  {dos_call;}
end;


(* -------------------------------------------------------- *)
procedure dos_unlink(name:    dos_filename);
   {delete a file, no error message if file doesn't exist}
begin
   dos_regs.ax := $4100;
   prepare_dos_name(name);
   msdos(dos_regs);
end;


(* -------------------------------------------------------- *)
procedure dos_file_times(fd:       dos_handle;
                         func:     dos_time_functions;
                         var time: word;
                         var date: word);
begin
   dos_regs.ax := $5700 + ord(func);
   dos_regs.bx := fd;
   dos_regs.cx := time;
   dos_regs.dx := date;
   dos_call;
   time := dos_regs.cx;
   date := dos_regs.dx;
end;


(* -------------------------------------------------------- *)
function dos_jdate(time,date: word): longint;
begin

(***
     write(' d=',date:5,' t=',time:5,' ');
     write('8',   (date shr 9) and 127:1); {year}
     write('/',   (date shr 5) and  15:2); {month}
     write('/',   (date      ) and  31:2); {day}
     write(' ',   (time shr 11) and 31:2); {hour}
     write(':',   (time shr  5) and 63:2); {minute}
     write(':',   (time shl  1) and 63:2); {second}
     writeln(' j=', (longint(date) shl 16) + longint(time));
 ***)

   dos_jdate := (longint(date) shl 16) + longint(time);
end;


(* -------------------------------------------------------- *)
function dos_exists(name: dos_filename): boolean;
var
   DirInfo:     SearchRec;

begin
   prepare_dos_name(name);
   FindFirst(dos_name,$21,DirInfo);
   if (DosError <> 0) then
      dos_exists := false
   else
      dos_exists := true;
end;

end.
