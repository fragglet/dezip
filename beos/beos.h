/* beos.h -- A few handy things for the BeOS port.     */
/* (c) 1996 Chris Herborth (chrish@qnx.com)            */
/* This is covered under the usual Info-ZIP copyright. */

/* We'll need this for read_16_swap()... */
#ifdef __MWERKS__
#  include <support/SupportDefs.h>
#endif
#ifdef __GNUC__
#  include <be/support/SupportDefs.h>
#endif

/* We do this to make printing a text version more sensible. */
typedef union {
    unsigned long l;
    char text[4];
} longtext;

/* Currently, the only things we need to store are the type/creator. */
typedef struct {
    short ID;
    short size;

    longtext type;
    longtext creator;
} extra_block;

typedef struct {
    unsigned short ID;
    unsigned short size;
} extra_header;

#define EF_BE_ID   0x4265
#define EF_BE_SIZE 0x0008
