/* 
   [VMS_attr.c, based on] FILE.C, a utility to modify file characteristics.
 
   Written by Joe Meadows Jr, at the Fred Hutchinson Cancer Research Center
   BITNET: JOE@FHCRCVAX
   PHONE: (206) 467-4970
   [...and stripped to the bone for unzip usage by Greg Roelofs.]
 
   There are no restrictions on this code, you may sell it, include it with
   any commercial package, or feed it to a whale.. However, I would appreciate
   it if you kept this comment in the source code so that anyone receiving
   this code knows who to contact in case of problems. Note that I do not
   demand this condition..
 */
 
#include <descrip.h>
#include <rms.h>
#include <stdio.h>
#include <iodef.h>
#include <atrdef.h> /* this gets created with the c3.0 compiler */
#include <fibdef.h> /* this gets created with the c3.0 compiler */

#include "fatdef.h"
#include "fchdef.h"
#include "fjndef.h"
 
#define RTYPE     fat$r_rtype_overlay.fat$r_rtype_bits
#define RATTRIB   fat$r_rattrib_overlay.fat$r_rattrib_bits

extern char  zipfn[];	/* GRR unzip:  name of file to be modified */
int  old_rtype;		/* save old zipfile attribute so can restore */

/* save...
char *cvt_time();
char *format_uic();
 */
 

 
int change_zipfile_attributes( restore )   /* GRR unzip:  used to be main() */
  int restore;
{
  static struct FAB Fab;
  static struct NAM Nam;
  static struct fibdef Fib; /* short fib */
 
  static struct dsc$descriptor FibDesc =
    {sizeof(Fib),DSC$K_DTYPE_Z,DSC$K_CLASS_S,&Fib};
  static struct dsc$descriptor_s DevDesc =
    {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,&Nam.nam$t_dvi[1]};
  static struct fatdef Fat;
  static union {
    struct fchdef fch;
    long int dummy;
  } uchar;
  static struct fjndef jnl;
  static long int Cdate[2],Rdate[2],Edate[2],Bdate[2];
  static short int revisions;
  static unsigned long uic;
  static union {
    unsigned short int value;
    struct {
      unsigned system : 4;
      unsigned owner : 4;
      unsigned group : 4;
      unsigned world : 4;
    } bits;
  } prot;
 
  static struct atrdef Atr[] = {
    {sizeof(Fat),ATR$C_RECATTR,&Fat},        /* record attributes */
    {sizeof(uchar),ATR$C_UCHAR,&uchar},      /* File characteristics */
    {sizeof(Cdate),ATR$C_CREDATE,&Cdate[0]}, /* Creation date */
    {sizeof(Rdate),ATR$C_REVDATE,&Rdate[0]}, /* Revision date */
    {sizeof(Edate),ATR$C_EXPDATE,&Edate[0]}, /* Expiration date */
    {sizeof(Bdate),ATR$C_BAKDATE,&Bdate[0]}, /* Backup date */
    {sizeof(revisions),ATR$C_ASCDATES,&revisions}, /* number of revisions */
    {sizeof(prot),ATR$C_FPRO,&prot},         /* file protection  */
    {sizeof(uic),ATR$C_UIC,&uic},            /* file owner */
    {sizeof(jnl),ATR$C_JOURNAL,&jnl},        /* journal flags */
    {0,0,0}
  } ;
 
  static char EName[NAM$C_MAXRSS];
  static char RName[NAM$C_MAXRSS];
  static struct dsc$descriptor_s FileName =
    {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
  static struct dsc$descriptor_s string = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
  static short int DevChan;
  static short int iosb[4];
 
  static long int i,status;
/* static char *retval; */
 
 
/*---------------------------------------------------------------------------
    Initialize attribute blocks, parse filename, resolve any wildcards, and
    get the file info.
  ---------------------------------------------------------------------------*/
 
    /* initialize RMS structures, we need a NAM to retrieve the FID */
    Fab = cc$rms_fab;
    Fab.fab$l_fna = zipfn; /* name of file */
    Fab.fab$b_fns = strlen(zipfn);
    Fab.fab$l_nam = &Nam; /* FAB has an associated NAM */
    Nam = cc$rms_nam;
    Nam.nam$l_esa = &EName; /* expanded filename */
    Nam.nam$b_ess = sizeof(EName);
    Nam.nam$l_rsa = &RName; /* resultant filename */
    Nam.nam$b_rss = sizeof(RName);
 
    /* do $PARSE and $SEARCH here */
    status = sys$parse(&Fab);
    if (!(status & 1)) return(status);
 
    /* search for the first file.. If none signal error */
    status = sys$search(&Fab);
    if (!(status & 1)) return(status);

    while (status & 1) {
        /* initialize Device name length, note that this points into the NAM
           to get the device name filled in by the $PARSE, $SEARCH services */
        DevDesc.dsc$w_length = Nam.nam$t_dvi[0];
 
        status = sys$assign(&DevDesc,&DevChan,0,0);
        if (!(status & 1)) return(status);
 
        FileName.dsc$a_pointer = Nam.nam$l_name;
        FileName.dsc$w_length = Nam.nam$b_name+Nam.nam$b_type+Nam.nam$b_ver;
 
        /* Initialize the FIB */
        for (i=0;i<3;i++)
            Fib.fib$r_fid_overlay.fib$w_fid[i]=Nam.nam$w_fid[i];
        for (i=0;i<3;i++)
            Fib.fib$r_did_overlay.fib$w_did[i]=Nam.nam$w_did[i];
 
        /* Use the IO$_ACCESS function to return info about the file */
        /* Note, used this way, the file is not opened, and the expiration */
        /* and revision dates are not modified */
        status = sys$qiow(0,DevChan,IO$_ACCESS,&iosb,0,0,
                          &FibDesc,&FileName,0,0,&Atr,0);
        if (!(status & 1)) return(status);
        status = iosb[0];
        if (!(status & 1)) return(status);

/* save...
        if ((Cdate[0]==0) && (Cdate[1]==0))
            printf("  /NO%s -\n",t_credate);
        else
            printf("  /%s=\"%s\" -\n",t_credate,cvt_time(Cdate));

        if ((Rdate[0]==0) && (Rdate[1]==0))
            printf("  /NO%s",t_revdate);
        else
            printf("  /%s=\"%s\"",t_revdate,cvt_time(Rdate));
 */

        if (restore)				/* GRR unzip */
            Fat.RTYPE.fat$v_rtype = old_rtype;
        else {
            old_rtype = Fat.RTYPE.fat$v_rtype;
            Fat.RTYPE.fat$v_rtype = FAT$C_STREAMLF;   /* Unix I/O loves it */
        }

/* save...
        status = cli_present(t_credate);
        if (status & 1) {
            status = cli_get_value(t_credate,&retval);
            bintim(retval,Cdate);
        }
        else
            if ((status == CLI$_NEGATED) || (status == CLI$_LOCNEG)) {
                Cdate[0]=0;
                Cdate[1]=0;
            };

        status = cli_present(t_revdate);
        if (status & 1) {
            status = cli_get_value(t_revdate,&retval);
            bintim(retval,Rdate);
        }
        else
            if ((status == CLI$_NEGATED) || (status == CLI$_LOCNEG)) {
                Rdate[0]=0;
                Rdate[1]=0;
            };
 */
 
 
        /* note, part of the FIB was cleared by earlier QIOW, so reset it */
        Fib.fib$r_acctl_overlay.fib$l_acctl = FIB$M_NORECORD;
        for (i=0;i<3;i++)
            Fib.fib$r_fid_overlay.fib$w_fid[i]=Nam.nam$w_fid[i];
        for (i=0;i<3;i++)
            Fib.fib$r_did_overlay.fib$w_did[i]=Nam.nam$w_did[i];
 
        /* Use the IO$_MODIFY function to change info about the file */
        /* Note, used this way, the file is not opened, however this would */
        /* normally cause the expiration and revision dates to be modified. */
        /* Using FIB$M_NORECORD prohibits this from happening. */
        status = sys$qiow(0,DevChan,IO$_MODIFY,&iosb,0,0,
                          &FibDesc,&FileName,0,0,&Atr,0);
        if (!(status & 1)) return(status);
 
        status = iosb[0];
        if (!(status & 1)) return(status);
 
        status = sys$dassgn(DevChan);
        if (!(status & 1)) return(status);
 
        /* look for next file, if none, no big deal.. */
        status = sys$search(&Fab);
    }
} /* end function change_zipfile_attributes() */
 

#if 0   /* save for possible later use */ 

char *cvt_time(date)
  long int date[2];
{
  static char str[27];
  static struct dsc$descriptor date_str={26,DSC$K_DTYPE_T,DSC$K_CLASS_S,&str};
 
  if ((date[0]==0) && (date[1]==0))
    return("none");
 
  sys$asctim(0,&date_str,date,0);
  str[26]='\0';
 
  return(&str);
}
 
bintim(time,binval)
  char *time;
  long int binval[2];
{
  static struct dsc$descriptor date_str={0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
  date_str.dsc$w_length = strlen(time);
  date_str.dsc$a_pointer = time;
 
  sys$bintim(&date_str,binval);
}
#endif /* 0 */ 
