/*
 mapname.c  for unzip v3.05

 Change DEC-20, VAX/VMS, DOS style filenames into normal Unix names.
 Almost ALL the code is from good old xxu, Author:  F. da Cruz, CUCCA

 We're assuming, of course, that someday somebody will be creating
 files on DECs and VAX/VMS systems, of course.

 Usage: set the "-m" switch on the unzip command line.

 Action: Renames argument files as follows:
   strips Unix and PKZIP DOS path name from front (up to rightmost '/')
   if present.
   strips DEC device:, node:: names from front (up to rightmost ':')
   if present.  (This also takes care of any DOS drive: artifacts.)
   strips DEC-20 <directory> or VMS [directory] name if present
   strips DEC-20 version number from end (everything after 2nd dot) if present
   strips VMS generation number from end (everything after ';') if present
   lowercases any uppercase letters
   honors DEC-20 CTRL-V quote for special characters
   discards unquoted unprintable characters

   Returns non-0 if filename zeroed out.

 Author:  David Kirschbaum, 25 Apr 90

 27 Apr 90: Reports indicate something's SERIOUSLY wrong.  When -m switch
 is enabled, it gobbles digits in file names!  Sigh ... Fixed.
 Sloppy testing.
 David Kirschbaum

 25 Apr 90:  Bill Davidsen did some tweaking.  v3.05

*/

#ifndef UNIX
#ifndef STRSIZ
#define STRSIZ 256
#endif
#endif

#include <stdio.h>
 /* this is your standard header for all C compiles */
#include <ctype.h>
#include <stdio.h>

#ifdef UNIX

/* On some systems the contents of sys/param.h duplicates the
   contents of sys/types.h, so you don't need (and can't use)
   sys/types.h. */

#include <sys/types.h>
#endif

#ifdef __STDC__

#include <string.h>
 /* this include defines strcpy, strcmp, etc. */
#endif


extern char filename[];		/* in unzip.c */
extern int mflag;

mapped_name()
{
    char name[13];			/* File name buffer (long enough
        				 * for a DOS filename) */
    char *pp, *cp, *xp;			/* Character pointers */
    char delim = '\0';			/* Directory Delimiter */
    int dc = 0;				/* Counters */
    int quote = 0;			/* Flags */
    int indir = 0;
    int done = 0;
    register int workch;		/* hold the character being tested */


    xp = filename;		/* Copy pointer for simplicity */
#ifdef MAP_DEBUG
    fprintf(stderr,"%s ",*xp);	/* Echo name of this file */
#endif
    pp = name;			/* Point to translation buffer */
    *name = '\0';		/* Initialize buffer */
    dc = 0;			/* Filename dot counter */
    done = 0;			/* Flag for early completion */

    for (cp = xp; workch = *cp++; ) { /* Loop thru chars... */

	if (quote) {		/* If this char quoted.. */
	    *pp++ = workch;	/*  include it literally. */
	    quote = 0;
        }
	else if (indir) {	/* If in directory name.. */
	    if (workch == delim)
		indir = 0;	/* look for end delimiter. */
        }
        else switch (workch) {
	    case '<':		/* Discard DEC-20 directory name */
		indir = 1;
		delim = '>';
		break;
	    case '[':		/* Discard VMS directory name */
		indir = 1;
		delim = ']';
		break;
	    case '/':		/* Discard Unix path name.. */
	    case '\\':		/*  or MS-DOS path name.. */
	    case ':':   	/*  or DEC dev: or node:: name */
		pp = name;
		break;
	    case '.':		/* DEC -20 generation number
				 * or MS-DOS type */
		if (++dc == 1)	/* Keep first dot */
		    *pp++ = workch;
		else		/* Discard everything starting */
		    done = 1;	/* with second dot. */
		break;
	    case ';':		/* VMS generation or DEC-20 attrib */
		done = 1;	/* Discard everything starting with */
		break;		/* semicolon */
	    case '\026':	/* Control-V quote for special chars */
		quote = 1;	/* Set flag for next time. */
		break;
	    default:		/* some other char */
		if(isdigit(workch))	/* v2.0k '0'..'9' */
		    *pp++ = workch;	/* v2.0k accept them, no tests */
		else{
		    if(mflag){		/* if -m switch.. */
		        if (isupper(workch)) /* Uppercase letter to lowercase */
        	            workch = tolower(workch);
		    }
		    if (workch == ' ')  /* change blanks to underscore */
		        *pp++ = '_';
		    else if (isprint(workch)) /* Other printable, just keep */
		        *pp++ = workch;
		}
	}  /* switch */
    }  /* for loop */
    *pp = '\0';				/* Done with name, terminate it */

    /* We COULD check for existing names right now,
     * create a "unique" name, etc.
     * However, since other unzips don't do that...
     * we won't bother.  Maybe another day, ne?
     * If this went bad, the name'll either be nulled out
     * (in which case we'll return non-0)
     * or following procedures won't be able to create the
     * extracted file, and other error msgs will result.
     */

    if(*name == '\0'){
	fprintf(stderr,"conversion of [%s] failed\n",filename);
	return(0);
    }
    strcpy(filename,name);	/* copy converted name into global */
    return(1);
}

