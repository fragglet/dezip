// -*- C++ -*-
//
// WARNING: This is C++, not C!  GNU C is not supported here!
//
// beos_init.cpp
//
// BeOS-specific C++ routines for use with Info-ZIP's UnZip 5.30 or later.
//
// This is necessary because we need to have an app_server connection to be
// able to ask the system to assign a MIME type to an un-typed file.  Strange
// but true (the update_mime_info() call needs to send/receive messages).
//
// If you're writing your own Zip application, you probably DO NOT want to
// include this file!

#include <app/Application.h>

#ifdef SFX
const static char *unzip_signature = "application/x-vnd.Info-ZIP.UnZipSFX";
#else
const static char *unzip_signature = "application/x-vnd.Info-ZIP.UnZip";
#endif

extern "C" int main_stub( int argc, char *argv[] );

int main( int argc, char **argv )
{
    BApplication app( unzip_signature );

    int retval = main_stub( argc, argv );

    app.PostMessage( B_QUIT_REQUESTED );
    app.Run();

    return retval;
}
