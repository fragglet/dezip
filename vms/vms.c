/*************************************************************************
 *                                                                       *
 * Copyright (C) 1992 Igor Mandrichenko.                                 *
 * Permission is granted to any individual or institution to use, copy,  *
 * or redistribute this software so long as all of the original files    *
 * are included unmodified, that it is not sold for profit, and that     *
 * this copyright notice is retained.                                    *
 *                                                                       *
 *************************************************************************/

/*
 *    vms.c  by Igor Mandrichenko
 *    version 1.1-2
 *
 *    This module contains routines to extract VMS file attributes
 *    from extra field and create file with these attributes.  This
 *    source is mainly based on sources of file_io.c from UNZIP 4.1
 *    by Info-ZIP.  [Info-ZIP note:  very little of this code is from
 *    file_io.c; it has virtually been written from the ground up.
 *    Of the few lines which are from the older code, most are mine
 *    (G. Roelofs) and I make no claims upon them.  On the contrary,
 *    my/our thanks to Igor for his contributions!]
 */

/* 
 *	Revision history:
 *	1.0-1	Mandrichenko	16-feb-1992
 *		Recognize -c option
 *	1.0-2	Mandrichenko	17-feb-1992
 *		Do not use ASYnchroneous mode.
 *	1.0-3   Mandrichenko	2-mar-1992
 *		Make code more standard
 *		Use lrec instead of crec -- unzip4.2p do not provide 
 *		crec now.
 *      1.1	Mandrichenko	5-mar-1992  
 *		Make use of asynchronous output.
 *		Be ready to extract RMS blocks of invalid size (because diff
 *              VMS version used to compress).
 *	1.1-1	Mandrichenko	11-mar-1992
 *		Use internal file attributes saved in pInfo to decide
 *		if the file is text.  [GRR:  temporarily disabled, since
 *              no way to override and force binary extraction]
 *	1.1-2   Mandrichenko	13-mar-1992
 *		Do not restore owner/protection info if -X not specified.
 */

#ifdef VMS			/*	VMS only !	*/

/************************************/
/*  File_IO Includes, Defines, etc. */
/************************************/

#ifdef VAXC
#include rms
#include descrip
#include syidef
#else
#include <rms.h>
#include <descrip.h>
#include <syidef.h>
#endif

#include "unzip.h"

#define ERR(s) !((s) & 1)

#define BUFS512	8192*2	/* Must be a multiple of 512 */

static int WriteBuffer __((int fd, unsigned char *buf, int len));
static int _flush_blocks __((void));
static int _flush_records __((void));
static byte *extract_block __((byte *));

/*
*   Local static storage
*/
static struct FAB *outfab = 0;
static struct RAB *outrab = 0;
static struct FAB fileblk;
static struct XABFHC *xabfhc = 0;
static struct XABDAT dattim, *xabdat = 0;
static struct XABRDT *xabrdt = 0;
static struct XABPRO *xabpro = 0;
static struct XABKEY *xabkey = 0;
static struct XABALL *xaball = 0;
static struct RAB rab;

static int text_file = 0;

static char locbuf[BUFS512];
static int loccnt = 0;
static char *locptr;


struct bufdsc
{	struct bufdsc	*next;
	byte *buf;
	int bufcnt;
};

static struct bufdsc b1,b2,*curbuf;
static byte buf1[BUFS512],buf2[BUFS512];

int create_output_file()
{				/* return non-0 if sys$create failed */
    int ierr, yr, mo, dy, hh, mm, ss;
    char timbuf[24];		/* length = first entry in "stupid" + 1 */
    int attr_given = 0;		/* =1 if VMS attributes are present in
				*     extra_field */

    rab = cc$rms_rab;		/* fill FAB & RAB with default values */
    fileblk = cc$rms_fab;

    text_file = /* pInfo->text || */ aflag || cflag;

    if (attr_given = find_vms_attrs())
    {	text_file = 0;
	if( cflag )
	{	printf("Can not put VMS file %s to stdout.\n",
			filename);
		return 50;
	}
    }
	
    if (!attr_given)
    {
	outfab = &fileblk;
	outfab->fab$l_xab = 0L;
	if (text_file)
	{
	    outfab->fab$b_rfm = FAB$C_VAR;	/* variable length records */
	    outfab->fab$b_rat = FAB$M_CR;	/* carriage-return carriage ctrl */
	}
	else
	{
	    outfab->fab$b_rfm = FAB$C_STMLF;	/* stream-LF record format */
	    outfab->fab$b_rat = FAB$M_CR;	/* carriage-return carriage ctrl */
	}
    }

    if(!cflag)
    	outfab->fab$l_fna = filename;
    else
    	outfab->fab$l_fna = "sys$output:";

    outfab->fab$b_fns = strlen(outfab->fab$l_fna);

    if (!attr_given || xabdat == 0)
    {
	static char *month[] =
	    {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
	     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
	/*  fixed-length string descriptor (why not just a pointer to timbuf? sigh.) */
	struct dsc$descriptor stupid =
	    {23, DSC$K_DTYPE_T, DSC$K_CLASS_S, timbuf};

	yr = ((lrec.last_mod_file_date >> 9) & 0x7f) + 1980;	/* dissect date */
	mo = ((lrec.last_mod_file_date >> 5) & 0x0f) - 1;
	dy = (lrec.last_mod_file_date & 0x1f);
	hh = (lrec.last_mod_file_time >> 11) & 0x1f;	/* dissect time */
	mm = (lrec.last_mod_file_time >> 5) & 0x3f;
	ss = (lrec.last_mod_file_time & 0x1f) * 2;

	dattim = cc$rms_xabdat;	/* fill XAB with default values */
	dattim.xab$l_nxt = outfab->fab$l_xab;
	outfab->fab$l_xab = (char*)(xabdat = &dattim);

	sprintf(timbuf, "%02d-%3s-%04d %02d:%02d:%02d.00", dy, month[mo], yr,
		hh, mm, ss);
	sys$bintim(&stupid, &dattim.xab$q_cdt);
    }

#ifdef DEBUG
    printf("XAB chain before CREATE dump:\n");
    dump_rms_block(outfab);
    {
	struct XABALL *x;
	for (x = outfab->fab$l_xab; x != 0L; x = x->xab$l_nxt)
	    dump_rms_block(x);
    }
#endif

    outfab->fab$w_ifi = 0;	/* Clear IFI. It may be nonzero after ZIP */

    if ((ierr = sys$create(outfab)) != RMS$_NORMAL)
    {
	message("[ can not create output file ]\n", ierr);
	message("", outfab->fab$l_stv);
	fprintf(stderr, "Can't create output file:  %s\n", filename);
	return (1);
    }

    if (!text_file && !cflag)	/* Do not reopen text files and stdout
				*  Just open them in right mode		*/
    {
	/*
	*	Reopen file for Block I/O with no XABs.
	*/
	if ((ierr = sys$close(outfab)) != RMS$_NORMAL)
	{
#ifdef DEBUG
	    message("[ create_output_file: sys$close failed ]\n", ierr);
	    message("", outfab->fab$l_stv);
#endif
	    fprintf(stderr, "Can't create output file:  %s\n", filename);
	    return (1);
	}


	outfab->fab$b_fac = FAB$M_BIO | FAB$M_PUT;	/* Get ready for block
							 * output */
	outfab->fab$l_xab = 0L;	/* Unlink all XABs */

	if ((ierr = sys$open(outfab)) != RMS$_NORMAL)
	{
	    message("[ Can not open output file ]\n", ierr);
	    message("", outfab->fab$l_stv);
	    return (1);
	}
    }

    outrab = &rab;
    rab.rab$l_fab = outfab;
    if( !text_file ) rab.rab$l_rop |= RAB$M_BIO;
    if( !text_file ) rab.rab$l_rop |= RAB$M_ASY;
    rab.rab$b_rac = RAB$C_SEQ;

    if ((ierr = sys$connect(outrab)) != RMS$_NORMAL)
    {
#ifdef DEBUG
	fprintf(stderr, "create_output_file: sys$connect failed on file %s\n",
		filename);
	fprintf(stderr, "                    status = %d\n", ierr);
	fprintf(stderr, "                   fab.sts = %d\n", outfab->fab$l_sts);
	fprintf(stderr, "                   fab.stv = %d\n", outfab->fab$l_stv);
#endif
	fprintf(stderr, "Can't create output file:  %s\n", filename);
	return (1);
    }

    locptr = &locbuf[0];
    loccnt = 0;

    b1.buf = &buf1[0];
    b1.bufcnt = 0;
    b1.next = &b2;
    b2.buf = &buf2[0];
    b2.bufcnt = 0;
    b2.next = &b1;
    curbuf = &b1;

    return (0);
}

/*
*   Extra record format
*   ===================
*   signature 	    (2 bytes)	= 'I','M'
*   size	    (2 bytes)
*   block signature (4 bytes)
*   flags	    (2 bytes)
*   uncomprssed size(2 bytes) 
*   reserved	    (4 bytes) 
*   data	    ((size-12) bytes)
*   ....
*/

struct extra_block
{
    UWORD sig;			/* Extra field block header structure */
    UWORD size;
    ULONG bid;
    UWORD flags;
    UWORD length;
    ULONG reserved;
    byte body[1];
};

/*
 *   Extra field signature and block signatures
 */

#define SIGNATURE "IM"
#define FABL	(cc$rms_fab.fab$b_bln)
#define RABL	(cc$rms_rab.rab$b_bln)
#define XALLL	(cc$rms_xaball.xab$b_bln)
#define XDATL	(cc$rms_xabdat.xab$b_bln)
#define XFHCL	(cc$rms_xabfhc.xab$b_bln)
#define XKEYL	(cc$rms_xabkey.xab$b_bln)
#define XPROL	(cc$rms_xabpro.xab$b_bln)
#define XRDTL	(cc$rms_xabrdt.xab$b_bln)
#define XSUML	(cc$rms_xabsum.xab$b_bln)
#define EXTBSL  4		/* Block signature length   */
#define RESL	8		/* Rserved 8 bytes  */
#define EXTHL	(4+EXTBSL)
#define FABSIG	"VFAB"
#define XALLSIG	"VALL"
#define XFHCSIG	"VFHC"
#define XDATSIG	"VDAT"
#define XRDTSIG	"VRDT"
#define XPROSIG	"VPRO"
#define XKEYSIG	"VKEY"
#define XNAMSIG	"VNAM"
#define VERSIG  "VMSV"



#define W(p)	(*(unsigned short*)(p))
#define L(p)	(*(unsigned long*)(p))
#define EQL_L(a,b)	( L(a) == L(b) )
#define EQL_W(a,b)	( W(a) == W(b) )

/****************************************************************
 * Function find_vms_attrs scans ZIP entry extra field if any   *
 * and looks for VMS attribute records. Returns 0 if either no  *
 * attributes found or no fab given.                            *
 ****************************************************************/
int find_vms_attrs()
{
    byte *scan = extra_field;
    struct extra_block *blk;
    struct XABALL *first_xab = 0L, *last_xab = 0L;
    int len;

    outfab = xabfhc = xabdat = xabrdt = xabpro = 0L;

    if (scan == NULL)
	return 0;
/*
    if (crec.extra_field_length)
	len = crec.extra_field_length;
    else
*/
    len = lrec.extra_field_length;

#define LINK(p)	{ 					\
		if( first_xab == 0L )			\
			first_xab = p;			\
		if( last_xab != 0L )			\
			last_xab -> xab$l_nxt = p;	\
		last_xab = p;				\
		p -> xab$l_nxt = 0;			\
	}
    /* End of macro LINK */

    while (len > 0)
    {
	blk = (struct block *)scan;
	if (EQL_W(&blk->sig, SIGNATURE))
	{
	    byte *block_id;
	    block_id = &blk->bid;
	    if (EQL_L(block_id, FABSIG))
	    {
		outfab = (struct FAB *) extract_block(blk, 0,
						&cc$rms_fab, FABL);
	    }
	    else if (EQL_L(block_id, XALLSIG))
	    {
		xaball = (struct XABALL *) extract_block(blk, 0, 
						&cc$rms_xaball, XALLL);
		LINK(xaball);
	    }
	    else if (EQL_L(block_id, XKEYSIG))
	    {
		xabkey = (struct XABKEY *) extract_block(blk, 0,
						&cc$rms_xabkey, XKEYL);
		LINK(xabkey);
	    }
	    else if (EQL_L(block_id, XFHCSIG))
	    {
		xabfhc = (struct XABFHC *) extract_block(blk, 0,
						&cc$rms_xabfhc, XFHCL);
		LINK(xabfhc);
	    }
	    else if (EQL_L(block_id, XDATSIG))
	    {
		xabdat = (struct XABDAT *) extract_block(blk, 0,
						&cc$rms_xabdat, XDATL);
		LINK(xabdat);
	    }
	    else if (EQL_L(block_id, XRDTSIG))
	    {
		xabrdt = (struct XABRDT *) extract_block(blk, 0,
						&cc$rms_xabrdt, XRDTL);
		/*	LINK(xabrdt);	-- Do not link xabrdt	*/
	    }
	    else if (EQL_L(block_id, XPROSIG))
	    {
		xabpro = (struct XABPRO *) extract_block(blk, 0,
						&cc$rms_xabpro, XPROL);
		/*	LINK(xabpro);	-- Do not link xabpro
					   until close */
	    }
	    else if (EQL_L(block_id, VERSIG))
	    {
		char verbuf[80];
		int verlen = 0;
		int vl = 0;
		int item = SYI$_VERSION;
		$DESCRIPTOR(version, verbuf);
		byte *vers;

		lib$getsyi(&item, 0, &version, &verlen, 0, 0);
		verbuf[verlen] = 0;
		vers = extract_block(blk, &vl, 0, 0);
		if (strncmp(verbuf, vers, verlen))
		{
		    printf("[ Warning: VMS version mismatch.");

		    printf("   This version %s --", verbuf);
		    strncpy(verbuf, vers, vl);
		    verbuf[vl] = 0;
		    printf(" version made by %s ]\n", verbuf);
		}
		free(vers);
	    }
	    else
		fprintf(stderr, "[ Warning: Unknown block signature %s ]\n",
			block_id);
	}
	len -= blk->size + 4;
	scan += blk->size + 4;
    }
    if (outfab != 0)
    {
	outfab->fab$l_xab = first_xab;
	return 1;
    }
    else
	return 0;
}

/******************************
 *   Function extract_block   *
 ******************************/
/*
 *  Simple uncompression routne. The compression uses bit stream.
 *  Compression scheme:
 *
 *  if(byte!=0)
 *      putbit(1),putyte(byte)
 *  else
 *	putbit(0)
 */
static byte *extract_block(p, retlen, init, needlen)
    int *retlen;
struct extra_block *p;
byte *init;
int needlen;
{
    byte *block;		/* Pointer to block allocated */
    byte *bitptr;		/* Pointer into compressed data */
    byte *outptr;		/* Pointer into output block */
    UWORD length;
    ULONG bitbuf = 0;
    int bitcnt = 0;

#define _FILL 	if(bitcnt+8 <= 32)			\
		{	bitbuf |= (*bitptr++) << bitcnt;\
			bitcnt += 8;			\
		}


    if( p->flags & 1 )
	length = p->length;	/* Block is compressed */
    else
        length = p->size - EXTBSL - RESL;	/* Simple case, uncompressed */

    if( needlen == 0 )
	needlen = length;

    if(retlen)
	*retlen = needlen;

    if( (p->flags & 1) || (needlen > length) )
    {	if ((block = (byte*)malloc(needlen)) == NULL)
		return NULL;
    }
/*
    else if( needlen > length )
    {	if ((block = (byte*)malloc(needlen)) == NULL)
		return NULL;
    }
*/
    else outptr = block = &p->body[0];

    if(init && (length < needlen))
	memcpy(block,init,needlen);

    if ((p->flags & 1) == 0)
	return block;		/* Do nothing more if uncompressed */

    outptr = block;
    bitptr = &p->body[0];

    if(length > needlen)
	length = needlen;

    while (length--)
    {
	if (bitcnt <= 0)
	    _FILL;

	if (bitbuf & 1)
	{
	    bitbuf >>= 1;
	    if ((bitcnt -= 1) < 8)
		_FILL;
	    *outptr++ = (byte) bitbuf;
	    bitcnt -= 8;
	    bitbuf >>= 8;
	}
	else
	{
	    *outptr++ = 0;
	    bitcnt -= 1;
	    bitbuf >>= 1;
	}
    }
    return block;
}

/***************************/
/*  Function FlushOutput() */
/***************************/

int FlushOutput()
{				/* return PK-type error code */
    /* flush contents of output buffer */
    if (tflag)
    {				/* Do not output. Update CRC only */
	UpdateCRC(outbuf, outcnt);
	outpos += outcnt;
	outcnt = 0;
	outptr = outbuf;
	return 0;
    }
    else
	return text_file ? _flush_records(0) : _flush_blocks(0);
}

static int _flush_blocks(final_flag)	/* Asynchronous version */
  int final_flag;
/* 1 if this is the final flushout */
{
    int round;
    int rest;
    int off = 0;
    int out_count = outcnt;
    int status;

    while(out_count > 0)
    {	if( curbuf -> bufcnt < BUFS512 )
	{	int ncpy;
		ncpy = out_count > (BUFS512-curbuf->bufcnt) ? 
				BUFS512-curbuf->bufcnt : 
				out_count;
		memcpy(curbuf->buf + curbuf->bufcnt, outbuf+off, ncpy);
		out_count -= ncpy;
		curbuf -> bufcnt += ncpy;
		off += ncpy;
	}		
	if( curbuf -> bufcnt == BUFS512 )
	{
		status = WriteBuffer(curbuf->buf,curbuf->bufcnt);
		if(status)
			return status;
		curbuf = curbuf -> next;
		curbuf -> bufcnt = 0;
	}
    }

    UpdateCRC(outbuf, outcnt);
    outpos += outcnt;
    outcnt = 0;
    outptr = outbuf;

    return (final_flag && (curbuf->bufcnt > 0)) ? 
    	WriteBuffer(curbuf->buf,curbuf->bufcnt) :
	0;	/* 0:  no error */
}

#define RECORD_END(c)	((c) == CR || (c) == LF)

static int _flush_records(final_flag)
  int final_flag;
/* 1 if this is the final flushout */
{
    int rest;
    int end = 0, start = 0;
    int off = 0;

    if (outcnt == 0 && loccnt == 0)
	return 0;		/* Nothing to do ... */

    if (loccnt)
    {
	for (end = 0; end < outcnt && !RECORD_END(outbuf[end]);)
	    ++end;
	if (end >= outcnt)
	{
	    fprintf(stderr, "[ Warning: Record too long (%d) ]\n",
		    outcnt + loccnt);
	    if (WriteRecord(locbuf, loccnt))
		return (50);
	    memcpy(locbuf, outbuf, outcnt);
	    locptr = &locbuf[loccnt = outcnt];
	}
	else
	{
	    memcpy(locptr, outbuf, end);
	    if (WriteRecord(locbuf, loccnt + end))
		return (50);
	    loccnt = 0;
	    locptr = &locbuf;
	}
	start = end + 1;
    }

    do
    {
	while (start < outcnt && outbuf[start] == CR)	/* Skip CR's at the
							*  beginning of rec. */
	    ++start;
	/* Find record end */
	for (end = start; end < outcnt && !RECORD_END(outbuf[end]);)
	    ++end;

	if (end < outcnt)
	{			/* Record end found, write the record */
	    if (WriteRecord(outbuf + start, end - start))
		return (50);
	    /* Shift to the begining of the next record */
	    start = end + 1;
	}
    } while (start < outcnt && end < outcnt);

    rest = outcnt - start;

    if (rest > 0)
	if (final_flag)
	{
	    /* This is a final flush. Put out all remaining in
	    *  the buffer				*/
	    if (loccnt && WriteRecord(locbuf, loccnt))
		return (50);
	}
	else
	{
	    memcpy(locptr, outbuf + start, rest);
	    locptr += rest;
	    loccnt += rest;
	}
    UpdateCRC(outbuf, outcnt);
    outpos += outcnt;
    outcnt = 0;
    outptr = outbuf;
    return (0);			/* 0:  no error */
}

/***************************/
/*  Function WriteBuffer() */
/***************************/

static int WriteBuffer(buf, len)/* return 0 if successful, 1 if not */
  unsigned char *buf;
int len;
{
    int status;

    status = sys$wait(outrab);
#ifdef DEBUG
    if(ERR(status))
    {	message("[ Write buffer: sys$wait faled ]\n",status);
	message("",outrab->rab$l_sts);
	message("",outrab->rab$l_sts);
    }
#endif
    outrab->rab$w_rsz = len;
    outrab->rab$l_rbf = buf;

    if (ERR(status = sys$write(outrab)))
    {
	fprintf(stderr, "WriteBuffer: sys$write failed.\n",
		filename);
	fprintf(stderr, "                    status = %d\n", status);
	fprintf(stderr, "                  rab->sts = %d\n", outrab->rab$l_sts);
	fprintf(stderr, "                       stv = %d\n", outrab->rab$l_stv);
	return 50;
    }
    return (0);
}

/***************************/
/*  Function WriteRecord() */
/***************************/

static int WriteRecord(rec, len)/* return 0 if successful, 1 if not */
  unsigned char *rec;
int len;
{
    int status;

    sys$wait(outrab);
#ifdef DEBUG
    if(ERR(status))
    {	message("[ Write buffer: sys$wait faled ]\n",status);
	message("",outrab->rab$l_sts);
	message("",outrab->rab$l_sts);
    }
#endif
    outrab->rab$w_rsz = len;
    outrab->rab$l_rbf = rec;

    if (ERR(status = sys$put(outrab)))
    {
	fprintf(stderr, "WriteRecord: sys$put failed.\n",
		filename);
	fprintf(stderr, "                    status = %d\n", status);
	fprintf(stderr, "                  rab->sts = %d\n", outrab->rab$l_sts);
	fprintf(stderr, "                       stv = %d\n", outrab->rab$l_stv);
	return 50;
    }
    return (0);
}

/********************************/
/*  Function CloseOutputFile()  */
/********************************/

int CloseOutputFile()
{
    int status;

    if (text_file) _flush_records(1);
    else
	_flush_blocks(1);

    if ((outfab->fab$l_xab = xabrdt) != 0L)	/* Link XABPRO and XABRDT */
	xabrdt->xab$l_nxt = (secinf ? xabpro : 0L);
    else
	outfab->fab$l_xab = (secinf ? xabpro : 0L);

    sys$wait(outrab);

    status = sys$close(outfab);
#ifdef DEBUG
    if (ERR(status))
    {
	message("\r[ Warning: can not set owner/protection/time attributes ]\n", status);
	message("", outfab->fab$l_stv);
    }
#endif
}

#ifdef DEBUG
dump_rms_block(p)
  unsigned char *p;
{
    unsigned char bid, len;
    int err;
    char *type;
    char buf[132];
    int i;

    err = 0;
    bid = p[0];
    len = p[1];
    switch (bid)
    {
	case FAB$C_BID:
	    type = "FAB";
	    break;
	case XAB$C_ALL:
	    type = "xabALL";
	    break;
	case XAB$C_KEY:
	    type = "xabKEY";
	    break;
	case XAB$C_DAT:
	    type = "xabDAT";
	    break;
	case XAB$C_RDT:
	    type = "xabRDT";
	    break;
	case XAB$C_FHC:
	    type = "xabFHC";
	    break;
	case XAB$C_PRO:
	    type = "xabPRO";
	    break;
	default:
	    type = "Unknown";
	    err = 1;
	    break;
    }
    printf("Block @%08X of type %s (%d).", p, type, bid);
    if (err)
    {
	printf("\n");
	return;
    }
    printf(" Size = %d\n", len);
    printf(" Offset - Hex - Dec\n");
    for (i = 0; i < len; i += 8)
    {
	int j;
	printf("%3d - ", i);
	for (j = 0; j < 8; j++)
	    if (i + j < len)
		printf("%02X ", p[i + j]);
	    else
		printf("   ");
	printf(" - ");
	for (j = 0; j < 8; j++)
	    if (i + j < len)
		printf("%03d ", p[i + j]);
	    else
		printf("    ");
	printf("\n");
    }
}

#endif				/* DEBUG */

message(string, status)
    int status;
char *string;
{
    char msgbuf[256];
    $DESCRIPTOR(msgd, msgbuf);
    int msglen = 0;

    if (ERR(lib$sys_getmsg(&status, &msglen, &msgd, 0, 0)))
	fprintf(stderr, "%s[ VMS status = %d ]\n", string, status);
    else
    {
	msgbuf[msglen] = 0;
	fprintf(stderr, "%s[ %s ]\n", string, msgbuf);
    }
}


#endif				/* !VMS */
