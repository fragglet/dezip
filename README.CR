__________________________________________________________________________

  This is the Info-ZIP README.CR for zcrypt27.zip, last updated 28 Mar 97.
__________________________________________________________________________


The files described below contain the encryption code for Zip 2.2 and
UnZip 5.3 (and later).  They constitute only an add-on to the exportable
versions (generally named zip22.zip and unzip53.tar.Z) and cannot be 
used without the complete Zip or UnZip packages.

This encryption code is not copyrighted and is put in the public domain.
It was originally written in Europe and can be freely distributed from
any country except the U.S.A.  If this code is imported into the US, it
cannot be re-exported from the US to another country.  (This restriction
might seem curious but this is what US law requires.)  However, Phil Katz
has said that he got an export license for his algorithm, so this hassle
of separate distribution may cease one day.

LIKE ANYTHING ELSE THAT'S FREE, ZIP, UNZIP AND THEIR ASSOCIATED UTILITIES 
ARE PROVIDED AS IS AND COME WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED 
OR IMPLIED. IN NO EVENT WILL THE AUTHORS BE LIABLE FOR ANY DAMAGES
RESULTING FROM THE USE OF THIS SOFTWARE.

The encryption code is a direct transcription of the algorithm from
Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
file is distributed with the PKZIP program (even in the version without
encryption capabilities).  Note that the encryption will probably resist
attacks by amateurs if the password is well chosen and long enough (at 
least 8 characters) but it will probably not resist attacks by experts.
Paul Kocher has made available information concerning a known-plaintext
attack for the PKWARE encryption scheme; see http://www.cryptography.com/
for details.)  Short passwords consisting of lowercase letters only can be
recovered in a few hours on any workstation.  But for casual cryptography
designed to keep your mother from reading your mail, it's OK.

For more serious encryption, check into PGP (Pretty Good Privacy), a
public-key-based encryption system available from various Internet sites.
PGP has Zip and UnZip built into it.  The most recent version at the time
this was written was 2.6.2 (and 2.6.3i for non-US users).

Zip 2.2 and UnZip 5.3 are compatible with PKZIP 2.04g.  (Thanks to Phil
Katz for accepting our suggested minor changes to the zipfile format.)

IMPORTANT NOTE:

  Zip archives produced by Zip 2.0 (or later) must not be *updated* by
  Zip 1.1 or PKZIP 1.10 or PKZIP 1.93a, if they contain encrypted members
  or if they have been produced in a pipe or on a non-seekable device.
  The old versions of Zip or PKZIP would destroy the zip structure.  The
  old versions can list the contents of the zipfile but cannot extract
  it anyway (because of the new compression algorithm).  If you do not
  use encryption and use regular disk files, you need not worry about
  this problem.


Contents:

  file           what it is
  ----           ----------
  README.CR      this file
  Where          where Zip/UnZip and related utilities can be found
  crypt.c        code for encryption and decryption (for Zip and UnZip)
  crypt.h        code for encryption and decryption (for Zip and UnZip)
  file_id.diz    description file for some BBSes

All of the files are in Unix (LF only) format.  On MSDOS systems, you
can use the -a option of UnZip to convert the source files to CRLF
format.  This is only necessary if you wish to edit the files -- they
will compile as is with Microsoft C and Turbo/Borland C++ 1.0 or
later.  However, you will have to convert the files (using "unzip -a")
to the CRLF format to compile with the older Turbo C 1.0 or 2.0.  You
should be able to find Zip and UnZip in the same place you found this 
(see http://www.cdrom.com/pub/infozip/ or the file "Where" for details).
