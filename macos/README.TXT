Macintosh Port of Info-Zip's Zip
by Dirk Haase, d_haase@sitec.net
homepage:   www.sitec.net/maczip
13.November 1998
================================


Some notes about this port:
===========================

MacZip requires at least System 7 and a Macintosh with a minimum of a
Motorola 68020 or PowerPC 601 processor. Other configurations may work
but it is not tested at all.

The application is distributed as a fat binary with both regular 68K
and native PowerPC versions included.

Move the executable(s) somewhere--for example, drag it (or them) to your
Applications folder.  For easy access, make an alias in the Launcher Control
Panel or directly on your desktop.
The GUI is very simple. It was not my intention to make a full-blown GUI,
however I think it is comfortable enough to use it as regular tool.

This port supports also Apple-event.So you can install it in your
WWW-Browser as a helper-app.

For more Info about the contents of this package, take a look into
the "macos/Contents" (or :macos:Contents) file. Some notes on how to
rebuild the Macintosh applications can be found in INSTALL.



Usage:
------

To extract a Zip archive, just drop it on MacZip.
MacZip will start and extract all files in a separate folder.
Or
Start MacZip; choose the app you want in the menu "Current App".
Select "File->Command Line" execute your selected app with the
Unix-style command-line
Or select
"File->Start Zip/Unzip" to execute your app via dialog-box.
For more information about switches choose File->Get help on
Zip/Unzip. A short introduction is also available online:
http://www.sitec.net/maczip/How-To-Do/

It's possible to stop the run of Zip/Unzip with the well
known shortcut [Command] + [.].

There are some Mac-specific switches available.
Zip Module:
       -df    [MacOS   only]  Include  only  data-fork  of  files
              zipped into the archive.  Good for exporting  files
              to  foreign operating-systems.  Resource-forks will
              be ignored at all.

       -jj    [MacOS  only] record Fullpath (+ Volname). The com-
              plete  path  including  volume  will  be stored. By
              default the relative path will be stored.

       -S     Include system and hidden  files.  This  option  is
              effective  on  some  systems only; it is ignored on
              Unix.  On MacOS, this option includes finder invis-
              ible files, which are ignored otherwise.

Unzip Module:
       -E     [MacOS only] display contents of MacOS extra  field
              during restore operation.

       -i     [MacOS only] ignore filenames stored in MacOS extra
              fields.  Instead,  the  most  compatible   filename
              stored in the generic part of the entry's header is
              used.

       -J     [MacOS only] ignore MacOS extra fields.  All Macin-
              tosh  specific  info  is  skipped.  Data-fork   and
              resource-fork are restored as separate files.




Performance and Limitations:
----------------------------

I was curious about performance items. So I took one small test on my 200 PPC
under MacOS 8.1. To compare `Drop Stuff' with `MacZip' I archived my Netscape
folder:
143 Files in 21 folders about 15.6 Megabytes.

Compress:
Drop Stuff (4.0) needs 70 sec to compress and additional 20 sec to create
a HQX file.
Compression ratio: 2.01 times smaller
MacZip needs 45 sec to compress, so it's about 1.5 - 2 times faster.
Compression ratio: 2.06 times smaller

Expand:
Stuffit Expander (4.02) needs 35 sec to create a '.sit' file and
additional 35 sec to expand all files.
MacZip need 25 sec to expand all files so it's 1.7 - 3.5 times faster.


Limitations/Problems:

    - Aliases are not supported. I tried, but I got broken aliases
      This port will silently ignore all aliases.
      It's on my to-do list for future releases.

    - Zip needs much memory to compress many files:
      You may need to increase the 'Preferred Size' in 'Get Info'.
      Values of 12 Megabytes or more are possible

    - Unzip needs about 500 Kbyte of memory to unzip no matter
      how many files were compressed and expanded.

    - and finally one big macintosh-related problem:
      This port has one weak point: It's based on pathnames.
      As you may be already know: Pathnames are not unique on a Mac !
      The main reason is that an attempt to implement support exact saving of
      the MacOS specific internal file-structures would require a throughout
      rewrite of major parts of shared code, probably sacrifying compatibility
      with other systems.
      I have no solution at the moment. The port will just warn you if you try
      zip from / to a volume which has a duplicate name.
      MacZip has problems to find the archive or the files.
      My (Big) recommendation: Name all your volumes with a unique name and
      MacZip will run without any problem.


Known Bugs:

    - Display of comments stored in the zip-file is in a mess.

    - crypted files in a zip archive are sometimes corrupt:
      I get an error message: invalid compressed data to inflate.
      Appearance of this error is purly be change:
      I did a small test: Unzipping an archive containing 3589 files
      56 files fails to unzip, so about 1.5%

I strongly recommend to test your archive (eg. unzip -t archive).




Zip-Programs / Macintosh Extra-Data:
-----------------------------------------
A brief overview:
Currently, as far as I know, there are 4 Zip-Programs available
for the Macintosh platform.
These programs build (of cause) different variants of Zip-Files:

    - Info-Zip's first Port of Zip. Ported by Johnny Lee
      This Port is rather outdated and no longer supported (since 1992).
      68K only. Only minimal Mac-info is stored (Creator/Type,
      Finderattributes).
      Until now, Only Unzip 5.32 survived.

    - ZipIt by Tom Brown. This is Shareware and still supported I think.
      ZipIt has a nice GUI, but I found it can't handle large Zipfiles
      quite well. ZipIt compresses Macintosh files using the MacBinary
      format. So, transfering files to other platforms is not so easy.
      Only minimal Mac-info is stored (Creator/Type, Finderattributes).
      Macfilenames are changed to a most compatible filename.

    - PKZIP/mac v2.03/210d. This is Shareware.
      This Zip implementation for the Mac can be found on ASI's website
      (http://www.asizip.com/products/products.htm).  The name of this
      program is misleading, it is NOT a product from PKWARE.  ASI last
      release version is v2.03, and they also offer a newer beta version
      PKZIP/mac 210d. But even the Beta version is rather outdated (1995).
      Only minimal Mac-info is stored (Creator/Type, Finderattributes).
      The Zipfile format looks like incompatible to other platforms.
      (More details about the compatibility issue can be found in
      proginfo/3rdparty.bug!)
      Macfilenames are restored without any change.

and finally:
    - Info-Zip's latest Port of Zip. MacZip 1.0. Ported by me :-)
      It is supported (of cause) and up to date. Fullset of macintosh
      info is stored: Creator/Type, Finderattributes, Findercomments,
      MacOS 8.0 Foldersettings, Icon/Folder-Positions ...
      Macfilenames are restored without any change.

Compatibility of my port; Extraction:
   - Archives from Info-Zip's first port (by Johnny Lee) are still compatible.
   - Archives from ZipIt can be extracted, but some files will still
     remain in a MacBinaryformat, no Finderinfo will be restored.
   - PKZiP/mac archives-files are extracted without resource-forks
     and without any Finderinfo. I have no information about that zip-format.

Compatibility of my port; Compression:
   - My port supports only the new Info-Zip format
     (introduced with this port). Therefore archives created by MacZip 1.0
     (1998) must be extracted with this version or later releases of Info-ZIP's
     UnZip to restore the complete set of Macintosh attributes.

Note: This port is complete unrelated to the shareware ZipIt. Even more,
handling of special Macintosh attributes is incompatible with ZipIt.
This port (MacZip) may be used to extract archives created by ZipIt,
but the Resource fork and the extra field of ZipIt will be ignored.
Make sure that you get the result as you expected.



Macintosh Files; File Forks:
----------------------------

All Macintosh files comprise two forks, known as the data fork and the
resource fork.  Unlike the bytes stored in the resource fork, the bytes in
the data fork do not have to exhibit any particular internal structure.
The application is responsible for interpreting the bytes in the data fork
in whatever manner is appropriate. The bytes in the resource fork usually
have a defined internal structure and contain data object like menus,
dialog-boxes, icons and pictures.
Although all Macintosh files contain both a data fork and a resource fork,
one or both of these forks may be empty.

MacZip stores data-forks and resource-forks separatly. The Zipfile-Format
does not allow to store two archive entries using exactly the same name.
My solution is to modify the Pathname of the resource-fork. All resource-fork
names are prepended with a leading special directory named "XtraStuf.mac".
So, when extracting on a Mac, you should never see this directory
"XtraStuf.mac" on your *disk*.

On all foreign systems that support directories in filenames (e.g: OS/2, Unix,
DOS/Windows, VMS) you will get a directory "XtraStuf.mac" when extracting
MacZip archives.
You can delete the complete directory "XtraStuf.mac" since Mac-resources
do not make much sense outside the MacOS world.



Textencoding; Charsets of the Filenames:
----------------------------------------

The following information is only important if you plan to transfer
archives across different platforms/language systems:

A typical Zip-archive does not support different charsets. All filenames
stored in the public area (= accessible by foreign systems other
than MacOS) must be coded in the charset ISO-8859-1 (CP1252 in the Microsoft
Windows world) or CP850 (DOSLatin1). The latter should only be used by
Zip programs that mark the archive entries as "created under DOS".
Apart from Macs, the commonly used platforms either support ISO-8859-1
directly, or are compatible with it.
To achieve maximum compatibility, MacZip convert filenames from the
Mac OS Roman character set to ISO-8859-1 and vice versa.
But not every char of the charset MacRoman has their equivalent
in ISO-8859-1. To make the mapping in most cases possible, I chose
most similar chars or at least the MIDDLE DOT.

Mac OS Roman character set is used for at least the following Mac OS
localizations:
U.S., British, Canadian French, French, Swiss French,
German, Swiss German, Italian, Swiss Italian, Dutch,
Swedish, Norwegian, Danish, Finnish, Spanish, Catalan,
Portuguese, Brazilian, and the default International system.

In all Mac OS encodings, character codes 0x00-0x7F are identical to
ASCII, except that
  - in Mac OS Japanese, yen sign replaces reverse solidus
  - in Mac OS Arabic, Farsi, and Hebrew, some of the punctuation in this
    range is treated as having strong left-right directionality,
    although the corresponding Unicode characters have neutral
    directionality
So, for best compatibility, confine filenames to the standard
7-bit ASCII character set.

If you generate a filename list of your archive (unzip -l), you will
see the converted filenames. Your can also extract the archive with
the switch '-i' (= ignore mac filenames), and test your result.

This MacZip-port uses its own filename stored in the archive.
At the moment, the filename will be not converted. However,
I'm planning to add support for unicode.

Currently, the following Mac OS encodings are NOT supported:
Japanese, ChineseTrad, Korean, Arabic, Hebrew, Greek, Cyrillic,
Devanagari, Gurmukhi, Gujarati, Oriya, Bengali, Tamil, Telugu
Kannada, Malayalam, Sinhalese, Burmese, Khmer, Thai, Laotian,
Georgian, Armenian, ChineseSimp, Tibetan, Mongolian, Ethiopic,
Vietnamese, ExtArabic and finally:
Symbol - this is the encoding for the font named "Symbol".
Dingbats - this is the encoding for the font named "Zapf Dingbats".
If you extract an archive coded with one of these charsets
you will propably get filenames with funny characters.

These problems apply only to filenames and NOT to the file
content.
Of cause: The content of the files will NEVER be converted !!



File-/Creator Type:
-------------

This port uses the creator type 'IZip' and it is registered at Apple
(since 08. March 1998). File types can not be registered any more.
This port uses '.zip' for normal Zip archive files and '.zp2' for
Zip archives that contain encrypted entries.
The creator 'IZip' type should be used for all future versions.



Hints for proper restoration of filetime stamps:
------------------------------------------------

UnZip requires the host computer to have proper timezone information in
order to handle certain tasks correctly (see unzip.doc).  To set the
timezone on the Macintosh, go to the Map Control Panel and enter the
correct number of hours (and, in a few locales, minutes) offset from
Universal Time/Greenwich Mean Time.  For example, the US Pacific timezone
is -8 hours from UTC/GMT during standard (winter) time and -7 hours from
UTC/GMT during Daylight Savings Time.  The US Eastern timezone is -5 hours
during the winter and -4 hours during the summer.

Discussion of Daylight Savings Time
-----------------------------------
The setting in the Date & Time control panel for Daylight Savings time
is a universal setting. That is, it assumes everybody in the world is
observing Daylight Savings time when its checkbox is selected.

If other areas of the world are not observing Daylight Savings time when
the checkbox is selected in the Date & Time control panel, then the Map
control panel will be off by an hour for all areas that are not recognizing
Daylight Savings time.

Conversely, if you set the Map control panel to an area that does not observe
Daylight Savings time and deselect/uncheck the checkbox for Daylight Savings
time in the Date & Time control panel, then time in all areas celebrating
Daylight Savings time will be off by an hour in the Map control panel.

Example:
     In the case of Hawaiians, sometimes they are three hours
     behind Pacific Standard Time (PST) and sometimes two hours
     behind Pacific Daylight Time (PDT). The Map control panel
     can only calculate differences between time zones relative
     to Greenwich Mean Time (GMT). Hawaii will always show up as
     three hours past the Pacific time zone and five hours past
     the Central time zone.

     When Hawaiians are not observing Daylight Savings time, but
     the rest of the country is, there is no combination of
     settings in Map and Date & Time control panels which will
     enable you to display Hawaiian local time correctly AND
     concurrently display the correct time in other places that
     do observe Daylight Savings time.

     The knowledge about which countries observe Daylight Savings
     time and which do not is not built into the Map control
     panel, so it does not allow for such a complex calculation.

     This same situation also occurs in other parts of the world
     besides Hawaii. Phoenix, Arizona is an example of an area of
     the U.S. which also does not observe Daylight Savings time.

Conclusion:
MacZip only know the GMT and DST offsets for the current time, not
for the time in question.
Do not use the setting in the Date and Time control panel for Daylight
Savings time. Instead: Change your current timezone accordingly.


Projects & Packages:
--------------------

A Note to version numbers: Version of MacZip is currently 1.00 and
is based on the zip-code version 2.3 and unzip-code version 5.4.
See About-Box for current version and compiler build-date.

Because of the amount of sources I splitted this port into
serveral projects. See http://www.sitec.net/maczip for updates.

- core source parts:
    unzipxxx.zip
    zipxxx.zip
      These archives contains the main-parts of the port. You can build
      libraries and a standalone-App with Metrowerks standard console SIOUX.
      They contain only sources, no executables.
      These archives are exact copies of the standard Info-ZIP source
      distributions; they were only repacked under MacOS using MacZip,
      with one minor addition: For those files that are stored in BinHex'ed
      format in the Info-ZIP reference source archives, unpacked version
      that are ready for use have been added.

- additional source part:
    MacZipxxxxsrc.zip: contains all the GUI-staff and the project-files to
      build the main-app.  Only sources of the GUI, no zip- or unzip-code.
      To build MacZip successfully you will need to also download the zip-
      and unzip-packages.

- executables:
    MacZipxxxxnc.hqx: contains only executables and some docs,
                      version without en-/decryption support
    MacZipxxxxc.hqx:  contains only executables and some docs, Crypt version

- encryption sources:
    zcryptxx.zip: To build crypt-versions of MacZip.
    download from ftp://ftp.icce.rug.nl/infozip/ (and subdirectories)



Credits:
--------

Macstuff.c and recurse.c: All the functions are from More Files.
More Files fixes many of the broken or underfunctional
parts of the file system. Thanks to Jim Luther.
(see morefiles.doc)

MatWild.c: Originally C++ code written by JJS (jstrout@ucsd.edu).







Dirk Haase
==========
