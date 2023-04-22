/*
  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  extract.c

  This file contains the high-level routines ("driver routines") for extrac-
  ting and testing zipfile members.  It calls the low-level routines in files
  explode.c, inflate.c, unreduce.c and unshrink.c.

  Contains:  extract_or_test_files()
             store_info()
             find_compr_idx()
             extract_or_test_entrylist()
             extract_or_test_member()
             TestExtraField()
             test_compr_eb()
             memextract()
             memflush()
             extract_izvms_block()    (VMS or VMS_TEXT_CONV)
             set_deferred_symlink()   (SYMLINKS only)
             fnfilter()
             dircomp()                (SET_DIR_ATTRIB only)
             UZbunzip2()              (USE_BZIP2 only)

  ---------------------------------------------------------------------------*/

#include "unzip.h"
#include "crc32.h"
#include "crypt.h"

#define GRRDUMP(buf, len)                                   \
    {                                                       \
        int i, j;                                           \
                                                            \
        for (j = 0; j < (len) / 16; ++j) {                  \
            printf("        ");                             \
            for (i = 0; i < 16; ++i)                        \
                printf("%02x ", (uch) (buf)[i + (j << 4)]); \
            printf("\n        ");                           \
            for (i = 0; i < 16; ++i) {                      \
                char c = (char) (buf)[i + (j << 4)];        \
                                                            \
                if (c == '\n')                              \
                    printf("\\n ");                         \
                else if (c == '\r')                         \
                    printf("\\r ");                         \
                else                                        \
                    printf(" %c ", c);                      \
            }                                               \
            printf("\n");                                   \
        }                                                   \
        if ((len) % 16) {                                   \
            printf("        ");                             \
            for (i = j << 4; i < (len); ++i)                \
                printf("%02x ", (uch) (buf)[i]);            \
            printf("\n        ");                           \
            for (i = j << 4; i < (len); ++i) {              \
                char c = (char) (buf)[i];                   \
                                                            \
                if (c == '\n')                              \
                    printf("\\n ");                         \
                else if (c == '\r')                         \
                    printf("\\r ");                         \
                else                                        \
                    printf(" %c ", c);                      \
            }                                               \
            printf("\n");                                   \
        }                                                   \
    }

static int store_info(void);
static int extract_or_test_entrylist(unsigned numchunk, ulg *pfilnum,
                                     ulg *pnum_bad_pwd,
                                     zoff_t *pold_extra_bytes,
                                     unsigned *pnum_dirs, direntry **pdirlist,
                                     int error_in_archive);
static int extract_or_test_member(void);
static int TestExtraField(uch *ef, unsigned ef_len);
static int test_compr_eb(uch *eb, unsigned eb_size, unsigned compr_offset,
                         int (*test_uc_ebdata)(uch *eb, unsigned eb_size,
                                               uch *eb_ucptr, ulg eb_ucsize));
static void set_deferred_symlink(slinkentry *slnk_entry);
static int dircomp(const void *a, const void *b);

/*******************************/
/*  Strings used in extract.c  */
/*******************************/

static const char VersionMsg[] =
    "   skipping: %-22s  need %s compat. v%u.%u (can do v%u.%u)\n";
static const char ComprMsgNum[] =
    "   skipping: %-22s  unsupported compression method %u\n";
static const char ComprMsgName[] =
    "   skipping: %-22s  `%s' method not supported\n";
static const char CmprNone[] = "store";
static const char CmprShrink[] = "shrink";
static const char CmprReduce[] = "reduce";
static const char CmprImplode[] = "implode";
static const char CmprTokenize[] = "tokenize";
static const char CmprDeflate[] = "deflate";
static const char CmprDeflat64[] = "deflate64";
static const char CmprDCLImplode[] = "DCL implode";
static const char CmprBzip[] = "bzip2";
static const char CmprLZMA[] = "LZMA";
static const char CmprIBMTerse[] = "IBM/Terse";
static const char CmprIBMLZ77[] = "IBM LZ77";
static const char CmprWavPack[] = "WavPack";
static const char CmprPPMd[] = "PPMd";
static const char *ComprNames[NUM_METHODS] = {
    CmprNone,       CmprShrink,  CmprReduce,   CmprReduce,   CmprReduce,
    CmprReduce,     CmprImplode, CmprTokenize, CmprDeflate,  CmprDeflat64,
    CmprDCLImplode, CmprBzip,    CmprLZMA,     CmprIBMTerse, CmprIBMLZ77,
    CmprWavPack,    CmprPPMd};
static const unsigned ComprIDs[NUM_METHODS] = {
    STORED,   SHRUNK,    REDUCED1,  REDUCED2,    REDUCED3,    REDUCED4,
    IMPLODED, TOKENIZED, DEFLATED,  ENHDEFLATED, DCLIMPLODED, BZIPPED,
    LZMAED,   IBMTERSED, IBMLZ77ED, WAVPACKED,   PPMDED};
static const char FilNamMsg[] = "%s:  bad filename length (%s)\n";
static const char WarnNoMemCFName[] =
    "%s:  warning, no memory for comparison with local header\n";
static const char LvsCFNamMsg[] = "%s:  mismatching \"local\" filename (%s),\n\
         continuing with \"central\" filename version\n";
static const char GP11FlagsDiffer[] = "file #%lu (%s):\n\
         mismatch between local and central GPF bit 11 (\"UTF-8\"),\n\
         continuing with central flag (IsUTF8 = %d)\n";
static const char WrnStorUCSizCSizDiff[] =
    "%s:  ucsize %s <> csize %s for STORED entry\n\
         continuing with \"compressed\" size value\n";
static const char ExtFieldMsg[] = "%s:  bad extra field length (%s)\n";
static const char OffsetMsg[] = "file #%lu:  bad zipfile offset (%s):  %ld\n";
static const char ExtractMsg[] = "%8sing: %-22s  %s%s";
static const char LengthMsg[] =
    "%s  %s:  %s bytes required to uncompress to %s bytes;\n    %s\
      supposed to require %s bytes%s%s%s\n";

static const char BadFileCommLength[] = "%s:  bad file comment length\n";
static const char LocalHdrSig[] = "local header sig";
static const char BadLocalHdr[] = "file #%lu:  bad local header\n";
static const char AttemptRecompensate[] = "  (attempting to re-compensate)\n";
static const char BackslashPathSep[] =
    "warning:  %s appears to use backslashes as path separators\n";
static const char AbsolutePathWarning[] =
    "warning:  stripped absolute path spec from %s\n";
static const char SkipVolumeLabel[] = "   skipping: %-22s  %svolume label\n";

static const char DirlistEntryNoMem[] =
    "warning:  cannot alloc memory for dir times/permissions/UID/GID\n";
static const char DirlistSortNoMem[] =
    "warning:  cannot alloc memory to sort dir times/perms/etc.\n";
static const char DirlistSetAttrFailed[] =
    "warning:  set times/attribs failed for %s\n";
static const char DirlistFailAttrSum[] =
    "     failed setting times/attribs for %lu dir entries";

static const char SymLnkWarnNoMem[] =
    "warning:  deferred symlink (%s) failed:\n\
          out of memory\n";
static const char SymLnkWarnInvalid[] =
    "warning:  deferred symlink (%s) failed:\n\
          invalid placeholder file\n";
static const char SymLnkDeferred[] = "finishing deferred symbolic links:\n";
static const char SymLnkFinish[] = "  %-22s -> %s\n";

static const char ReplaceQuery[] =
    "replace %s? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
static const char AssumeNone[] =
    " NULL\n(EOF or read error, treating as \"[N]one\" ...)\n";
static const char NewNameQuery[] = "new name: ";
static const char InvalidResponse[] = "error:  invalid response [%s]\n";

static const char ErrorInArchive[] =
    "At least one %serror was detected in %s.\n";
static const char ZeroFilesTested[] = "Caution:  zero files tested in %s.\n";

static const char VMSFormatQuery[] =
    "\n%s:  stored in VMS format.  Extract anyway? (y/n) ";

static const char SkipCannotGetPasswd[] =
    "   skipping: %-22s  unable to get password\n";
static const char SkipIncorrectPasswd[] =
    "   skipping: %-22s  incorrect password\n";
static const char FilesSkipBadPasswd[] =
    "%lu file%s skipped because of incorrect password.\n";
static const char MaybeBadPasswd[] =
    "    (may instead be incorrect password)\n";

static const char NoErrInCompData[] =
    "No errors detected in compressed data of %s.\n";
static const char NoErrInTestedFiles[] =
    "No errors detected in %s for the %lu file%s tested.\n";
static const char FilesSkipped[] =
    "%lu file%s skipped because of unsupported compression or encoding.\n";

static const char ErrUnzipFile[] = "  error:  %s%s %s\n";
static const char ErrUnzipNoFile[] = "\n  error:  %s%s\n";
static const char NotEnoughMem[] = "not enough memory to ";
static const char InvalidComprData[] = "invalid compressed data to ";
static const char Inflate[] = "inflate";
static const char BUnzip[] = "bunzip";

static const char Explode[] = "explode";
static const char Unshrink[] = "unshrink";

static const char FileUnknownCompMethod[] = "%s:  unknown compression method\n";
static const char BadCRC[] = " bad CRC %08lx  (should be %08lx)\n";

/* TruncEAs[] also used in OS/2 mapname(), close_outfile() */
char const TruncEAs[] = " compressed EA data missing (%d bytes)%s";
char const TruncNTSD[] = " compressed WinNT security data missing (%d bytes)%s";

static const char InconsistEFlength[] = "bad extra-field entry:\n \
     EF block length (%u bytes) exceeds remaining EF data (%u bytes)\n";
static const char TooSmallEBlength[] = "bad extra-field entry:\n \
     EF block length (%u bytes) invalid (< %d)\n";
static const char InvalidComprDataEAs[] = " invalid compressed data for EAs\n";
static const char UnsuppNTSDVersEAs[] = " unsupported NTSD EAs version %d\n";
static const char BadCRC_EAs[] = " bad CRC for extended attributes\n";
static const char UnknComprMethodEAs[] =
    " unknown compression method for EAs (%u)\n";
static const char NotEnoughMemEAs[] = " out of memory while inflating EAs\n";
static const char UnknErrorEAs[] = " unknown error on extended attributes\n";

static const char UnsupportedExtraField[] =
    "\nerror:  unsupported extra-field compression type (%u)--skipping\n";
static const char BadExtraFieldCRC[] =
    "error [%s]:  bad extra-field CRC %08lx (should be %08lx)\n";
static const char NotEnoughMemCover[] =
    "error: not enough memory for bomb detection\n";
static const char OverlappedComponents[] =
    "error: invalid zip file with overlapped components (possible zip bomb)\n";

/* A growable list of spans. */
typedef zoff_t bound_t;
typedef struct {
    bound_t beg; /* start of the span */
    bound_t end; /* one past the end of the span */
} span_t;
typedef struct {
    span_t *span; /* allocated, distinct, and sorted list of spans */
    size_t num;   /* number of spans in the list */
    size_t max;   /* allocated number of spans (num <= max) */
} cover_t;

/*
 * Return the index of the first span in cover whose beg is greater than val.
 * If there is no such span, then cover->num is returned.
 */
static size_t cover_find(cover, val)
cover_t *cover;
bound_t val;
{
    size_t lo = 0, hi = cover->num;
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (val < cover->span[mid].beg)
            hi = mid;
        else
            lo = mid + 1;
    }
    return hi;
}

/* Return true if val lies within any one of the spans in cover. */
static int cover_within(cover, val)
cover_t *cover;
bound_t val;
{
    size_t pos = cover_find(cover, val);
    return pos > 0 && val < cover->span[pos - 1].end;
}

/*
 * Add a new span to the list, but only if the new span does not overlap any
 * spans already in the list. The new span covers the values beg..end-1. beg
 * must be less than end.
 *
 * Keep the list sorted and merge adjacent spans. Grow the allocated space for
 * the list as needed. On success, 0 is returned. If the new span overlaps any
 * existing spans, then 1 is returned and the new span is not added to the
 * list. If the new span is invalid because beg is greater than or equal to
 * end, then -1 is returned. If the list needs to be grown but the memory
 * allocation fails, then -2 is returned.
 */
static int cover_add(cover, beg, end)
cover_t *cover;
bound_t beg;
bound_t end;
{
    size_t pos;
    int prec, foll;

    if (beg >= end)
        /* The new span is invalid. */
        return -1;

    /* Find where the new span should go, and make sure that it does not
       overlap with any existing spans. */
    pos = cover_find(cover, beg);
    if ((pos > 0 && beg < cover->span[pos - 1].end) ||
        (pos < cover->num && end > cover->span[pos].beg))
        return 1;

    /* Check for adjacencies. */
    prec = pos > 0 && beg == cover->span[pos - 1].end;
    foll = pos < cover->num && end == cover->span[pos].beg;
    if (prec && foll) {
        /* The new span connects the preceding and following spans. Merge the
           following span into the preceding span, and delete the following
           span. */
        cover->span[pos - 1].end = cover->span[pos].end;
        cover->num--;
        memmove(cover->span + pos, cover->span + pos + 1,
                (cover->num - pos) * sizeof(span_t));
    } else if (prec)
        /* The new span is adjacent only to the preceding span. Extend the end
           of the preceding span. */
        cover->span[pos - 1].end = end;
    else if (foll)
        /* The new span is adjacent only to the following span. Extend the
           beginning of the following span. */
        cover->span[pos].beg = beg;
    else {
        /* The new span has gaps between both the preceding and the following
           spans. Assure that there is room and insert the span.  */
        if (cover->num == cover->max) {
            size_t max = cover->max == 0 ? 16 : cover->max << 1;
            span_t *span = realloc(cover->span, max * sizeof(span_t));
            if (span == NULL)
                return -2;
            cover->span = span;
            cover->max = max;
        }
        memmove(cover->span + pos + 1, cover->span + pos,
                (cover->num - pos) * sizeof(span_t));
        cover->num++;
        cover->span[pos].beg = beg;
        cover->span[pos].end = end;
    }
    return 0;
}

/**************************************/
/*  Function extract_or_test_files()  */
/**************************************/

int extract_or_test_files() /* return PK-type error code */
{
    unsigned i, j;
    zoff_t cd_bufstart;
    uch *cd_inptr;
    int cd_incnt;
    ulg filnum = 0L, blknum = 0L;
    int reached_end;
    int no_endsig_found;
    int error, error_in_archive = PK_COOL;
    int *fn_matched = NULL, *xn_matched = NULL;
    zucn_t members_processed;
    ulg num_skipped = 0L, num_bad_pwd = 0L;
    zoff_t old_extra_bytes = 0L;
    unsigned num_dirs = 0;
    direntry *dirlist = (direntry *) NULL,
             **sorted_dirlist = (direntry **) NULL;

    /*
     * First, two general initializations are applied. These have been moved
     * here from process_zipfiles() because they are only needed for accessing
     * and/or extracting the data content of the zip archive.
     */

    /* a) initialize the CRC table pointer (once) */
    if (CRC_32_TAB == NULL) {
        if ((CRC_32_TAB = get_crc_table()) == NULL) {
            return PK_MEM;
        }
    }

    /* b) check out if specified extraction root directory exists */
    if (uO.exdir != (char *) NULL && G.extract_flag) {
        G.create_dirs = !uO.fflag;
        if ((error = checkdir(uO.exdir, ROOT)) > MPN_INF_SKIP) {
            /* out of memory, or file in way */
            return (error == MPN_NOMEM ? PK_MEM : PK_ERR);
        }
    }

    /* One more: initialize cover structure for bomb detection. Start with
       spans that cover any extra bytes at the start, the central directory,
       the end of central directory record (including the Zip64 end of central
       directory locator, if present), and the Zip64 end of central directory
       record, if present. */
    if (G.cover == NULL) {
        G.cover = malloc(sizeof(cover_t));
        if (G.cover == NULL) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(NotEnoughMemCover)));
            return PK_MEM;
        }
        ((cover_t *) G.cover)->span = NULL;
        ((cover_t *) G.cover)->max = 0;
    }
    ((cover_t *) G.cover)->num = 0;
    if (cover_add((cover_t *) G.cover,
                  G.extra_bytes + G.ecrec.offset_start_central_directory,
                  G.extra_bytes + G.ecrec.offset_start_central_directory +
                      G.ecrec.size_central_directory) != 0) {
        Info(slide, 0x401, ((char *) slide, LoadFarString(NotEnoughMemCover)));
        return PK_MEM;
    }
    if ((G.extra_bytes != 0 &&
         cover_add((cover_t *) G.cover, 0, G.extra_bytes) != 0) ||
        (G.ecrec.have_ecr64 &&
         cover_add((cover_t *) G.cover, G.ecrec.ec64_start, G.ecrec.ec64_end) !=
             0) ||
        cover_add((cover_t *) G.cover, G.ecrec.ec_start, G.ecrec.ec_end) != 0) {
        Info(slide, 0x401,
             ((char *) slide, LoadFarString(OverlappedComponents)));
        return PK_BOMB;
    }

    /*---------------------------------------------------------------------------
        The basic idea of this function is as follows.  Since the central di-
        rectory lies at the end of the zipfile and the member files lie at the
        beginning or middle or wherever, it is not very desirable to simply
        read a central directory entry, jump to the member and extract it, and
        then jump back to the central directory.  In the case of a large zipfile
        this would lead to a whole lot of disk-grinding, especially if each mem-
        ber file is small.  Instead, we read from the central directory the per-
        tinent information for a block of files, then go extract/test the whole
        block.  Thus this routine contains two small(er) loops within a very
        large outer loop:  the first of the small ones reads a block of files
        from the central directory; the second extracts or tests each file; and
        the outer one loops over blocks.  There's some file-pointer positioning
        stuff in between, but that's about it.  Btw, it's because of this jump-
        ing around that we can afford to be lenient if an error occurs in one of
        the member files:  we should still be able to go find the other members,
        since we know the offset of each from the beginning of the zipfile.
      ---------------------------------------------------------------------------*/

    G.pInfo = G.info;

    G.newzip = TRUE;
    G.reported_backslash = FALSE;

    /* malloc space for check on unmatched filespecs (OK if one or both NULL) */
    if (G.filespecs > 0 && (fn_matched = (int *) malloc(
                                G.filespecs * sizeof(int))) != (int *) NULL)
        for (i = 0; i < G.filespecs; ++i)
            fn_matched[i] = FALSE;
    if (G.xfilespecs > 0 && (xn_matched = (int *) malloc(
                                 G.xfilespecs * sizeof(int))) != (int *) NULL)
        for (i = 0; i < G.xfilespecs; ++i)
            xn_matched[i] = FALSE;

    /*---------------------------------------------------------------------------
        Begin main loop over blocks of member files.  We know the entire central
        directory is on this disk:  we would not have any of this information
      un- less the end-of-central-directory record was on this disk, and we
      would not have gotten to this routine unless this is also the disk on
      which the central directory starts.  In practice, this had better be the
      ONLY disk in the archive, but we'll add multi-disk support soon.
      ---------------------------------------------------------------------------*/

    members_processed = 0;
    no_endsig_found = FALSE;
    reached_end = FALSE;
    while (!reached_end) {
        j = 0;

        /*
         * Loop through files in central directory, storing offsets, file
         * attributes, case-conversion and text-conversion flags until block
         * size is reached.
         */

        while ((j < DIR_BLKSIZ)) {
            G.pInfo = &G.info[j];

            if (readbuf(G.sig, 4) == 0) {
                error_in_archive = PK_EOF;
                reached_end = TRUE; /* ...so no more left to do */
                break;
            }
            if (memcmp(G.sig, central_hdr_sig, 4)) { /* is it a new entry? */
                /* no new central directory entry
                 * -> is the number of processed entries compatible with the
                 *    number of entries as stored in the end_central record?
                 */
                if ((members_processed &
                     (G.ecrec.have_ecr64 ? MASK_ZUCN64 : MASK_ZUCN16)) ==
                    G.ecrec.total_entries_central_dir) {
                    /* yes, so look if we ARE back at the end_central record
                     */
                    no_endsig_found =
                        ((memcmp(G.sig,
                                 (G.ecrec.have_ecr64 ? end_central64_sig
                                                     : end_central_sig),
                                 4) != 0) &&
                         (!G.ecrec.is_zip64_archive) &&
                         (memcmp(G.sig, end_central_sig, 4) != 0));
                } else {
                    /* no; we have found an error in the central directory
                     * -> report it and stop searching for more Zip entries
                     */
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(CentSigMsg),
                          j + blknum * DIR_BLKSIZ + 1));
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(ReportMsg)));
                    error_in_archive = PK_BADERR;
                }
                reached_end = TRUE; /* ...so no more left to do */
                break;
            }
            /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */
            if ((error = process_cdir_file_hdr()) != PK_COOL) {
                error_in_archive = error; /* only PK_EOF defined */
                reached_end = TRUE;       /* ...so no more left to do */
                break;
            }
            if ((error = do_string(G.crec.filename_length, DS_FN)) != PK_COOL) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) { /* fatal:  no more left to do */
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(FilNamMsg),
                          FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
            if ((error = do_string(G.crec.extra_field_length, EXTRA_FIELD)) !=
                0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) { /* fatal */
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(ExtFieldMsg),
                          FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
            if ((error = do_string(G.crec.file_comment_length, SKIP)) !=
                PK_COOL) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) { /* fatal */
                    Info(slide, 0x421,
                         ((char *) slide, LoadFarString(BadFileCommLength),
                          FnFilter1(G.filename)));
                    reached_end = TRUE;
                    break;
                }
            }
            if (G.process_all_files) {
                if (store_info())
                    ++j; /* file is OK; info[] stored; continue with next */
                else
                    ++num_skipped;
            } else {
                int do_this_file;

                if (G.filespecs == 0)
                    do_this_file = TRUE;
                else { /* check if this entry matches an `include' argument */
                    do_this_file = FALSE;
                    for (i = 0; i < G.filespecs; i++)
                        if (match(G.filename, G.pfnames[i], uO.C_flag)) {
                            do_this_file = TRUE; /* ^-- ignore case or not? */
                            if (fn_matched)
                                fn_matched[i] = TRUE;
                            break; /* found match, so stop looping */
                        }
                }
                if (do_this_file) { /* check if this is an excluded file */
                    for (i = 0; i < G.xfilespecs; i++)
                        if (match(G.filename, G.pxnames[i], uO.C_flag)) {
                            do_this_file = FALSE; /* ^-- ignore case or not? */
                            if (xn_matched)
                                xn_matched[i] = TRUE;
                            break;
                        }
                }
                if (do_this_file) {
                    if (store_info())
                        ++j; /* file is OK */
                    else
                        ++num_skipped; /* unsupp. compression or encryption */
                }
            } /* end if (process_all_files) */

            members_processed++;

        } /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cd_bufstart = G.cur_zipfile_bufstart;
        cd_inptr = G.inptr;
        cd_incnt = G.incnt;

        /*-----------------------------------------------------------------------
            Second loop:  process files in current block, extracting or testing
            each one.
          -----------------------------------------------------------------------*/

        error = extract_or_test_entrylist(j, &filnum, &num_bad_pwd,
                                          &old_extra_bytes, &num_dirs, &dirlist,
                                          error_in_archive);
        if (error != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;
            /* ...and keep going (unless disk full or user break) */
            if (G.disk_full > 1 || error_in_archive == IZ_CTRLC ||
                error == PK_BOMB) {
                /* clear reached_end to signal premature stop ... */
                reached_end = FALSE;
                /* ... and cancel scanning the central directory */
                break;
            }
        }

        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */

        G.cur_zipfile_bufstart = lseek(G.zipfd, cd_bufstart, SEEK_SET);
        read(G.zipfd, (char *) G.inbuf, INBUFSIZ); /* been here before... */
        G.inptr = cd_inptr;
        G.incnt = cd_incnt;
        ++blknum;

#ifdef TEST
        printf("\ncd_bufstart = %ld (%.8lXh)\n", cd_bufstart, cd_bufstart);
        printf("cur_zipfile_bufstart = %ld (%.8lXh)\n", cur_zipfile_bufstart,
               cur_zipfile_bufstart);
        printf("inptr-inbuf = %d\n", G.inptr - G.inbuf);
        printf("incnt = %d\n\n", G.incnt);
#endif

    } /* end while-loop (blocks of files in central directory) */

    /*---------------------------------------------------------------------------
        Process the list of deferred symlink extractions and finish up
        the symbolic links.
      ---------------------------------------------------------------------------*/

    if (G.slink_last != NULL) {
        if (QCOND2)
            Info(slide, 0, ((char *) slide, LoadFarString(SymLnkDeferred)));
        while (G.slink_head != NULL) {
            set_deferred_symlink(G.slink_head);
            /* remove the processed entry from the chain and free its memory */
            G.slink_last = G.slink_head;
            G.slink_head = G.slink_last->next;
            free(G.slink_last);
        }
        G.slink_last = NULL;
    }

    /*---------------------------------------------------------------------------
        Go back through saved list of directories, sort and set times/perms/UIDs
        and GIDs from the deepest level on up.
      ---------------------------------------------------------------------------*/

    if (num_dirs > 0) {
        sorted_dirlist = (direntry **) malloc(num_dirs * sizeof(direntry *));
        if (sorted_dirlist == (direntry **) NULL) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(DirlistSortNoMem)));
            while (dirlist != (direntry *) NULL) {
                direntry *d = dirlist;

                dirlist = dirlist->next;
                free(d);
            }
        } else {
            ulg ndirs_fail = 0;

            if (num_dirs == 1)
                sorted_dirlist[0] = dirlist;
            else {
                for (i = 0; i < num_dirs; ++i) {
                    sorted_dirlist[i] = dirlist;
                    dirlist = dirlist->next;
                }
                qsort((char *) sorted_dirlist, num_dirs, sizeof(direntry *),
                      dircomp);
            }

            Trace((stderr, "setting directory times/perms/attributes\n"));
            for (i = 0; i < num_dirs; ++i) {
                direntry *d = sorted_dirlist[i];

                Trace((stderr, "dir = %s\n", d->fn));
                if ((error = set_direc_attribs(d)) != PK_OK) {
                    ndirs_fail++;
                    Info(slide, 0x201,
                         ((char *) slide, LoadFarString(DirlistSetAttrFailed),
                          d->fn));
                    if (!error_in_archive)
                        error_in_archive = error;
                }
                free(d);
            }
            free(sorted_dirlist);
            if (!uO.tflag && QCOND2) {
                if (ndirs_fail > 0)
                    Info(slide, 0,
                         ((char *) slide, LoadFarString(DirlistFailAttrSum),
                          ndirs_fail));
            }
        }
    }

    /*---------------------------------------------------------------------------
        Check for unmatched filespecs on command line and print warning if any
        found.  Free allocated memory.  (But suppress check when central dir
        scan was interrupted prematurely.)
      ---------------------------------------------------------------------------*/

    if (fn_matched) {
        if (reached_end)
            for (i = 0; i < G.filespecs; ++i)
                if (!fn_matched[i]) {
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(FilenameNotMatched),
                          G.pfnames[i]));
                    if (error_in_archive <= PK_WARN)
                        error_in_archive = PK_FIND; /* some files not found */
                }
        free((void *) fn_matched);
    }
    if (xn_matched) {
        if (reached_end)
            for (i = 0; i < G.xfilespecs; ++i)
                if (!xn_matched[i])
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(ExclFilenameNotMatched),
                          G.pxnames[i]));
        free((void *) xn_matched);
    }

    /*---------------------------------------------------------------------------
        Now, all locally allocated memory has been released.  When the central
        directory processing has been interrupted prematurely, it is safe to
        return immediately.  All completeness checks and summary messages are
        skipped in this case.
      ---------------------------------------------------------------------------*/
    if (!reached_end)
        return error_in_archive;

    /*---------------------------------------------------------------------------
        Double-check that we're back at the end-of-central-directory record, and
        print quick summary of results, if we were just testing the archive.  We
        send the summary to stdout so that people doing the testing in the back-
        ground and redirecting to a file can just do a "tail" on the output
      file.
      ---------------------------------------------------------------------------*/

    if (no_endsig_found) { /* just to make sure */
        Info(slide, 0x401, ((char *) slide, LoadFarString(EndSigMsg)));
        Info(slide, 0x401, ((char *) slide, LoadFarString(ReportMsg)));
        if (!error_in_archive) /* don't overwrite stronger error */
            error_in_archive = PK_WARN;
    }
    if (uO.tflag) {
        ulg num = filnum - num_bad_pwd;

        if (uO.qflag < 2) { /* GRR 930710:  was (uO.qflag == 1) */
            if (error_in_archive)
                Info(slide, 0,
                     ((char *) slide, LoadFarString(ErrorInArchive),
                      (error_in_archive == PK_WARN) ? "warning-" : "",
                      G.zipfn));
            else if (num == 0L)
                Info(slide, 0,
                     ((char *) slide, LoadFarString(ZeroFilesTested), G.zipfn));
            else if (G.process_all_files && (num_skipped + num_bad_pwd == 0L))
                Info(slide, 0,
                     ((char *) slide, LoadFarString(NoErrInCompData), G.zipfn));
            else
                Info(slide, 0,
                     ((char *) slide, LoadFarString(NoErrInTestedFiles),
                      G.zipfn, num, (num == 1L) ? "" : "s"));
            if (num_skipped > 0L)
                Info(slide, 0,
                     ((char *) slide, LoadFarString(FilesSkipped), num_skipped,
                      (num_skipped == 1L) ? "" : "s"));
            if (num_bad_pwd > 0L)
                Info(slide, 0,
                     ((char *) slide, LoadFarString(FilesSkipBadPasswd),
                      num_bad_pwd, (num_bad_pwd == 1L) ? "" : "s"));
        }
    }

    /* give warning if files not tested or extracted (first condition can still
     * happen if zipfile is empty and no files specified on command line) */

    if ((filnum == 0) && error_in_archive <= PK_WARN) {
        if (num_skipped > 0L)
            error_in_archive = IZ_UNSUP; /* unsupport. compression/encryption */
        else
            error_in_archive = PK_FIND; /* no files found at all */
    } else if ((filnum == num_bad_pwd) && error_in_archive <= PK_WARN)
        error_in_archive = IZ_BADPWD; /* bad passwd => all files skipped */
    else if ((num_skipped > 0L) && error_in_archive <= PK_WARN)
        error_in_archive = IZ_UNSUP; /* was PK_WARN; Jean-loup complained */
    else if ((num_bad_pwd > 0L) && !error_in_archive)
        error_in_archive = PK_WARN;

    return error_in_archive;

} /* end function extract_or_test_files() */

/***************************/
/*  Function store_info()  */
/***************************/

static int store_info() /* return 0 if skipping, 1 if OK */
{
#define UNKN_BZ2 (G.crec.compression_method != BZIPPED)

#ifdef USE_LZMA
#define UNKN_LZMA (G.crec.compression_method != LZMAED)
#else
#define UNKN_LZMA TRUE /* LZMA unknown */
#endif

#ifdef USE_WAVP
#define UNKN_WAVP (G.crec.compression_method != WAVPACKED)
#else
#define UNKN_WAVP TRUE /* WavPack unknown */
#endif

#ifdef USE_PPMD
#define UNKN_PPMD (G.crec.compression_method != PPMDED)
#else
#define UNKN_PPMD TRUE /* PPMd unknown */
#endif

#define UNKN_RED                              \
    (G.crec.compression_method >= REDUCED1 && \
     G.crec.compression_method <= REDUCED4)
#define UNKN_SHR FALSE /* unshrinking not unknown */
#define UNKN_COMPR                                                        \
    (UNKN_RED || UNKN_SHR || G.crec.compression_method == TOKENIZED ||    \
     (G.crec.compression_method > ENHDEFLATED && UNKN_BZ2 && UNKN_LZMA && \
      UNKN_WAVP && UNKN_PPMD))

#if (defined(USE_BZIP2) && (UNZIP_VERSION < UNZIP_BZ2VERS))
    int unzvers_support = (UNKN_BZ2 ? UNZIP_VERSION : UNZIP_BZ2VERS);
#define UNZVERS_SUPPORT unzvers_support
#else
#define UNZVERS_SUPPORT UNZIP_VERSION
#endif

    /*---------------------------------------------------------------------------
        Check central directory info for version/compatibility requirements.
      ---------------------------------------------------------------------------*/

    G.pInfo->encrypted = G.crec.general_purpose_bit_flag & 1; /* bit field */
    G.pInfo->ExtLocHdr = (G.crec.general_purpose_bit_flag & 8) == 8; /* bit */
    G.pInfo->textfile = G.crec.internal_file_attributes & 1; /* bit field */
    G.pInfo->crc = G.crec.crc32;
    G.pInfo->compr_size = G.crec.csize;
    G.pInfo->uncompr_size = G.crec.ucsize;

    switch (uO.aflag) {
    case 0:
        G.pInfo->textmode = FALSE; /* bit field */
        break;
    case 1:
        G.pInfo->textmode = G.pInfo->textfile; /* auto-convert mode */
        break;
    default: /* case 2: */
        G.pInfo->textmode = TRUE;
        break;
    }

    if (G.crec.version_needed_to_extract[1] == VMS_) {
        if (G.crec.version_needed_to_extract[0] > VMS_UNZIP_VERSION) {
            if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(VersionMsg),
                      FnFilter1(G.filename), "VMS",
                      G.crec.version_needed_to_extract[0] / 10,
                      G.crec.version_needed_to_extract[0] % 10,
                      VMS_UNZIP_VERSION / 10, VMS_UNZIP_VERSION % 10));
            return 0;
        } else if (!uO.tflag && !IS_OVERWRT_ALL) { /* if -o, extract anyway */
            Info(slide, 0x481,
                 ((char *) slide, LoadFarString(VMSFormatQuery),
                  FnFilter1(G.filename)));
            fgets(G.answerbuf, sizeof(G.answerbuf), stdin);
            if ((*G.answerbuf != 'y') && (*G.answerbuf != 'Y'))
                return 0;
        }
        /* usual file type:  don't need VMS to extract */
    } else if (G.crec.version_needed_to_extract[0] > UNZVERS_SUPPORT) {
        if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(VersionMsg),
                  FnFilter1(G.filename), "PK",
                  G.crec.version_needed_to_extract[0] / 10,
                  G.crec.version_needed_to_extract[0] % 10,
                  UNZVERS_SUPPORT / 10, UNZVERS_SUPPORT % 10));
        return 0;
    }

    if (UNKN_COMPR) {
        if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))) {
            unsigned cmpridx;

            if ((cmpridx = find_compr_idx(G.crec.compression_method)) <
                NUM_METHODS)
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(ComprMsgName),
                      FnFilter1(G.filename),
                      LoadFarStringSmall(ComprNames[cmpridx])));
            else
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(ComprMsgNum),
                      FnFilter1(G.filename), G.crec.compression_method));
        }
        return 0;
    }

    /* store a copy of the central header filename for later comparison */
    if ((G.pInfo->cfilname = malloc(strlen(G.filename) + 1)) == NULL) {
        Info(slide, 0x401,
             ((char *) slide, LoadFarString(WarnNoMemCFName),
              FnFilter1(G.filename)));
    } else
        strcpy(G.pInfo->cfilname, G.filename);

    /* map whatever file attributes we have into the local format */
    mapattr(); /* GRR:  worry about return value later */

    G.pInfo->diskstart = G.crec.disk_number_start;
    G.pInfo->offset = (zoff_t) G.crec.relative_offset_local_header;
    return 1;

} /* end function store_info() */

/*******************************/
/*  Function find_compr_idx()  */
/*******************************/

unsigned find_compr_idx(compr_methodnum)
unsigned compr_methodnum;
{
    unsigned i;

    for (i = 0; i < NUM_METHODS; i++) {
        if (ComprIDs[i] == compr_methodnum)
            break;
    }
    return i;
}

/******************************************/
/*  Function extract_or_test_entrylist()  */
/******************************************/

static int
extract_or_test_entrylist(numchunk, pfilnum, pnum_bad_pwd, pold_extra_bytes,
                          pnum_dirs, pdirlist,
                          error_in_archive) /* return PK-type error code */
unsigned numchunk;
ulg *pfilnum;
ulg *pnum_bad_pwd;
zoff_t *pold_extra_bytes;
unsigned *pnum_dirs;
direntry **pdirlist;
int error_in_archive;
{
    unsigned i;
    int renamed, query;
    int skip_entry;
    zoff_t bufstart, inbuf_offset, request;
    int error, errcode;

/* possible values for local skip_entry flag: */
#define SKIP_NO         0 /* do not skip this entry */
#define SKIP_Y_EXISTING 1 /* skip this entry, do not overwrite file */
#define SKIP_Y_NONEXIST 2 /* skip this entry, do not create new file */

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

    for (i = 0; i < numchunk; ++i) {
        (*pfilnum)++; /* *pfilnum = i + blknum*DIR_BLKSIZ + 1; */
        G.pInfo = &G.info[i];

        /* if the target position is not within the current input buffer
         * (either haven't yet read far enough, or (maybe) skipping back-
         * ward), skip to the target position and reset readbuf(). */

        /* seek_zipf(pInfo->offset);  */
        request = G.pInfo->offset + G.extra_bytes;
        if (cover_within((cover_t *) G.cover, request)) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(OverlappedComponents)));
            return PK_BOMB;
        }
        inbuf_offset = request % INBUFSIZ;
        bufstart = request - inbuf_offset;

        Trace((stderr, "\ndebug: request = %ld, inbuf_offset = %ld\n",
               (long) request, (long) inbuf_offset));
        Trace((stderr, "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
               (long) bufstart, (long) G.cur_zipfile_bufstart));
        if (request < 0) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarStringSmall(SeekMsg), G.zipfn,
                  LoadFarString(ReportMsg)));
            error_in_archive = PK_ERR;
            if (*pfilnum == 1 && G.extra_bytes != 0L) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(AttemptRecompensate)));
                *pold_extra_bytes = G.extra_bytes;
                G.extra_bytes = 0L;
                request = G.pInfo->offset; /* could also check if != 0 */
                inbuf_offset = request % INBUFSIZ;
                bufstart = request - inbuf_offset;
                Trace((stderr, "debug: request = %ld, inbuf_offset = %ld\n",
                       (long) request, (long) inbuf_offset));
                Trace((stderr,
                       "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
                       (long) bufstart, (long) G.cur_zipfile_bufstart));
                /* try again */
                if (request < 0) {
                    Trace((stderr, "debug: recompensated request still < 0\n"));
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(SeekMsg), G.zipfn,
                          LoadFarString(ReportMsg)));
                    error_in_archive = PK_BADERR;
                    continue;
                }
            } else {
                error_in_archive = PK_BADERR;
                continue; /* this one hosed; try next */
            }
        }

        if (bufstart != G.cur_zipfile_bufstart) {
            Trace((stderr, "debug: bufstart != cur_zipfile_bufstart\n"));
            G.cur_zipfile_bufstart = lseek(G.zipfd, bufstart, SEEK_SET);
            if ((G.incnt = read(G.zipfd, (char *) G.inbuf, INBUFSIZ)) <= 0) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(OffsetMsg), *pfilnum,
                      "lseek", (long) bufstart));
                error_in_archive = PK_BADERR;
                continue; /* can still do next file */
            }
            G.inptr = G.inbuf + (int) inbuf_offset;
            G.incnt -= (int) inbuf_offset;
        } else {
            G.incnt += (int) (G.inptr - G.inbuf) - (int) inbuf_offset;
            G.inptr = G.inbuf + (int) inbuf_offset;
        }

        /* should be in proper position now, so check for sig */
        if (readbuf(G.sig, 4) == 0) { /* bad offset */
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(OffsetMsg), *pfilnum, "EOF",
                  (long) request));
            error_in_archive = PK_BADERR;
            continue; /* but can still try next one */
        }
        if (memcmp(G.sig, local_hdr_sig, 4)) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(OffsetMsg), *pfilnum,
                  LoadFarStringSmall(LocalHdrSig), (long) request));
            /*
                GRRDUMP(G.sig, 4)
                GRRDUMP(local_hdr_sig, 4)
             */
            error_in_archive = PK_ERR;
            if ((*pfilnum == 1 && G.extra_bytes != 0L) ||
                (G.extra_bytes == 0L && *pold_extra_bytes != 0L)) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(AttemptRecompensate)));
                if (G.extra_bytes) {
                    *pold_extra_bytes = G.extra_bytes;
                    G.extra_bytes = 0L;
                } else
                    G.extra_bytes = *pold_extra_bytes; /* third attempt */
                if (((error = seek_zipf(G.pInfo->offset)) != PK_OK) ||
                    (readbuf(G.sig, 4) == 0)) { /* bad offset */
                    if (error != PK_BADERR)
                        Info(slide, 0x401,
                             ((char *) slide, LoadFarString(OffsetMsg),
                              *pfilnum, "EOF", (long) request));
                    error_in_archive = PK_BADERR;
                    continue; /* but can still try next one */
                }
                if (memcmp(G.sig, local_hdr_sig, 4)) {
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(OffsetMsg), *pfilnum,
                          LoadFarStringSmall(LocalHdrSig), (long) request));
                    error_in_archive = PK_BADERR;
                    continue;
                }
            } else
                continue; /* this one hosed; try next */
        }
        if ((error = process_local_file_hdr()) != PK_COOL) {
            Info(slide, 0x421,
                 ((char *) slide, LoadFarString(BadLocalHdr), *pfilnum));
            error_in_archive = error; /* only PK_EOF defined */
            continue;                 /* can still try next one */
        }
        if (((G.lrec.general_purpose_bit_flag & (1 << 11)) == (1 << 11)) !=
            (G.pInfo->GPFIsUTF8 != 0)) {
            if (QCOND2) {
#define cFile_PrintBuf G.pInfo->cfilname
                Info(slide, 0x421,
                     ((char *) slide, LoadFarStringSmall2(GP11FlagsDiffer),
                      *pfilnum, FnFilter1(cFile_PrintBuf), G.pInfo->GPFIsUTF8));
#undef cFile_PrintBuf
            }
            if (error_in_archive < PK_WARN)
                error_in_archive = PK_WARN;
        }
        if ((error = do_string(G.lrec.filename_length, DS_FN_L)) != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(FilNamMsg),
                      FnFilter1(G.filename), "local"));
                continue; /* go on to next one */
            }
        }
        if (G.extra_field != (uch *) NULL) {
            free(G.extra_field);
            G.extra_field = (uch *) NULL;
        }
        if ((error = do_string(G.lrec.extra_field_length, EXTRA_FIELD)) != 0) {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(ExtFieldMsg),
                      FnFilter1(G.filename), "local"));
                continue; /* go on */
            }
        }
        /* Filename consistency checks must come after reading in the local
         * extra field, so that a UTF-8 entry name e.f. block has already
         * been processed.
         */
        if (G.pInfo->cfilname != (char *) NULL) {
            if (strcmp(G.pInfo->cfilname, G.filename) != 0) {
#define cFile_PrintBuf G.pInfo->cfilname
                Info(slide, 0x401,
                     ((char *) slide, LoadFarStringSmall2(LvsCFNamMsg),
                      FnFilter2(cFile_PrintBuf), FnFilter1(G.filename)));
#undef cFile_PrintBuf
                strcpy(G.filename, G.pInfo->cfilname);
                if (error_in_archive < PK_WARN)
                    error_in_archive = PK_WARN;
            }
            free(G.pInfo->cfilname);
            G.pInfo->cfilname = (char *) NULL;
        }
        /* Size consistency checks must come after reading in the local extra
         * field, so that any Zip64 extension local e.f. block has already
         * been processed.
         */
        if (G.lrec.compression_method == STORED) {
            zusz_t csiz_decrypted = G.lrec.csize;

            if (G.pInfo->encrypted) {
                if (csiz_decrypted < 12) {
                    /* handle the error now to prevent unsigned overflow */
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate)));
                    return PK_ERR;
                }
                csiz_decrypted -= 12;
            }
            if (G.lrec.ucsize != csiz_decrypted) {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarStringSmall2(WrnStorUCSizCSizDiff),
                      FnFilter1(G.filename), FmZofft(G.lrec.ucsize, NULL, "u"),
                      FmZofft(csiz_decrypted, NULL, "u")));
                G.lrec.ucsize = csiz_decrypted;
                if (error_in_archive < PK_WARN)
                    error_in_archive = PK_WARN;
            }
        }

        if (G.pInfo->encrypted && (error = decrypt(uO.pwdarg)) != PK_COOL) {
            if (error == PK_WARN) {
                if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(SkipIncorrectPasswd),
                          FnFilter1(G.filename)));
                ++(*pnum_bad_pwd);
            } else { /* (error > PK_WARN) */
                if (error > error_in_archive)
                    error_in_archive = error;
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(SkipCannotGetPasswd),
                      FnFilter1(G.filename)));
            }
            continue; /* go on to next file */
        }

        /*
         * just about to extract file:  if extracting to disk, check if
         * already exists, and if so, take appropriate action according to
         * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
         * loop because we don't store the possibly renamed filename[] in
         * info[])
         */
        if (!uO.tflag && !uO.cflag) {
            renamed = FALSE; /* user hasn't renamed output file yet */

        startover:
            query = FALSE;
            skip_entry = SKIP_NO;
            /* for files from DOS FAT, check for use of backslash instead
             *  of slash as directory separator (bug in some zipper(s); so
             *  far, not a problem in HPFS, NTFS or VFAT systems)
             */
            if (G.pInfo->hostnum == FS_FAT_ && !MBSCHR(G.filename, '/')) {
                char *p = G.filename;

                if (*p)
                    do {
                        if (*p == '\\') {
                            if (!G.reported_backslash) {
                                Info(slide, 0x21,
                                     ((char *) slide,
                                      LoadFarString(BackslashPathSep),
                                      G.zipfn));
                                G.reported_backslash = TRUE;
                                if (!error_in_archive)
                                    error_in_archive = PK_WARN;
                            }
                            *p = '/';
                        }
                    } while (*PREINCSTR(p));
            }

            if (!renamed) {
                /* remove absolute path specs */
                if (G.filename[0] == '/') {
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(AbsolutePathWarning),
                          FnFilter1(G.filename)));
                    if (!error_in_archive)
                        error_in_archive = PK_WARN;
                    do {
                        char *p = G.filename + 1;
                        do {
                            *(p - 1) = *p;
                        } while (*p++ != '\0');
                    } while (G.filename[0] == '/');
                }
            }

            /* mapname can create dirs if not freshening or if renamed */
            error = mapname(renamed);
            if ((errcode = error & ~MPN_MASK) != PK_OK &&
                error_in_archive < errcode)
                error_in_archive = errcode;
            if ((errcode = error & MPN_MASK) > MPN_INF_TRUNC) {
                if (errcode == MPN_CREATED_DIR) {
                    direntry *d_entry;

                    error = defer_dir_attribs(&d_entry);
                    if (d_entry == (direntry *) NULL) {
                        /* There may be no dir_attribs info available, or
                         * we have encountered a mem allocation error.
                         * In case of an error, report it and set program
                         * error state to warning level.
                         */
                        if (error) {
                            Info(slide, 0x401,
                                 ((char *) slide,
                                  LoadFarString(DirlistEntryNoMem)));
                            if (!error_in_archive)
                                error_in_archive = PK_WARN;
                        }
                    } else {
                        d_entry->next = (*pdirlist);
                        (*pdirlist) = d_entry;
                        ++(*pnum_dirs);
                    }
                } else if (errcode == MPN_VOL_LABEL) {
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(SkipVolumeLabel),
                          FnFilter1(G.filename), ""));
                } else if (errcode > MPN_INF_SKIP && error_in_archive < PK_ERR)
                    error_in_archive = PK_ERR;
                Trace((stderr, "mapname(%s) returns error code = %d\n",
                       FnFilter1(G.filename), error));
                continue; /* go on to next file */
            }

            switch (check_for_newer(G.filename)) {
            case DOES_NOT_EXIST:
                /* freshen (no new files): skip unless just renamed */
                if (uO.fflag && !renamed)
                    skip_entry = SKIP_Y_NONEXIST;
                break;
            case EXISTS_AND_OLDER: {
                if (IS_OVERWRT_NONE)
                    /* never overwrite:  skip file */
                    skip_entry = SKIP_Y_EXISTING;
                else if (!IS_OVERWRT_ALL)
                    query = TRUE;
            } break;
            case EXISTS_AND_NEWER: /* (or equal) */
                if (IS_OVERWRT_NONE || (uO.uflag && !renamed)) {
                    /* skip if update/freshen & orig name */
                    skip_entry = SKIP_Y_EXISTING;
                } else {
                    if (!IS_OVERWRT_ALL)
                        query = TRUE;
                }
                break;
            }
            if (query) {
                extent fnlen;
            reprompt:
                Info(slide, 0x81,
                     ((char *) slide, LoadFarString(ReplaceQuery),
                      FnFilter1(G.filename)));
                if (fgets(G.answerbuf, sizeof(G.answerbuf), stdin) ==
                    (char *) NULL) {
                    Info(slide, 1, ((char *) slide, LoadFarString(AssumeNone)));
                    *G.answerbuf = 'N';
                    if (!error_in_archive)
                        error_in_archive = 1; /* not extracted:  warning */
                }
                switch (*G.answerbuf) {
                case 'r':
                case 'R':
                    do {
                        Info(slide, 0x81,
                             ((char *) slide, LoadFarString(NewNameQuery)));
                        fgets(G.filename, FILNAMSIZ, stdin);
                        /* usually get \n here:  better check for it */
                        fnlen = strlen(G.filename);
                        if (lastchar(G.filename, fnlen) == '\n')
                            G.filename[--fnlen] = '\0';
                    } while (fnlen == 0);
                    renamed = TRUE;
                    goto startover; /* sorry for a goto */
                case 'A':           /* dangerous option:  force caps */
                    G.overwrite_mode = OVERWRT_ALWAYS;
                    /* FALL THROUGH, extract */
                case 'y':
                case 'Y':
                    break;
                case 'N':
                    G.overwrite_mode = OVERWRT_NEVER;
                    /* FALL THROUGH, skip */
                case 'n':
                    /* skip file */
                    skip_entry = SKIP_Y_EXISTING;
                    break;
                case '\n':
                case '\r':
                    /* Improve echo of '\n' and/or '\r'
                       (sizeof(G.answerbuf) == 10 (see globals.h), so
                       there is enough space for the provided text...) */
                    strcpy(G.answerbuf, "{ENTER}");
                    /* fall through ... */
                default:
                    /* usually get \n here:  remove it for nice display
                       (fnlen can be re-used here, we are outside the
                       "enter new filename" loop) */
                    fnlen = strlen(G.answerbuf);
                    if (lastchar(G.answerbuf, fnlen) == '\n')
                        G.answerbuf[--fnlen] = '\0';
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(InvalidResponse),
                          G.answerbuf));
                    goto reprompt; /* yet another goto? */
                }                  /* end switch (*answerbuf) */
            }                      /* end if (query) */
            if (skip_entry != SKIP_NO) {
                continue;
            }
        } /* end if (extracting to disk) */

        G.disk_full = 0;
        if ((error = extract_or_test_member()) != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error; /* ...and keep going */
            if (G.disk_full > 1) {
                return error_in_archive; /* (unless disk full) */
            }
        }
        error = cover_add((cover_t *) G.cover, request,
                          G.cur_zipfile_bufstart + (G.inptr - G.inbuf));
        if (error < 0) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(NotEnoughMemCover)));
            return PK_MEM;
        }
        if (error != 0) {
            Info(slide, 0x401,
                 ((char *) slide, LoadFarString(OverlappedComponents)));
            return PK_BOMB;
        }
    } /* end for-loop (i:  files in current block) */

    return error_in_archive;

} /* end function extract_or_test_entrylist() */

/* wsize is used in extract_or_test_member() and UZbunzip2() */
#define wsize WSIZE /* wsize is a constant */

/***************************************/
/*  Function extract_or_test_member()  */
/***************************************/

static int extract_or_test_member() /* return PK-type error code */
{
    char *nul = "[empty] ", *txt = "[text]  ", *bin = "[binary]";
    register int b;
    int r, error = PK_COOL;

    /*---------------------------------------------------------------------------
        Initialize variables, buffers, etc.
      ---------------------------------------------------------------------------*/

    G.bits_left = 0;
    G.bitbuf = 0L; /* unreduce and unshrink only */
    G.zipeof = 0;
    G.newfile = TRUE;
    G.crc32val = CRCVAL_INITIAL;

    /* If file is a (POSIX-compatible) symbolic link and we are extracting
     * to disk, prepare to restore the link. */
    G.symlnk =
        (G.pInfo->symlink && !uO.tflag && !uO.cflag && (G.lrec.ucsize > 0));

    if (uO.tflag) {
        if (!uO.qflag)
            Info(slide, 0,
                 ((char *) slide, LoadFarString(ExtractMsg), "test",
                  FnFilter1(G.filename), "", ""));
    } else {
        if (uO.cflag) {
            G.outfile = stdout;
#define NEWLINE "\n"
        } else if (open_outfile())
            return PK_DISK;
    }

    /*---------------------------------------------------------------------------
        Unpack the file.
      ---------------------------------------------------------------------------*/

    defer_leftover_input(); /* so NEXTBYTE bounds check will work */
    switch (G.lrec.compression_method) {
    case STORED:
        if (!uO.tflag && QCOND2) {
            if (G.symlnk) /* can also be deflated, but rarer... */
                Info(slide, 0,
                     ((char *) slide, LoadFarString(ExtractMsg), "link",
                      FnFilter1(G.filename), "", ""));
            else
                Info(slide, 0,
                     ((char *) slide, LoadFarString(ExtractMsg), "extract",
                      FnFilter1(G.filename),
                      (uO.aflag !=
                       1 /* && G.pInfo->textfile==G.pInfo->textmode */)
                          ? ""
                          : (G.lrec.ucsize == 0L
                                 ? nul
                                 : (G.pInfo->textfile ? txt : bin)),
                      uO.cflag ? NEWLINE : ""));
        }
        G.outptr = redirSlide;
        G.outcnt = 0L;
        while ((b = NEXTBYTE) != EOF) {
            *G.outptr++ = (uch) b;
            if (++G.outcnt == wsize) {
                error = flush(redirSlide, G.outcnt, 0);
                G.outptr = redirSlide;
                G.outcnt = 0L;
                if (error != PK_COOL || G.disk_full)
                    break;
            }
        }
        if (G.outcnt) { /* flush final (partial) buffer */
            r = flush(redirSlide, G.outcnt, 0);
            if (error < r)
                error = r;
        }
        break;

    case SHRUNK:
        if (!uO.tflag && QCOND2) {
            Info(slide, 0,
                 ((char *) slide, LoadFarString(ExtractMsg),
                  LoadFarStringSmall(Unshrink), FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)
                      ? ""
                      : (G.pInfo->textfile ? txt : bin),
                  uO.cflag ? NEWLINE : ""));
        }
        if ((r = unshrink()) != PK_COOL) {
            if (r < PK_DISK) {
                if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipFile),
                          r == PK_MEM3 ? LoadFarString(NotEnoughMem)
                                       : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink),
                          FnFilter1(G.filename)));
                else
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                          r == PK_MEM3 ? LoadFarString(NotEnoughMem)
                                       : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink)));
            }
            error = r;
        }
        break;

    case IMPLODED:
        if (!uO.tflag && QCOND2) {
            Info(slide, 0,
                 ((char *) slide, LoadFarString(ExtractMsg), "explod",
                  FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)
                      ? ""
                      : (G.pInfo->textfile ? txt : bin),
                  uO.cflag ? NEWLINE : ""));
        }
        if ((r = explode()) != 0) {
            if (r == 5) { /* treat 5 specially */
                int warning = ((zusz_t) G.used_csize <= G.lrec.csize);

                if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(LengthMsg), "",
                          warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "", FmZofft(G.lrec.csize, NULL, "u"),
                          " [", FnFilter1(G.filename), "]"));
                else
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarString(LengthMsg), "\n",
                          warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "", FmZofft(G.lrec.csize, NULL, "u"),
                          "", "", "."));
                error = warning ? PK_WARN : PK_ERR;
            } else if (r < PK_DISK) {
                if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode), FnFilter1(G.filename)));
                else
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode)));
                error = ((r == 3) ? PK_MEM3 : PK_ERR);
            } else {
                error = r;
            }
        }
        break;

    case DEFLATED:
    case ENHDEFLATED:
        if (!uO.tflag && QCOND2) {
            Info(slide, 0,
                 ((char *) slide, LoadFarString(ExtractMsg), "inflat",
                  FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)
                      ? ""
                      : (G.pInfo->textfile ? txt : bin),
                  uO.cflag ? NEWLINE : ""));
        }
#define UZinflate inflate
        if ((r = UZinflate((G.lrec.compression_method == ENHDEFLATED))) != 0) {
            if (r < PK_DISK) {
                if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate), FnFilter1(G.filename)));
                else
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate)));
                error = ((r == 3) ? PK_MEM3 : PK_ERR);
            } else {
                error = r;
            }
        }
        break;

    case BZIPPED:
        if (!uO.tflag && QCOND2) {
            Info(slide, 0,
                 ((char *) slide, LoadFarString(ExtractMsg), "bunzipp",
                  FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)
                      ? ""
                      : (G.pInfo->textfile ? txt : bin),
                  uO.cflag ? NEWLINE : ""));
        }
        if ((r = UZbunzip2()) != 0) {
            if (r < PK_DISK) {
                if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip), FnFilter1(G.filename)));
                else
                    Info(slide, 0x401,
                         ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                          r == 3 ? LoadFarString(NotEnoughMem)
                                 : LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip)));
                error = ((r == 3) ? PK_MEM3 : PK_ERR);
            } else {
                error = r;
            }
        }
        break;

    default: /* should never get to this point */
        Info(slide, 0x401,
             ((char *) slide, LoadFarString(FileUnknownCompMethod),
              FnFilter1(G.filename)));
        /* close and delete file before return? */
        undefer_input();
        return PK_WARN;

    } /* end switch (compression method) */

    /*---------------------------------------------------------------------------
        Close the file and set its date and time (not necessarily in that
      order), and make sure the CRC checked out OK.  Logical-AND the CRC for
      64-bit machines (redundant on 32-bit machines).
      ---------------------------------------------------------------------------*/

    if (!uO.tflag && !uO.cflag) /* don't close NULL file or stdout */
        close_outfile();

    /* GRR: CONVERT close_outfile() TO NON-VOID:  CHECK FOR ERRORS! */

    if (G.disk_full) { /* set by flush() */
        if (G.disk_full > 1) {
            /* delete the incomplete file if we can */
            if (unlink(G.filename) != 0)
                Trace((stderr, "extract.c:  could not delete %s\n",
                       FnFilter1(G.filename)));
            error = PK_DISK;
        } else {
            error = PK_WARN;
        }
    }

    if (error >
        PK_WARN) { /* don't print redundant CRC error if error already */
        undefer_input();
        return error;
    }
    if (G.crc32val != G.lrec.crc32) {
        /* if quiet enough, we haven't output the filename yet:  do it */
        if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
            Info(slide, 0x401,
                 ((char *) slide, "%-22s ", FnFilter1(G.filename)));
        Info(slide, 0x401,
             ((char *) slide, LoadFarString(BadCRC), G.crc32val, G.lrec.crc32));
        if (G.pInfo->encrypted)
            Info(slide, 0x401, ((char *) slide, LoadFarString(MaybeBadPasswd)));
        error = PK_ERR;
    } else if (uO.tflag) {
        if (G.extra_field) {
            if ((r = TestExtraField(G.extra_field, G.lrec.extra_field_length)) >
                error)
                error = r;
        } else if (!uO.qflag)
            Info(slide, 0, ((char *) slide, " OK\n"));
    } else {
        if (QCOND2 && !error) /* GRR:  is stdout reset to text mode yet? */
            Info(slide, 0, ((char *) slide, "\n"));
    }

    undefer_input();

    if ((G.lrec.general_purpose_bit_flag & 8) != 0) {
        /* skip over data descriptor (harder than it sounds, due to signature
         * ambiguity)
         */
#define SIG 0x08074b50
#define LOW 0xffffffff
        uch buf[12];
        unsigned shy = 12 - readbuf((char *) buf, 12);
        ulg crc = shy ? 0 : makelong(buf);
        ulg clen = shy ? 0 : makelong(buf + 4);
        ulg ulen = shy ? 0 : makelong(buf + 8); /* or high clen if ZIP64 */
        if (crc == SIG &&                       /* if not SIG, no signature */
            (G.lrec.crc32 != SIG ||             /* if not SIG, have signature */
             (clen == SIG &&                    /* if not SIG, no signature */
              ((G.lrec.csize & LOW) != SIG ||   /* if not SIG, have signature */
               (ulen == SIG &&                  /* if not SIG, no signature */
                (G.zip64 ? G.lrec.csize >> 32 : G.lrec.ucsize) != SIG
                /* if not SIG, have signature */
                )))))
            /* skip four more bytes to account for signature */
            shy += 4 - readbuf((char *) buf, 4);
        if (G.zip64)
            shy += 8 - readbuf((char *) buf, 8); /* skip eight more for ZIP64 */
        if (shy)
            error = PK_ERR;
    }

    return error;

} /* end function extract_or_test_member() */

/*******************************/
/*  Function TestExtraField()  */
/*******************************/

static int TestExtraField(ef, ef_len)
uch *ef;
unsigned ef_len;
{
    ush ebID;
    unsigned ebLen;
    unsigned eb_cmpr_offs = 0;
    int r;

    /* we know the regular compressed file data tested out OK, or else we
     * wouldn't be here ==> print filename if any extra-field errors found
     */
    while (ef_len >= EB_HEADSIZE) {
        ebID = makeword(ef);
        ebLen = (unsigned) makeword(ef + EB_LEN);

        if (ebLen > (ef_len - EB_HEADSIZE)) {
            /* Discovered some extra field inconsistency! */
            if (uO.qflag)
                Info(slide, 1,
                     ((char *) slide, "%-22s ", FnFilter1(G.filename)));
            Info(slide, 1,
                 ((char *) slide, LoadFarString(InconsistEFlength), ebLen,
                  (ef_len - EB_HEADSIZE)));
            return PK_ERR;
        }

        switch (ebID) {
        case EF_OS2:
        case EF_ACL:
        case EF_MAC3:
        case EF_BEOS:
        case EF_ATHEOS:
            switch (ebID) {
            case EF_OS2:
            case EF_ACL:
                eb_cmpr_offs = EB_OS2_HLEN;
                break;
            case EF_MAC3:
                if (ebLen >= EB_MAC3_HLEN &&
                    (makeword(ef + (EB_HEADSIZE + EB_FLGS_OFFS)) &
                     EB_M3_FL_UNCMPR) &&
                    (makelong(ef + EB_HEADSIZE) == ebLen - EB_MAC3_HLEN))
                    eb_cmpr_offs = 0;
                else
                    eb_cmpr_offs = EB_MAC3_HLEN;
                break;
            case EF_BEOS:
            case EF_ATHEOS:
                if (ebLen >= EB_BEOS_HLEN &&
                    (*(ef + (EB_HEADSIZE + EB_FLGS_OFFS)) & EB_BE_FL_UNCMPR) &&
                    (makelong(ef + EB_HEADSIZE) == ebLen - EB_BEOS_HLEN))
                    eb_cmpr_offs = 0;
                else
                    eb_cmpr_offs = EB_BEOS_HLEN;
                break;
            }
            if ((r = test_compr_eb(ef, ebLen, eb_cmpr_offs, NULL)) != PK_OK) {
                if (uO.qflag)
                    Info(slide, 1,
                         ((char *) slide, "%-22s ", FnFilter1(G.filename)));
                switch (r) {
                case IZ_EF_TRUNC:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(TruncEAs),
                          ebLen - (eb_cmpr_offs + EB_CMPRHEADLEN), "\n"));
                    break;
                case PK_ERR:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(InvalidComprDataEAs)));
                    break;
                case PK_MEM3:
                case PK_MEM4:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(NotEnoughMemEAs)));
                    break;
                default:
                    if ((r & 0xff) != PK_ERR)
                        Info(slide, 1,
                             ((char *) slide, LoadFarString(UnknErrorEAs)));
                    else {
                        ush m = (ush) (r >> 8);
                        if (m == DEFLATED) /* GRR KLUDGE! */
                            Info(slide, 1,
                                 ((char *) slide, LoadFarString(BadCRC_EAs)));
                        else
                            Info(slide, 1,
                                 ((char *) slide,
                                  LoadFarString(UnknComprMethodEAs), m));
                    }
                    break;
                }
                return r;
            }
            break;

        case EF_NTSD:
            Trace((stderr, "ebID: %i / ebLen: %u\n", ebID, ebLen));
            r = ebLen < EB_NTSD_L_LEN
                    ? IZ_EF_TRUNC
                    : ((ef[EB_HEADSIZE + EB_NTSD_VERSION] > EB_NTSD_MAX_VER)
                           ? (PK_WARN | 0x4000)
                           : test_compr_eb(ef, ebLen, EB_NTSD_L_LEN, NULL));
            if (r != PK_OK) {
                if (uO.qflag)
                    Info(slide, 1,
                         ((char *) slide, "%-22s ", FnFilter1(G.filename)));
                switch (r) {
                case IZ_EF_TRUNC:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(TruncNTSD),
                          ebLen - (EB_NTSD_L_LEN + EB_CMPRHEADLEN), "\n"));
                    break;
                case PK_ERR:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(InvalidComprDataEAs)));
                    break;
                case PK_MEM3:
                case PK_MEM4:
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(NotEnoughMemEAs)));
                    break;
                case (PK_WARN | 0x4000):
                    Info(slide, 1,
                         ((char *) slide, LoadFarString(UnsuppNTSDVersEAs),
                          (int) ef[EB_HEADSIZE + EB_NTSD_VERSION]));
                    r = PK_WARN;
                    break;
                default:
                    if ((r & 0xff) != PK_ERR)
                        Info(slide, 1,
                             ((char *) slide, LoadFarString(UnknErrorEAs)));
                    else {
                        ush m = (ush) (r >> 8);
                        if (m == DEFLATED) /* GRR KLUDGE! */
                            Info(slide, 1,
                                 ((char *) slide, LoadFarString(BadCRC_EAs)));
                        else
                            Info(slide, 1,
                                 ((char *) slide,
                                  LoadFarString(UnknComprMethodEAs), m));
                    }
                    break;
                }
                return r;
            }
            break;
        case EF_PKVMS:
            if (ebLen < 4) {
                Info(slide, 1,
                     ((char *) slide, LoadFarString(TooSmallEBlength), ebLen,
                      4));
            } else if (makelong(ef + EB_HEADSIZE) !=
                       crc32(CRCVAL_INITIAL, ef + (EB_HEADSIZE + 4),
                             (extent) (ebLen - 4))) {
                Info(slide, 1, ((char *) slide, LoadFarString(BadCRC_EAs)));
            }
            break;
        case EF_PKW32:
        case EF_PKUNIX:
        case EF_ASIUNIX:
        case EF_IZVMS:
        case EF_IZUNIX:
        case EF_VMCMS:
        case EF_MVS:
        case EF_SPARK:
        case EF_TANDEM:
        case EF_THEOS:
        case EF_AV:
        default:
            break;
        }
        ef_len -= (ebLen + EB_HEADSIZE);
        ef += (ebLen + EB_HEADSIZE);
    }

    if (!uO.qflag)
        Info(slide, 0, ((char *) slide, " OK\n"));

    return PK_COOL;

} /* end function TestExtraField() */

/******************************/
/*  Function test_compr_eb()  */
/******************************/

static int test_compr_eb(uch *eb, unsigned eb_size, unsigned compr_offset,
                         int (*test_uc_ebdata)(uch *eb, unsigned eb_size,
                                               uch *eb_ucptr, ulg eb_ucsize))
{
    ulg eb_ucsize;
    uch *eb_ucptr;
    int r;
    ush eb_compr_method;

    if (compr_offset < 4) /* field is not compressed: */
        return PK_OK;     /* do nothing and signal OK */

    /* Return no/bad-data error status if any problem is found:
     *    1. eb_size is too small to hold the uncompressed size
     *       (eb_ucsize).  (Else extract eb_ucsize.)
     *    2. eb_ucsize is zero (invalid).  2014-12-04 SMS.
     *    3. eb_ucsize is positive, but eb_size is too small to hold
     *       the compressed data header.
     */
    if ((eb_size < (EB_UCSIZE_P + 4)) ||
        ((eb_ucsize = makelong(eb + (EB_HEADSIZE + EB_UCSIZE_P))) == 0L) ||
        ((eb_ucsize > 0L) && (eb_size <= (compr_offset + EB_CMPRHEADLEN))))
        return IZ_EF_TRUNC; /* no/bad compressed data! */

    /* 2015-02-10 Mancha(?), Michal Zalewski, Tomas Hoger, SMS.
     * For STORE method, compressed and uncompressed sizes must agree.
     * http://www.info-zip.org/phpBB3/viewtopic.php?f=7&t=450
     */
    eb_compr_method = makeword(eb + (EB_HEADSIZE + compr_offset));
    if ((eb_compr_method == STORED) &&
        (eb_size != compr_offset + EB_CMPRHEADLEN + eb_ucsize))
        return PK_ERR;

    if ((eb_ucptr = (uch *) malloc((extent) eb_ucsize)) == (uch *) NULL)
        return PK_MEM4;

    r = memextract(eb_ucptr, eb_ucsize, eb + (EB_HEADSIZE + compr_offset),
                   (ulg) (eb_size - compr_offset));

    if (r == PK_OK && test_uc_ebdata != NULL)
        r = (*test_uc_ebdata)(eb, eb_size, eb_ucptr, eb_ucsize);

    free(eb_ucptr);
    return r;

} /* end function test_compr_eb() */

/***************************/
/*  Function memextract()  */
/***************************/

int memextract(tgt, tgtsize, src, srcsize) /* extract compressed */
                                           /*  extra field block; */
uch *tgt;                                  /*  return PK-type error */
ulg tgtsize;                               /*  level */
const uch *src;
ulg srcsize;
{
    zoff_t old_csize = G.csize;
    uch *old_inptr = G.inptr;
    int old_incnt = G.incnt;
    int r, error = PK_OK;
    ush method;
    ulg extra_field_crc;

    method = makeword(src);
    extra_field_crc = makelong(src + 2);

    /* compressed extra field exists completely in memory at this location: */
    G.inptr = (uch *) src + (2 + 4); /* method and extra_field_crc */
    G.incnt = (int) (G.csize = (long) (srcsize - (2 + 4)));
    G.mem_mode = TRUE;
    G.outbufptr = tgt;
    G.outsize = tgtsize;

    switch (method) {
    case STORED:
        memcpy((char *) tgt, (char *) G.inptr, (extent) G.incnt);
        G.outcnt = (ulg) G.csize; /* for CRC calculation */
        break;
    case DEFLATED:
    case ENHDEFLATED:
        G.outcnt = 0L;
        if ((r = UZinflate((method == ENHDEFLATED))) != 0) {
            if (!uO.tflag)
                Info(slide, 0x401,
                     ((char *) slide, LoadFarStringSmall(ErrUnzipNoFile),
                      r == 3 ? LoadFarString(NotEnoughMem)
                             : LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Inflate)));
            error = (r == 3) ? PK_MEM3 : PK_ERR;
        }
        if (G.outcnt == 0L) /* inflate's final FLUSH sets outcnt */
            break;
        break;
    default:
        if (uO.tflag)
            error = PK_ERR | ((int) method << 8);
        else {
            Info(
                slide, 0x401,
                ((char *) slide, LoadFarString(UnsupportedExtraField), method));
            error = PK_ERR; /* GRR:  should be passed on up via SetEAs() */
        }
        break;
    }

    G.inptr = old_inptr;
    G.incnt = old_incnt;
    G.csize = old_csize;
    G.mem_mode = FALSE;

    if (!error) {
        register ulg crcval = crc32(CRCVAL_INITIAL, tgt, (extent) G.outcnt);

        if (crcval != extra_field_crc) {
            if (uO.tflag)
                error = PK_ERR | (DEFLATED << 8); /* kludge for now */
            else {
                Info(slide, 0x401,
                     ((char *) slide, LoadFarString(BadExtraFieldCRC), G.zipfn,
                      crcval, extra_field_crc));
                error = PK_ERR;
            }
        }
    }
    return error;

} /* end function memextract() */

/*************************/
/*  Function memflush()  */
/*************************/

int memflush(rawbuf, size) const uch *rawbuf;
ulg size;
{
    if (size > G.outsize)
        /* Here, PK_DISK is a bit off-topic, but in the sense of marking
           "overflow of output space", its use may be tolerated. */
        return PK_DISK; /* more data than output buffer can hold */

    memcpy((char *) G.outbufptr, (char *) rawbuf, (extent) size);
    G.outbufptr += (unsigned int) size;
    G.outsize -= size;
    G.outcnt += size;

    return 0;

} /* end function memflush() */

/***********************************/
/* Function set_deferred_symlink() */
/***********************************/

static void set_deferred_symlink(slnk_entry) slinkentry *slnk_entry;
{
    extent ucsize = slnk_entry->targetlen;
    char *linkfname = slnk_entry->fname;
    char *linktarget = (char *) malloc(ucsize + 1);

    if (!linktarget) {
        Info(slide, 0x201,
             ((char *) slide, LoadFarString(SymLnkWarnNoMem),
              FnFilter1(linkfname)));
        return;
    }
    linktarget[ucsize] = '\0';
    G.outfile = fopen(linkfname, "rb"); /* open link placeholder for reading */
    /* Check that the following conditions are all fulfilled:
     * a) the placeholder file exists,
     * b) the placeholder file contains exactly "ucsize" bytes
     *    (read the expected placeholder content length + 1 extra byte, this
     *    should return the expected content length),
     * c) the placeholder content matches the link target specification as
     *    stored in the symlink control structure.
     */
    if (!G.outfile || fread(linktarget, 1, ucsize + 1, G.outfile) != ucsize ||
        strcmp(slnk_entry->target, linktarget)) {
        Info(slide, 0x201,
             ((char *) slide, LoadFarString(SymLnkWarnInvalid),
              FnFilter1(linkfname)));
        free(linktarget);
        if (G.outfile)
            fclose(G.outfile);
        return;
    }
    fclose(G.outfile); /* close "data" file for good... */
    unlink(linkfname); /* ...and delete it */
    if (QCOND2)
        Info(slide, 0,
             ((char *) slide, LoadFarString(SymLnkFinish), FnFilter1(linkfname),
              FnFilter2(linktarget)));
    if (symlink(linktarget, linkfname)) /* create the real link */
        perror("symlink error");
    free(linktarget);
    set_symlnk_attribs(slnk_entry);
    return; /* can't set time on symlinks */

} /* end function set_deferred_symlink() */

/*************************/
/*  Function fnfilter()  */
/*************************/

char *fnfilter(raw, space, size) /* convert name to safely printable form */
    const char *raw;
uch *space;
extent size;
{
#ifndef NATIVE /* ASCII:  filter ANSI escape codes, etc. */
    const uch *r = (const uch *) raw;
    uch *s = space;
    uch *slim = NULL;
    uch *se = NULL;
    int have_overflow = FALSE;

    if (size > 0) {
        slim = space + size
#ifdef _MBCS
               - (MB_CUR_MAX - 1)
#endif
               - 4;
    }
    while (*r) {
        if (size > 0 && s >= slim && se == NULL) {
            se = s;
        }
        if (*r < 32) {
            /* ASCII control codes are escaped as "^{letter}". */
            if (se != NULL && (s > (space + (size - 4)))) {
                have_overflow = TRUE;
                break;
            }
            *s++ = '^', *s++ = (uch) (64 + *r++);
        } else {
#ifdef _MBCS
            unsigned i = CLEN(r);
            if (se != NULL && (s > (space + (size - i - 2)))) {
                have_overflow = TRUE;
                break;
            }
            for (; i > 0; i--)
                *s++ = *r++;
#else
            if (se != NULL && (s > (space + (size - 3)))) {
                have_overflow = TRUE;
                break;
            }
            *s++ = *r++;
#endif
        }
    }
    if (have_overflow) {
        strcpy((char *) se, "...");
    } else {
        *s = '\0';
    }

    return (char *) space;

#else /* NATIVE:  EBCDIC or whatever */
    return (char *) raw;
#endif

} /* end function fnfilter() */

/* must sort saved directories so can set perms from bottom up */

/************************/
/*  Function dircomp()  */
/************************/

static int dircomp(a, b) /* used by qsort(); swiped from Zip */
    const void *a,
    *b;
{
    /* order is significant:  this sorts in reverse order (deepest first) */
    return strcmp((*(direntry **) b)->fn, (*(direntry **) a)->fn);
    /* return namecmp((*(direntry **)b)->fn, (*(direntry **)a)->fn); */
}

/**************************/
/*  Function UZbunzip2()  */
/**************************/

int UZbunzip2()
/* decompress a bzipped entry using the libbz2 routines */
{
    int retval = 0; /* return code: 0 = "no error" */
    int err = BZ_OK;
    bz_stream bstrm;

    if (G.incnt <= 0 && G.csize <= 0L) {
        /* avoid an infinite loop */
        Trace((stderr, "UZbunzip2() got empty input\n"));
        return 2;
    }

    bstrm.next_out = (char *) redirSlide;
    bstrm.avail_out = wsize;

    bstrm.next_in = (char *) G.inptr;
    bstrm.avail_in = G.incnt;

    {
        /* local buffer for efficiency */
        /* $TODO Check for BZIP LIB version? */

        bstrm.bzalloc = NULL;
        bstrm.bzfree = NULL;
        bstrm.opaque = NULL;

        Trace((stderr, "initializing bzlib()\n"));
        err = BZ2_bzDecompressInit(&bstrm, 0, 0);

        if (err == BZ_MEM_ERROR)
            return 3;
        else if (err != BZ_OK)
            Trace((stderr, "oops!  (BZ2_bzDecompressInit() err = %d)\n", err));
    }

    while (G.csize > 0) {
        Trace((stderr, "first loop:  G.csize = %ld\n", G.csize));
        while (bstrm.avail_out > 0) {
            err = BZ2_bzDecompress(&bstrm);

            if (err == BZ_DATA_ERROR) {
                retval = 2;
                goto uzbunzip_cleanup_exit;
            } else if (err == BZ_MEM_ERROR) {
                retval = 3;
                goto uzbunzip_cleanup_exit;
            } else if (err != BZ_OK && err != BZ_STREAM_END)
                Trace((stderr, "oops!  (bzip(first loop) err = %d)\n", err));

            if (G.csize <= 0L) /* "END-of-entry-condition" ? */
                break;

            if (bstrm.avail_in == 0) {
                if (fillinbuf() == 0) {
                    /* no "END-condition" yet, but no more data */
                    retval = 2;
                    goto uzbunzip_cleanup_exit;
                }

                bstrm.next_in = (char *) G.inptr;
                bstrm.avail_in = G.incnt;
            }
            Trace((stderr, "     avail_in = %u\n", bstrm.avail_in));
        }
        /* flush slide[] */
        if ((retval = FLUSH(wsize - bstrm.avail_out)) != 0)
            goto uzbunzip_cleanup_exit;
        Trace((stderr, "inside loop:  flushing %ld bytes (ptr diff = %ld)\n",
               (long) (wsize - bstrm.avail_out),
               (long) (bstrm.next_out - (char *) redirSlide)));
        bstrm.next_out = (char *) redirSlide;
        bstrm.avail_out = wsize;
    }

    /* no more input, so loop until we have all output */
    Trace((stderr, "beginning final loop:  err = %d\n", err));
    while (err != BZ_STREAM_END) {
        err = BZ2_bzDecompress(&bstrm);
        if (err == BZ_DATA_ERROR) {
            retval = 2;
            goto uzbunzip_cleanup_exit;
        } else if (err == BZ_MEM_ERROR) {
            retval = 3;
            goto uzbunzip_cleanup_exit;
        } else if (err != BZ_OK && err != BZ_STREAM_END) {
            Trace((stderr, "oops!  (bzip(final loop) err = %d)\n", err));
            exit(PK_MEM3);
        }
        /* final flush of slide[] */
        if ((retval = FLUSH(wsize - bstrm.avail_out)) != 0)
            goto uzbunzip_cleanup_exit;
        Trace((stderr, "final loop:  flushing %ld bytes (ptr diff = %ld)\n",
               (long) (wsize - bstrm.avail_out),
               (long) (bstrm.next_out - (char *) redirSlide)));
        bstrm.next_out = (char *) redirSlide;
        bstrm.avail_out = wsize;
    }
#ifdef LARGE_FILE_SUPPORT
    Trace((stderr, "total in = %llu, total out = %llu\n",
           (zusz_t) (bstrm.total_in_lo32) + ((zusz_t) (bstrm.total_in_hi32))
               << 32,
           (zusz_t) (bstrm.total_out_lo32) + ((zusz_t) (bstrm.total_out_hi32))
               << 32));
#else
    Trace((stderr, "total in = %lu, total out = %lu\n", bstrm.total_in_lo32,
           bstrm.total_out_lo32));
#endif

    G.inptr = (uch *) bstrm.next_in;
    G.incnt -= G.inptr - G.inbuf; /* reset for other routines */

uzbunzip_cleanup_exit:
    err = BZ2_bzDecompressEnd(&bstrm);
    if (err != BZ_OK)
        Trace((stderr, "oops!  (BZ2_bzDecompressEnd() err = %d)\n", err));

    return retval;
} /* end function UZbunzip2() */
