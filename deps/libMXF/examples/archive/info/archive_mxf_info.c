/*
 * Copyright (C) 2011, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Jim Easterbrook
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mxf/mxf.h>
#include <mxf/mxf_app.h>
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_page_file.h>
#include <mxf/mxf_macros.h>



typedef struct
{
    wchar_t *companyName;
    wchar_t *productName;
    wchar_t *versionString;
    mxfTimestamp modificationDate;
} WriterIdentification;

typedef struct
{
    /* info */
    MXFList writerIdents;
    uint32_t pseFailureCount;
    uint32_t vtrErrorCount;
    uint32_t digiBetaDropoutCount;
    uint32_t timecodeBreakCount;

    int numVideoTracks;
    uint32_t videoWidth;
    uint32_t videoHeight;
    uint32_t componentDepth;
    mxfRational aspectRatio;
    int isProgressive;
    int numAudioTracks;
    mxfRational audioSamplingRate;
    uint32_t audioQuantizationBits;
    mxfRational editRate;
    int64_t duration;
    int64_t contentPackageLen;

    MXFList pseFailures;
    MXFList vtrErrors;
    MXFList digiBetaDropouts;
    MXFList timecodeBreaks;

    InfaxData sourceInfaxData;
    InfaxData ltoInfaxData;

    mxfUTF16Char *tempWString;

    /* mxf file data */
    const char *mxfFilename;
    MXFFile *mxfFile;
    MXFDataModel *dataModel;
    MXFPartition *headerPartition;
    MXFPartition *footerPartition;
    MXFHeaderMetadata *headerMetadata;
    MXFList *list;

    /* timecode reading data */
    int timecodeReadingInitialised;
    int64_t essenceDataStart;

    /* these are references and should not be free'd */
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *identSet;
    MXFMetadataSet *contentStorageSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *fileSourcePackageSet;
    MXFMetadataSet *tapeSourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *sourceClipSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFMetadataSet *timecodeComponentSet;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *multipleDescriptorSet;
    MXFMetadataSet *descriptorSet;
    MXFMetadataSet *cdciDescriptorSet;
    MXFMetadataSet *bwfDescriptorSet;
    MXFMetadataSet *tapeDescriptorSet;
    MXFMetadataSet *videoMaterialPackageTrackSet;
    MXFMetadataSet *videoSequenceSet;
} Reader;


static const mxfKey g_SysItemElementKey = MXF_SS1_ELEMENT_KEY(0x01, 0x00);



static void get_content_package_len(Reader *reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int64_t filePos;

    /* record file pos which is restored later */
    filePos = mxf_file_tell(reader->mxfFile);


    /* seek to the start of the essence */
    CHK_OFAIL(mxf_file_seek(reader->mxfFile, 0, SEEK_SET));
    while (1)
    {
        CHK_OFAIL(mxf_read_kl(reader->mxfFile, &key, &llen, &len));

        if (mxf_equals_key(&key, &g_SysItemElementKey))
        {
            break;
        }
        else if (mxf_is_footer_partition_pack(&key))
        {
            mxf_log_info("MXF file contains no essence data\n");

            /* restore file to the original position */
            mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);

            return;
        }
        else
        {
            CHK_OFAIL(mxf_skip(reader->mxfFile, len));
        }
    }

    /* read the first content package */
    reader->contentPackageLen = mxfKey_extlen + llen;
    while (1)
    {
        CHK_OFAIL(mxf_skip(reader->mxfFile, len));
        reader->contentPackageLen += len;

        CHK_OFAIL(mxf_read_kl(reader->mxfFile, &key, &llen, &len));
        if (mxf_equals_key(&key, &g_SysItemElementKey) || mxf_is_partition_pack(&key))
        {
            break;
        }
        reader->contentPackageLen += mxfKey_extlen + llen;
    }

    /* restore file to the original position */
    mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);

    return;


fail:
    mxf_log_warn("Failed to determine the content package length in the MXF file\n");
    reader->contentPackageLen = 0;

    /* restore file to the original position */
    mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);
}

static void report_actual_frame_count(Reader *reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int64_t essenceStartPos;
    int64_t filePos;
    struct stat statBuf;
    int64_t frameCount;


    /* record file pos which is restored later */
    filePos = mxf_file_tell(reader->mxfFile);


    /* seek to the start of the essence */
    CHK_OFAIL(mxf_file_seek(reader->mxfFile, 0, SEEK_SET));
    while (1)
    {
        CHK_OFAIL(mxf_read_kl(reader->mxfFile, &key, &llen, &len));

        if (mxf_equals_key(&key, &g_SysItemElementKey))
        {
            break;
        }
        else
        {
            CHK_OFAIL(mxf_skip(reader->mxfFile, len));
        }
    }
    CHK_OFAIL(mxf_file_seek(reader->mxfFile, -(mxfKey_extlen + llen), SEEK_CUR));

    /* calculate the number of frames present using the file size */
    essenceStartPos = mxf_file_tell(reader->mxfFile);
    CHK_OFAIL(stat(reader->mxfFilename, &statBuf) == 0);
    if (statBuf.st_size - essenceStartPos > 0)
    {
        frameCount = (statBuf.st_size - essenceStartPos) / reader->contentPackageLen;

        if (reader->headerPartition->footerPartition == 0 ||
            essenceStartPos + frameCount * reader->contentPackageLen < (int64_t)reader->headerPartition->footerPartition)
        {
            mxf_log_warn("%" PRId64 " complete frames are present in the MXF file\n", frameCount);
        }
        else
        {
            mxf_log_warn("All frames are present in the MXF file\n");
        }
    }
    else
    {
        mxf_log_warn("0 complete frames are present in the MXF file\n");
    }


    /* restore file to the original position */
    mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);

    return;


fail:
    mxf_log_warn("Failed to determine the number of frame actually present in the MXF file\n");

    /* restore file to the original position */
    mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);
}

static int check_can_read_rip(Reader *reader)
{
    MXFRIP rip;
    int result;
    int64_t filePos;

    memset(&rip, 0, sizeof(rip));

    /* record file pos which is restored later */
    filePos = mxf_file_tell(reader->mxfFile);

    /* try read the RIP */
    result = mxf_read_rip(reader->mxfFile, &rip);
    if (result)
    {
        mxf_clear_rip(&rip);
    }

    /* restore file to the original position */
    mxf_file_seek(reader->mxfFile, filePos, SEEK_SET);

    return result;
}

static void convert_12m_to_timecode(uint8_t *t12m, ArchiveTimecode *t)
{
    t->frame = ((t12m[0] >> 4) & 0x03) * 10 + (t12m[0] & 0xf);
    t->sec   = ((t12m[1] >> 4) & 0x07) * 10 + (t12m[1] & 0xf);
    t->min   = ((t12m[2] >> 4) & 0x07) * 10 + (t12m[2] & 0xf);
    t->hour  = ((t12m[3] >> 4) & 0x03) * 10 + (t12m[3] & 0xf);
}

static int initialise_timecode_reader(Reader *reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    /* position the mxf file at the start of the essence data */
    CHK_ORET(mxf_file_seek(reader->mxfFile, 0, SEEK_SET));
    while (1)
    {
        CHK_ORET(mxf_read_kl(reader->mxfFile, &key, &llen, &len));

        if (mxf_equals_key(&key, &g_SysItemElementKey))
        {
            break;
        }
        else
        {
            CHK_ORET(mxf_skip(reader->mxfFile, len));
        }
    }
    CHK_ORET(mxf_file_seek(reader->mxfFile, -(llen + mxfKey_extlen), SEEK_CUR));

    /* set the position */
    CHK_ORET((reader->essenceDataStart = mxf_file_tell(reader->mxfFile)) >= 0);

    reader->timecodeReadingInitialised = 1;

    return 1;
}

static int read_timecode_at_position(Reader *reader, int64_t position,
                                     ArchiveTimecode *vitc, ArchiveTimecode *ltc)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint8_t t12m[8];
    uint16_t localTag;
    uint16_t localItemLen;
    int haveTimecodeItem = 0;

    CHK_ORET(reader->timecodeReadingInitialised);

    /* position at frame */
    CHK_ORET(mxf_file_seek(reader->mxfFile,
                           reader->essenceDataStart + position * reader->contentPackageLen,
                           SEEK_SET));

    /* read and check the kl */
    CHK_ORET(mxf_read_kl(reader->mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_SysItemElementKey));

    /* read the timecode local item */
    while (len > 0)
    {
        CHK_ORET(mxf_read_uint16(reader->mxfFile, &localTag));
        CHK_ORET(mxf_read_uint16(reader->mxfFile, &localItemLen));
        len -= 4;

        if (localTag == 0x0102)
        {
            CHK_ORET(localItemLen == 24);

            /* skip the array header */
            CHK_ORET(mxf_skip(reader->mxfFile, 8));

            /* read the timecode */
            CHK_ORET(mxf_file_read(reader->mxfFile, t12m, 8) == 8);
            convert_12m_to_timecode(t12m, vitc);
            CHK_ORET(mxf_file_read(reader->mxfFile, t12m, 8) == 8);
            convert_12m_to_timecode(t12m, ltc);

            len -= localItemLen;

            haveTimecodeItem = 1;
            break;
        }
        else
        {
            CHK_ORET(mxf_skip(reader->mxfFile, localItemLen));

            len -= localItemLen;
        }
    }
    CHK_ORET(haveTimecodeItem);

    if (len > 0)
    {
        CHK_ORET(mxf_skip(reader->mxfFile, len));
    }

    return 1;
}

static int read_time_string_at_position(Reader *reader, int64_t position,
                                        char *vitcStr, size_t vitcStrSize,
                                        char *ltcStr, size_t ltcStrSize)
{
    ArchiveTimecode vitc;
    ArchiveTimecode ltc;

    CHK_ORET(read_timecode_at_position(reader, position, &vitc, &ltc));

    mxf_snprintf(vitcStr, vitcStrSize, "%02d:%02d:%02d:%02d", vitc.hour, vitc.min, vitc.sec, vitc.frame);
    mxf_snprintf(ltcStr,  ltcStrSize,  "%02d:%02d:%02d:%02d", ltc.hour, ltc.min, ltc.sec, ltc.frame);

    return 1;
}

static int convert_string(mxfUTF16Char *input, char *output, size_t size)
{
    CHK_ORET(wcstombs(output, input, size) != (size_t)(-1));

    return 1;
}

static void free_writer_ident_in_list(void *data)
{
    WriterIdentification *ident;

    if (data == NULL)
    {
        return;
    }

    ident = (WriterIdentification*)data;
    SAFE_FREE(ident->companyName);
    SAFE_FREE(ident->productName);
    SAFE_FREE(ident->versionString);

    free(data);
}

static int create_pse_failure(Reader *reader, PSEFailure **failure)
{
    PSEFailure *newFailure = NULL;

    CHK_MALLOC_ORET(newFailure, PSEFailure);
    memset(newFailure, 0, sizeof(PSEFailure));
    CHK_OFAIL(mxf_append_list_element(&reader->pseFailures, newFailure));

    *failure = newFailure;
    return 1;

fail:
    SAFE_FREE(newFailure);
    return 0;
}

static int create_vtr_error(Reader *reader, VTRErrorAtPos **error)
{
    VTRErrorAtPos *newError = NULL;

    CHK_MALLOC_ORET(newError, VTRErrorAtPos);
    memset(newError, 0, sizeof(VTRErrorAtPos));
    CHK_OFAIL(mxf_append_list_element(&reader->vtrErrors, newError));

    *error = newError;
    return 1;

fail:
    SAFE_FREE(newError);
    return 0;
}

static int create_digibeta_dropout(Reader *reader, DigiBetaDropout **digiBetaDropout)
{
    DigiBetaDropout *newDigiBetaDropout = NULL;

    CHK_MALLOC_ORET(newDigiBetaDropout, DigiBetaDropout);
    memset(newDigiBetaDropout, 0, sizeof(DigiBetaDropout));
    CHK_OFAIL(mxf_append_list_element(&reader->digiBetaDropouts, newDigiBetaDropout));

    *digiBetaDropout = newDigiBetaDropout;
    return 1;

fail:
    SAFE_FREE(newDigiBetaDropout);
    return 0;
}

static int create_timecode_break(Reader *reader, TimecodeBreak **timecodeBreak)
{
    TimecodeBreak *newTimecodeBreak = NULL;

    CHK_MALLOC_ORET(newTimecodeBreak, TimecodeBreak);
    memset(newTimecodeBreak, 0, sizeof(TimecodeBreak));
    CHK_OFAIL(mxf_append_list_element(&reader->timecodeBreaks, newTimecodeBreak));

    *timecodeBreak = newTimecodeBreak;
    return 1;

fail:
    SAFE_FREE(newTimecodeBreak);
    return 0;
}

static int create_writer_ident(Reader *reader, WriterIdentification **ident)
{
    WriterIdentification *newIdent = NULL;

    CHK_MALLOC_ORET(newIdent, WriterIdentification);
    memset(newIdent, 0, sizeof(WriterIdentification));
    CHK_OFAIL(mxf_append_list_element(&reader->writerIdents, newIdent));

    *ident = newIdent;
    return 1;

fail:
    SAFE_FREE(newIdent);
    return 0;
}

static int create_reader(Reader **reader)
{
    Reader *newReader = NULL;

    CHK_MALLOC_ORET(newReader, Reader);
    memset(newReader, 0, sizeof(Reader));
    mxf_initialise_list(&newReader->writerIdents, free_writer_ident_in_list);
    mxf_initialise_list(&newReader->pseFailures, free);
    mxf_initialise_list(&newReader->vtrErrors, free);
    mxf_initialise_list(&newReader->digiBetaDropouts, free);
    mxf_initialise_list(&newReader->timecodeBreaks, free);

    *reader = newReader;
    return 1;
}

static void free_reader(Reader **reader)
{
    if (*reader == NULL)
    {
        return;
    }

    SAFE_FREE((*reader)->tempWString);

    mxf_clear_list(&(*reader)->writerIdents);
    mxf_clear_list(&(*reader)->pseFailures);
    mxf_clear_list(&(*reader)->vtrErrors);
    mxf_clear_list(&(*reader)->digiBetaDropouts);
    mxf_clear_list(&(*reader)->timecodeBreaks);

    mxf_free_list(&(*reader)->list);
    mxf_free_partition(&(*reader)->headerPartition);
    mxf_free_partition(&(*reader)->footerPartition);
    mxf_free_header_metadata(&(*reader)->headerMetadata);
    mxf_free_data_model(&(*reader)->dataModel);

    mxf_file_close(&(*reader)->mxfFile);

    SAFE_FREE(*reader);
}


#define GET_STRING_ITEM(name, cName) \
    if (mxf_have_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_ORET(mxf_uu_get_utf16string_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &reader->tempWString)); \
        CHK_ORET(convert_string(reader->tempWString, infaxData->cName, sizeof(infaxData->cName))); \
        SAFE_FREE(reader->tempWString); \
    }

#define GET_DATE_ITEM(name, cName) \
    if (mxf_have_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_ORET(mxf_get_timestamp_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
        infaxData->cName.hour = 0; \
        infaxData->cName.min = 0; \
        infaxData->cName.sec = 0; \
        infaxData->cName.qmsec = 0; \
    }

#define GET_INT64_ITEM(name, cName) \
    if (mxf_have_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_ORET(mxf_get_int64_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
    }

#define GET_UINT32_ITEM(name, cName) \
    if (mxf_have_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_ORET(mxf_get_uint32_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
    }

static int get_infax_data(Reader *reader, InfaxData *infaxData)
{
    GET_STRING_ITEM(APP_Format, format);
    GET_STRING_ITEM(APP_ProgrammeTitle, progTitle);
    GET_STRING_ITEM(APP_EpisodeTitle, epTitle);
    GET_DATE_ITEM(APP_TransmissionDate, txDate);
    GET_STRING_ITEM(APP_MagazinePrefix, magPrefix);
    GET_STRING_ITEM(APP_ProgrammeNumber, progNo);
    GET_STRING_ITEM(APP_ProductionCode, prodCode);
    GET_STRING_ITEM(APP_SpoolStatus, spoolStatus);
    GET_DATE_ITEM(APP_StockDate, stockDate);
    GET_STRING_ITEM(APP_SpoolDescriptor, spoolDesc);
    GET_STRING_ITEM(APP_Memo, memo);
    GET_INT64_ITEM(APP_Duration, duration);
    GET_STRING_ITEM(APP_SpoolNumber, spoolNo);
    GET_STRING_ITEM(APP_AccessionNumber, accNo);
    GET_STRING_ITEM(APP_CatalogueDetail, catDetail);
    GET_UINT32_ITEM(APP_ItemNumber, itemNo);

    return 1;
}

static int get_info(Reader *reader, int checkIssues, int showPSEFailures, int showVTRErrors, int showDigiBetaDropouts,
                    int showTimecodeBreaks)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFListIterator iter;
    WriterIdentification *writerIdent;
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    mxfUL dataDef;
    int64_t duration;
    uint32_t sequenceComponentCount;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    PSEFailure *pseFailure;
    VTRErrorAtPos *vtrError;
    DigiBetaDropout *digiBetaDropout;
    TimecodeBreak *timecodeBreak;
    int knownDMTrack;
    uint64_t headerByteCount;
    MXFListIterator setsIter;
    int fileIsComplete;
    mxfRational editRate;
    uint8_t frameLayout;


    /* read the header partition pack */
    if (!mxf_read_header_pp_kl(reader->mxfFile, &key, &llen, &len))
    {
        mxf_log_error("Could not find header partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    if (!mxf_partition_is_complete(&key))
    {
        mxf_log_warn("Header partition is incomplete" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
    }
    CHK_ORET(mxf_read_partition(reader->mxfFile, &key, len, &reader->headerPartition));


    /* check the operational pattern is OP 1A */
    if (!mxf_is_op_1a(&reader->headerPartition->operationalPattern))
    {
        mxf_log_error("Input file is not OP 1A" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }


    /* check the file is complete */
    fileIsComplete = check_can_read_rip(reader);
    if (!fileIsComplete)
    {
        mxf_log_warn("Failed to read the MXF Random Index Pack - file is incomplete\n");
    }


    /* get the content package length */
    get_content_package_len(reader);


    /* report number of frames actually present if the file is incomplete */
    if (!fileIsComplete)
    {
        /* don't bother if were unable to read the content package length */
        if (reader->contentPackageLen <= 0)
        {
            mxf_log_warn("Cannot check the actual frame count because failed to read the first content package\n");
            mxf_log_warn("Assuming 0 complete frames are present in the MXF file\n");
        }
        else
        {
            report_actual_frame_count(reader);
        }
    }


    /* read the header metadata in the footer is showing PSE failures, etc */
    if (checkIssues || showPSEFailures || showVTRErrors || showDigiBetaDropouts || showTimecodeBreaks)
    {
        if (fileIsComplete)
        {
            CHK_ORET(mxf_file_seek(reader->mxfFile, reader->headerPartition->footerPartition, SEEK_SET));
            CHK_ORET(mxf_read_next_nonfiller_kl(reader->mxfFile, &key, &llen, &len));
            CHK_ORET(mxf_is_footer_partition_pack(&key));
            CHK_ORET(mxf_read_partition(reader->mxfFile, &key, len, &reader->footerPartition));
            headerByteCount = reader->footerPartition->headerByteCount;

            if (!mxf_partition_is_closed(&key))
            {
                mxf_log_warn("Footer partition is open" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
            if (!mxf_partition_is_complete(&key))
            {
                mxf_log_warn("Footer partition is incomplete" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
        }
        else
        {
            mxf_log_warn("Cannot show PSE failures or VTR errors from an incomplete MXF file\n");
            headerByteCount = reader->headerPartition->headerByteCount;
        }
    }
    else
    {
        headerByteCount = reader->headerPartition->headerByteCount;
    }


    /* load the data model  */
    CHK_ORET(mxf_load_data_model(&reader->dataModel));
    CHK_ORET(mxf_app_load_extensions(reader->dataModel));
    CHK_ORET(mxf_finalise_data_model(reader->dataModel));


    /* create and read the header metadata */
    CHK_ORET(mxf_create_header_metadata(&reader->headerMetadata, reader->dataModel));
    CHK_ORET(mxf_read_next_nonfiller_kl(reader->mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_is_header_metadata(&key));
    CHK_ORET(mxf_read_header_metadata(reader->mxfFile, reader->headerMetadata, headerByteCount, &key, llen, len));


    /* PSE failure, VTR error and digibeta dropout counts in the Preface */
    CHK_ORET(mxf_find_singular_set_by_key(reader->headerMetadata, &MXF_SET_K(Preface), &reader->prefaceSet));
    reader->vtrErrorCount = 0;
    if (mxf_have_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_VTRErrorCount)))
    {
        CHK_ORET(mxf_get_uint32_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_VTRErrorCount), &reader->vtrErrorCount));
    }
    reader->pseFailureCount = 0;
    if (mxf_have_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_PSEFailureCount)))
    {
        CHK_ORET(mxf_get_uint32_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_PSEFailureCount), &reader->pseFailureCount));
    }
    reader->digiBetaDropoutCount = 0;
    if (mxf_have_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_DigiBetaDropoutCount)))
    {
        CHK_ORET(mxf_get_uint32_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_DigiBetaDropoutCount), &reader->digiBetaDropoutCount));
    }
    reader->timecodeBreakCount = 0;
    if (mxf_have_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_TimecodeBreakCount)))
    {
        CHK_ORET(mxf_get_uint32_item(reader->prefaceSet, &MXF_ITEM_K(Preface, APP_TimecodeBreakCount), &reader->timecodeBreakCount));
    }



    /* Writer identification info */

    CHK_ORET(mxf_find_set_by_key(reader->headerMetadata, &MXF_SET_K(Identification), &reader->list));
    mxf_initialise_list_iter(&iter, reader->list);
    while (mxf_next_list_iter_element(&iter))
    {
        reader->identSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
        CHK_ORET(create_writer_ident(reader, &writerIdent));

        CHK_ORET(mxf_uu_get_utf16string_item(reader->identSet, &MXF_ITEM_K(Identification, CompanyName), &writerIdent->companyName));
        CHK_ORET(mxf_uu_get_utf16string_item(reader->identSet, &MXF_ITEM_K(Identification, ProductName), &writerIdent->productName));
        CHK_ORET(mxf_uu_get_utf16string_item(reader->identSet, &MXF_ITEM_K(Identification, VersionString), &writerIdent->versionString));
        CHK_ORET(mxf_get_timestamp_item(reader->identSet, &MXF_ITEM_K(Identification, ModificationDate), &writerIdent->modificationDate));
    }
    mxf_free_list(&reader->list);


    /* process the MaterialPackage for edit rate and duration */

    CHK_ORET(mxf_find_singular_set_by_key(reader->headerMetadata, &MXF_SET_K(MaterialPackage), &reader->materialPackageSet));
    CHK_ORET(mxf_uu_get_package_tracks(reader->materialPackageSet, &arrayIter));
    while (mxf_uu_next_track(reader->headerMetadata, &arrayIter, &reader->materialPackageTrackSet))
    {
        CHK_ORET(mxf_uu_get_track_datadef(reader->materialPackageTrackSet, &dataDef));

        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }

        /* edit rate and duration */
        CHK_ORET(mxf_get_rational_item(reader->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));
        CHK_ORET(mxf_uu_get_track_duration(reader->materialPackageTrackSet, &duration));

        if (reader->editRate.numerator == 0)
        {
            reader->editRate = editRate;
            reader->duration = duration;
        }
        else if (reader->numVideoTracks == 0 && mxf_is_picture(&dataDef))
        {
            /* swap duration and edit rate to that the video edit rate is used as the package edit rate */
            int64_t tmpDuration;
            mxfRational tmpEditRate;

            tmpEditRate = reader->editRate;
            reader->editRate = editRate;
            editRate = tmpEditRate;
            tmpDuration = reader->duration;
            reader->duration = duration;
            duration = tmpDuration;
        }

        /* package duration is the minimum track duration */
        if (editRate.numerator   != reader->editRate.numerator ||
            editRate.denominator != reader->editRate.denominator)
        {
            duration = duration * reader->editRate.numerator * editRate.denominator /
                            (reader->editRate.denominator * editRate.numerator);
        }
        if (duration < reader->duration)
        {
            reader->duration = duration;
        }

        if (mxf_is_picture(&dataDef))
        {
            reader->numVideoTracks++;
        }
        else
        {
            reader->numAudioTracks++;
        }
    }

    /* process file SourcePackage CDCIDescriptor and WAVEAudioDescriptor */

    CHK_ORET(mxf_uu_get_top_file_package(reader->headerMetadata, &reader->fileSourcePackageSet));

    CHK_ORET(mxf_get_strongref_item(reader->fileSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &reader->descriptorSet));
    CHK_ORET(mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->descriptorSet->key, &MXF_SET_K(MultipleDescriptor)));
    mxf_initialise_array_item_iterator(reader->descriptorSet, &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), &arrayIter);
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
    {
        CHK_ORET(mxf_get_strongref(reader->headerMetadata, arrayElement, &reader->descriptorSet));
        if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->descriptorSet->key, &MXF_SET_K(CDCIEssenceDescriptor)))
        {
            CHK_ORET(mxf_get_rational_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &reader->aspectRatio));
            if (mxf_have_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth)))
            {
                CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &reader->videoWidth));
            }
            else
            {
                CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &reader->videoWidth));
            }
            if (mxf_have_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight)))
            {
                CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &reader->videoHeight));
            }
            else
            {
                CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &reader->videoHeight));
            }
            CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), &reader->componentDepth));
            if (mxf_have_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout)))
            {
                CHK_ORET(mxf_get_uint8_item(reader->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &frameLayout));
                if (frameLayout == MXF_FULL_FRAME || frameLayout == MXF_SEGMENTED_FRAME)
                {
                    reader->isProgressive = 1;
                }
            }
        }
        if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->descriptorSet->key, &MXF_SET_K(GenericSoundEssenceDescriptor)))
        {
            CHK_ORET(mxf_get_rational_item(reader->descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &reader->audioSamplingRate));
            CHK_ORET(mxf_get_uint32_item(reader->descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), &reader->audioQuantizationBits));
        }
    }

    /* process file SourcePackage tracks */

    CHK_ORET(mxf_uu_get_package_tracks(reader->fileSourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(reader->headerMetadata, &arrayIter, &reader->sourcePackageTrackSet))
    {
        CHK_ORET(mxf_uu_get_track_datadef(reader->sourcePackageTrackSet, &dataDef));

        /* PSE analysis, VTR error, digibeta dropout, timecode break and BBC DMS */

        if (mxf_is_descriptive_metadata(&dataDef))
        {
            knownDMTrack = 0;
            CHK_ORET(mxf_get_strongref_item(reader->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &reader->sequenceSet));
            if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                /* first check required property exists to workaround bug in writer */
                if (!mxf_have_item(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    if (checkIssues)
                    {
                        return 0;
                    }
                    continue;
                }

                CHK_ORET(mxf_get_array_item_count(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &sequenceComponentCount));
                if (sequenceComponentCount > 0)
                {
                    CHK_ORET(mxf_get_array_item_element(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                    CHK_ORET(mxf_get_strongref(reader->headerMetadata, arrayElement, &reader->dmSet));
                    if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmSet->key, &MXF_SET_K(DMSegment)))
                    {
                        if (mxf_have_item(reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                        {
                            CHK_ORET(mxf_get_strongref_item(reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                            if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmFrameworkSet->key, &MXF_SET_K(APP_PSEAnalysisFramework)))
                            {
                                /* PSE failures track */
                                knownDMTrack = 1;
                                if (showPSEFailures)
                                {
                                    mxf_initialise_sets_iter(reader->headerMetadata, &setsIter);

                                    mxf_initialise_array_item_iterator(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                                    while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                                    {
                                        CHK_ORET(create_pse_failure(reader, &pseFailure));
                                        CHK_ORET(mxf_get_strongref_s(reader->headerMetadata, &setsIter, arrayElement, &reader->dmSet));
                                        CHK_ORET(mxf_get_position_item(reader->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &pseFailure->position));
                                        CHK_ORET(mxf_get_strongref_item_s(&setsIter, reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                                        CHK_ORET(mxf_get_int16_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_RedFlash), &pseFailure->redFlash));
                                        CHK_ORET(mxf_get_int16_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_SpatialPattern), &pseFailure->spatialPattern));
                                        CHK_ORET(mxf_get_int16_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_LuminanceFlash), &pseFailure->luminanceFlash));
                                        CHK_ORET(mxf_get_boolean_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_ExtendedFailure), &pseFailure->extendedFailure));
                                    }
                                }
                            }
                            else if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmFrameworkSet->key, &MXF_SET_K(APP_VTRReplayErrorFramework)))
                            {
                                /* VTR error track */
                                knownDMTrack = 1;
                                if (showVTRErrors)
                                {
                                    mxf_initialise_sets_iter(reader->headerMetadata, &setsIter);

                                    mxf_initialise_array_item_iterator(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                                    while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                                    {
                                        CHK_ORET(create_vtr_error(reader, &vtrError));
                                        CHK_ORET(mxf_get_strongref_s(reader->headerMetadata, &setsIter, arrayElement, &reader->dmSet));
                                        CHK_ORET(mxf_get_position_item(reader->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &vtrError->position));
                                        CHK_ORET(mxf_get_strongref_item_s(&setsIter, reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                                        CHK_ORET(mxf_get_uint8_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_VTRReplayErrorFramework, APP_VTRErrorCode), &vtrError->errorCode));
                                    }
                                }
                            }
                            else if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmFrameworkSet->key, &MXF_SET_K(APP_DigiBetaDropoutFramework)))
                            {
                                /* digibeta dropout track */
                                knownDMTrack = 1;
                                if (showDigiBetaDropouts)
                                {
                                    mxf_initialise_sets_iter(reader->headerMetadata, &setsIter);

                                    mxf_initialise_array_item_iterator(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                                    while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                                    {
                                        CHK_ORET(create_digibeta_dropout(reader, &digiBetaDropout));
                                        CHK_ORET(mxf_get_strongref_s(reader->headerMetadata, &setsIter, arrayElement, &reader->dmSet));
                                        CHK_ORET(mxf_get_position_item(reader->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &digiBetaDropout->position));
                                        CHK_ORET(mxf_get_strongref_item_s(&setsIter, reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                                        CHK_ORET(mxf_get_int32_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_DigiBetaDropoutFramework, APP_Strength), &digiBetaDropout->strength));
                                    }
                                }
                            }
                            else if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmFrameworkSet->key, &MXF_SET_K(APP_TimecodeBreakFramework)))
                            {
                                /* timecode break dropout track */
                                knownDMTrack = 1;
                                if (showTimecodeBreaks)
                                {
                                    mxf_initialise_sets_iter(reader->headerMetadata, &setsIter);

                                    mxf_initialise_array_item_iterator(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                                    while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                                    {
                                        CHK_ORET(create_timecode_break(reader, &timecodeBreak));
                                        CHK_ORET(mxf_get_strongref_s(reader->headerMetadata, &setsIter, arrayElement, &reader->dmSet));
                                        CHK_ORET(mxf_get_position_item(reader->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &timecodeBreak->position));
                                        CHK_ORET(mxf_get_strongref_item_s(&setsIter, reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                                        CHK_ORET(mxf_get_uint16_item(reader->dmFrameworkSet, &MXF_ITEM_K(APP_TimecodeBreakFramework, APP_TimecodeType), &timecodeBreak->timecodeType));
                                    }
                                }
                            }
                            else if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmFrameworkSet->key, &MXF_SET_K(APP_InfaxFramework)))
                            {
                                /* Infax data track */
                                knownDMTrack = 1;
                                CHK_ORET(mxf_get_strongref_item(reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));
                                CHK_ORET(get_infax_data(reader, &reader->ltoInfaxData));
                            }
                        }
                    }
                }
            }
            if (!knownDMTrack)
            {
                mxf_log_warn("Unknown descriptive metadata track found in the file source package - info tool update required\n");
            }
        }
    }


    /* tape SourcePackage */

    CHK_ORET(mxf_find_set_by_key(reader->headerMetadata, &MXF_SET_K(SourcePackage), &reader->list));
    mxf_initialise_list_iter(&iter, reader->list);
    while (mxf_next_list_iter_element(&iter))
    {
        reader->sourcePackageSet = (MXFMetadataSet*)(mxf_get_iter_element(&iter));

        /* it is the tape SourcePackage if it has a TapeDescriptor */
        CHK_ORET(mxf_get_strongref_item(reader->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &reader->descriptorSet));
        if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->descriptorSet->key, &MXF_SET_K(TapeDescriptor)))
        {
            /* go through the tracks and find the DMS track */
            CHK_ORET(mxf_uu_get_package_tracks(reader->sourcePackageSet, &arrayIter));
            while (mxf_uu_next_track(reader->headerMetadata, &arrayIter, &reader->sourcePackageTrackSet))
            {
                CHK_ORET(mxf_uu_get_track_datadef(reader->sourcePackageTrackSet, &dataDef));

                if (mxf_is_descriptive_metadata(&dataDef))
                {
                    /* get to the single DMSegment */
                    CHK_ORET(mxf_get_strongref_item(reader->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &reader->sequenceSet));
                    if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->sequenceSet->key, &MXF_SET_K(Sequence)))
                    {
                        CHK_ORET(mxf_get_array_item_count(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &sequenceComponentCount));
                        if (sequenceComponentCount != 1)
                        {
                            /* Sequence of length 1 is expected for the DMS track */
                            continue;
                        }

                        CHK_ORET(mxf_get_array_item_element(reader->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                        CHK_ORET(mxf_get_strongref(reader->headerMetadata, arrayElement, &reader->dmSet));
                    }
                    else
                    {
                        reader->dmSet = reader->sequenceSet;
                    }

                    /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
                    if (mxf_is_subclass_of(reader->headerMetadata->dataModel, &reader->dmSet->key, &MXF_SET_K(DMSegment)))
                    {
                        if (mxf_have_item(reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                        {
                            CHK_ORET(mxf_get_strongref_item(reader->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &reader->dmFrameworkSet));

                            CHK_ORET(get_infax_data(reader, &reader->sourceInfaxData));
                            break;
                        }
                    }
                }
            }

            break;
        }
    }

    mxf_free_list(&reader->list);


    return 1;
}

static void write_vtr_errors(Reader *reader, int noSourceTimecode)
{
    MXFListIterator iter;
    VTRErrorAtPos *vtrError;
    uint64_t count;
    char vitcStr[12];
    char ltcStr[12];

    printf("\nVTR error results:\n");
    printf("    %d errors detected\n", reader->vtrErrorCount);
    if (mxf_get_list_length(&reader->vtrErrors) > 0)
    {
        if (noSourceTimecode)
        {
            printf("    %10s:%10s %10s    %s\n",
                   "num", "frame", "code", "description");
        }
        else
        {
            printf("    %10s:%10s%16s%16s %10s    %s\n",
                   "num", "frame", "vitc", "ltc", "code", "description");
        }

        count = 0;
        mxf_initialise_list_iter(&iter, &reader->vtrErrors);
        while (mxf_next_list_iter_element(&iter))
        {
            vtrError = (VTRErrorAtPos*)mxf_get_iter_element(&iter);
            printf("    %10" PRId64 ":%10" PRId64, count, vtrError->position);
            if (!noSourceTimecode)
            {
                read_time_string_at_position(reader, vtrError->position, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
                printf("%16s%16s", vitcStr, ltcStr);
            }
            printf(" %8s%02x", "0x", vtrError->errorCode);
            if (vtrError->errorCode == 0x00)
            {
                printf("    No error\n");
            }
            else
            {
                /* video error state */
                if ((vtrError->errorCode & 0x07) == 0x01)
                {
                    printf("    Video almost good, ");
                }
                else if ((vtrError->errorCode & 0x07) == 0x02)
                {
                    printf("    Video state cannot be determined, ");
                }
                else if ((vtrError->errorCode & 0x07) == 0x03)
                {
                    printf("    Video state unclear, ");
                }
                else if ((vtrError->errorCode & 0x07) == 0x04)
                {
                    printf("    Video no good, ");
                }
                else
                {
                    printf("    Video good, ");
                }

                /* audio error state */
                if ((vtrError->errorCode & 0x70) == 0x10)
                {
                    printf("Audio almost good\n");
                }
                else if ((vtrError->errorCode & 0x70) == 0x20)
                {
                    printf("Audio state cannot be determined\n");
                }
                else if ((vtrError->errorCode & 0x70) == 0x30)
                {
                    printf("Audio state unclear\n");
                }
                else if ((vtrError->errorCode & 0x70) == 0x40)
                {
                    printf("Audio no good\n");
                }
                else
                {
                    printf("Audio good\n");
                }
            }

            count++;
        }
    }
}

static void write_digibeta_dropouts(Reader *reader, int noSourceTimecode)
{
    MXFListIterator iter;
    DigiBetaDropout *digiBetaDropout;
    uint64_t count;
    char vitcStr[12];
    char ltcStr[12];

    printf("\nDigiBeta dropout results:\n");
    printf("    %d dropouts detected\n", reader->digiBetaDropoutCount);
    if (mxf_get_list_length(&reader->digiBetaDropouts) > 0)
    {
        if (noSourceTimecode)
        {
            printf("    %10s:%10s %10s\n",
                   "num", "frame", "strength");
        }
        else
        {
            printf("    %10s:%10s%16s%16s %10s\n",
                   "num", "frame", "vitc", "ltc", "strength");
        }

        count = 0;
        mxf_initialise_list_iter(&iter, &reader->digiBetaDropouts);
        while (mxf_next_list_iter_element(&iter))
        {
            digiBetaDropout = (DigiBetaDropout*)mxf_get_iter_element(&iter);
            printf("    %10" PRId64 ":%10" PRId64, count, digiBetaDropout->position);
            if (!noSourceTimecode)
            {
                read_time_string_at_position(reader, digiBetaDropout->position, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
                printf("%16s%16s", vitcStr, ltcStr);
            }
            printf(" %10d\n", digiBetaDropout->strength);

            count++;
        }
    }
}

static void write_timecode_breaks(Reader *reader, int noSourceTimecode)
{
    MXFListIterator iter;
    TimecodeBreak *timecodeBreak;
    uint64_t count;
    char vitcStr[12];
    char ltcStr[12];

    printf("\nTimecode break results:\n");
    printf("    %d breaks detected/stored\n", reader->timecodeBreakCount);
    if (mxf_get_list_length(&reader->timecodeBreaks) > 0)
    {
        if (noSourceTimecode)
        {
            printf("    %10s:%10s %10s\n",
                   "num", "frame", "type");
        }
        else
        {
            printf("    %10s:%10s%16s%16s %10s\n",
                   "num", "frame", "vitc", "ltc", "type");
        }

        count = 0;
        mxf_initialise_list_iter(&iter, &reader->timecodeBreaks);
        while (mxf_next_list_iter_element(&iter))
        {
            timecodeBreak = (TimecodeBreak*)mxf_get_iter_element(&iter);
            printf("    %10" PRId64 ":%10" PRId64, count, timecodeBreak->position);
            if (!noSourceTimecode)
            {
                read_time_string_at_position(reader, timecodeBreak->position, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
                printf("%16s%16s", vitcStr, ltcStr);
            }
            printf("     0x%04x\n", timecodeBreak->timecodeType);

            count++;
        }
    }
}

static void write_infax_data(InfaxData *infaxData)
{
    printf("    Format: %s\n", infaxData->format);
    printf("    Programme title: %s\n", infaxData->progTitle);
    printf("    Episode title: %s\n", infaxData->epTitle);
    printf("    Transmission date: %04u-%02u-%02u\n",
           infaxData->txDate.year,
           infaxData->txDate.month,
           infaxData->txDate.day);
    printf("    Magazine prefix: %s\n", infaxData->magPrefix);
    printf("    Programme number: %s\n", infaxData->progNo);
    printf("    Production code: %s\n", infaxData->prodCode);
    printf("    Spool status: %s\n", infaxData->spoolStatus);
    printf("    Stock date: %04u-%02u-%02u\n",
           infaxData->stockDate.year,
           infaxData->stockDate.month,
           infaxData->stockDate.day);
    printf("    Spool descriptor: %s\n", infaxData->spoolDesc);
    printf("    Memo: %s\n", infaxData->memo);
    printf("    Duration: %02" PRId64 ":%02" PRId64 ":%02" PRId64 "\n",
           infaxData->duration / (60 * 60),
           (infaxData->duration % (60 * 60)) / 60,
           (infaxData->duration % (60 * 60)) % 60);
    printf("    Spool number: %s\n", infaxData->spoolNo);
    printf("    Accession number: %s\n", infaxData->accNo);
    printf("    Catalogue detail: %s\n", infaxData->catDetail);
    printf("    Item number: %d\n", infaxData->itemNo);
}

static int write_info(Reader *reader, int showPSEFailures, int showVTRErrors, int showDigiBetaDropouts,
                      int showTimecodeBreaks, int noSourceTimecode)
{
    MXFListIterator iter;
    WriterIdentification *writerIdent;
    int i;
    PSEFailure *pseFailure = NULL;
    uint64_t count;
    uint16_t roundedRate = (uint16_t)(reader->editRate.numerator + reader->editRate.denominator/2) / reader->editRate.denominator;
    char vitcStr[12];
    char ltcStr[12];
    char rateStr[16];
    char audioSamplingRateStr[16];

    if (reader->contentPackageLen > 0)
    {
        CHK_ORET(initialise_timecode_reader(reader));
    }

    if (reader->editRate.denominator == 1)
    {
        mxf_snprintf(rateStr, sizeof(rateStr), "%d", reader->editRate.numerator);
    }
    else
    {
        mxf_snprintf(rateStr, sizeof(rateStr), "%.2f",
                     reader->editRate.numerator / (float)reader->editRate.denominator);
    }

    if (reader->audioSamplingRate.numerator == 48000 && reader->audioSamplingRate.denominator == 1)
    {
        mxf_snprintf(audioSamplingRateStr, sizeof(audioSamplingRateStr), "48kHz");
    }
    else if (reader->audioSamplingRate.denominator > 0)
    {
        mxf_snprintf(audioSamplingRateStr, sizeof(audioSamplingRateStr), "%.2fkHz",
                     reader->audioSamplingRate.numerator / (float)(1000 * reader->audioSamplingRate.denominator));
    }
    else
    {
        audioSamplingRateStr[0] = 0;
    }


    printf("BBC Archive MXF file information\n");

    printf("\nMXF writer identifications:\n");
    i = 0;
    mxf_initialise_list_iter(&iter, &reader->writerIdents);
    while (mxf_next_list_iter_element(&iter))
    {
        writerIdent = (WriterIdentification*)mxf_get_iter_element(&iter);

        if (i == 0)
        {
            printf("%d) Created on %04d-%02u-%02u %02u:%02u:%02u.%03u UTC using ", i,
                   writerIdent->modificationDate.year, writerIdent->modificationDate.month,
                   writerIdent->modificationDate.day, writerIdent->modificationDate.hour,
                   writerIdent->modificationDate.min, writerIdent->modificationDate.sec,
                   writerIdent->modificationDate.qmsec * 4);
        }
        else
        {
            printf("%d) Modified %04d-%02u-%02u %02u:%02u:%02u.%03u UTC using ", i,
                   writerIdent->modificationDate.year, writerIdent->modificationDate.month,
                   writerIdent->modificationDate.day, writerIdent->modificationDate.hour,
                   writerIdent->modificationDate.min, writerIdent->modificationDate.sec,
                   writerIdent->modificationDate.qmsec * 4);
        }
        printf("%ls '%ls' ('%ls')\n", writerIdent->companyName,
               writerIdent->productName, writerIdent->versionString);
        i++;
    }


    printf("\nAV contents:\n");
    printf("    %d video tracks", reader->numVideoTracks);
    if (reader->numVideoTracks == 0)
    {
        printf("\n");
    }
    else
    {
        printf(" (size: %ux%u, depth: %d-bit, aspect ratio: %d:%d, rate: %s fps, %s)\n",
               reader->videoWidth, reader->videoHeight,
               reader->componentDepth,
               reader->aspectRatio.numerator, reader->aspectRatio.denominator,
               rateStr,
               (reader->isProgressive ? "progressive" : "interlaced"));
    }
    printf("    %d audio tracks", reader->numAudioTracks);
    if (reader->numAudioTracks == 0)
    {
        printf("\n");
    }
    else
    {
        printf(" (bits: %d, rate: %s)\n", reader->audioQuantizationBits, audioSamplingRateStr);
    }
    printf("    duration is %" PRId64 " frames at %s fps (%02u:%02u:%02u:%02u)\n",
           reader->duration,
           rateStr,
           (uint16_t)(  reader->duration / (roundedRate * 60 * 60)),
           (uint16_t)(( reader->duration % (roundedRate * 60 * 60)) / (roundedRate * 60)),
           (uint16_t)(((reader->duration % (roundedRate * 60 * 60)) % (roundedRate * 60)) / roundedRate),
           (uint16_t)(((reader->duration % (roundedRate * 60 * 60)) % (roundedRate * 60)) % roundedRate));


    if (reader->contentPackageLen > 0)
    {
        read_time_string_at_position(reader, 0, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
        printf("    start VITC: %s\n", vitcStr);
        printf("    start LTC: %s\n", ltcStr);
    }

    printf("\nSource videotape information:\n");
    write_infax_data(&reader->sourceInfaxData);

    printf("\nLTO/MXF destination information:\n");
    write_infax_data(&reader->ltoInfaxData);


    printf("\nPhoto Sensitive Epilepsy analysis results summary:\n");
    printf("    %d RAW failures detected.", reader->pseFailureCount);
    if (reader->pseFailureCount == 0)
    {
        printf("\n");
    }
    else
    {
        printf(" Check for PSE failure using the '-p, --show-pse-failures' and '-s, --summary-info' command-line options\n");
    }

    printf("\nVTR error results summary:\n");
    printf("    %d errors detected\n", reader->vtrErrorCount);

    printf("\nDigiBeta dropout results summary:\n");
    printf("    %d dropouts detected\n", reader->digiBetaDropoutCount);

    printf("\nTimecode break results summary:\n");
    printf("    %d breaks detected/stored\n", reader->timecodeBreakCount);


    if (showPSEFailures)
    {
        printf("\nPhoto Sensitive Epilepsy analysis results:\n");
        if (mxf_get_list_length(&reader->pseFailures) == 0)
        {
            printf("    Passed - ");
            printf("%d failures detected\n", reader->pseFailureCount);
        }
        else
        {
            printf("    FAILED - ");
            printf("%d failures detected\n", reader->pseFailureCount);
            if (noSourceTimecode)
            {
                printf("    %10s: %10s%10s%10s%10s%10s\n",
                       "num", "frame", "red", "spatial", "lumin", "ext");
            }
            else
            {
                printf("    %10s: %10s%16s%16s%10s%10s%10s%10s\n",
                       "num", "frame", "vitc", "ltc", "red", "spatial", "lumin", "ext");
            }

            count = 0;
            mxf_initialise_list_iter(&iter, &reader->pseFailures);
            while (mxf_next_list_iter_element(&iter))
            {
                pseFailure = (PSEFailure*)mxf_get_iter_element(&iter);
                printf("    %10" PRId64 ": %10" PRId64, count, pseFailure->position);
                if (!noSourceTimecode)
                {
                    read_time_string_at_position(reader, pseFailure->position, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
                    printf("%16s%16s", vitcStr, ltcStr);
                }
                if (pseFailure->redFlash > 0)
                {
                    printf("%10.1f", pseFailure->redFlash / 1000.0);
                }
                else
                {
                    printf("%10.1f", 0.0);
                }
                if (pseFailure->spatialPattern > 0)
                {
                    printf("%10.1f", pseFailure->spatialPattern / 1000.0);
                }
                else
                {
                    printf("%10.1f", 0.0);
                }
                if (pseFailure->luminanceFlash > 0)
                {
                    printf("%10.1f", pseFailure->luminanceFlash / 1000.0);
                }
                else
                {
                    printf("%10.1f", 0.0);
                }
                if (pseFailure->extendedFailure == 0)
                {
                    printf("%10s\n", "F");
                }
                else
                {
                    printf("%10s\n", "T");
                }

                count++;
            }
        }
    }


    if (showVTRErrors)
    {
        write_vtr_errors(reader, noSourceTimecode);
    }

    if (showDigiBetaDropouts)
    {
        write_digibeta_dropouts(reader, noSourceTimecode);
    }

    if (showTimecodeBreaks)
    {
        write_timecode_breaks(reader, noSourceTimecode);
    }

    return 1;
}

static int write_summary(Reader *reader, int showPSEFailures, int showVTRErrors, int showDigiBetaDropouts,
                         int showTimecodeBreaks, int noSourceTimecode)
{
    MXFListIterator iter;
    WriterIdentification *writerIdent;
    PSEFailure *pseFailure = NULL;
    uint64_t count;
    InfaxData *infaxData;
    int redCount;
    int spatialCount;
    int luminanceCount;
    int extendedCount;
    #define MAX_HIST 34
    int redHist[MAX_HIST + 1];
    int spatialHist[MAX_HIST + 1];
    int luminanceHist[MAX_HIST + 1];
    int i;
    int threshold;
    char vitcStr[12];
    char ltcStr[12];

    if (reader->contentPackageLen > 0)
    {
        CHK_ORET(initialise_timecode_reader(reader));
    }

    mxf_initialise_list_iter(&iter, &reader->writerIdents);
    if (mxf_next_list_iter_element(&iter))
    {
        writerIdent = (WriterIdentification*)mxf_get_iter_element(&iter);
        printf("Date of analysis: %04d-%02u-%02u %02u:%02u:%02u\n",
               writerIdent->modificationDate.year,
               writerIdent->modificationDate.month,
               writerIdent->modificationDate.day,
               writerIdent->modificationDate.hour,
               writerIdent->modificationDate.min,
               writerIdent->modificationDate.sec);
    }
    infaxData = &reader->sourceInfaxData;
    printf("Programme title: %s\n", infaxData->progTitle);
    printf("Episode title: %s\n", infaxData->epTitle);
    printf("Magazine prefix: %s\n", infaxData->magPrefix);
    printf("Programme number: %s\n", infaxData->progNo);
    printf("Production code: %s\n", infaxData->prodCode);
    printf("Duration: %02" PRId64 ":%02" PRId64 ":%02" PRId64 "\n",
           infaxData->duration / (60 * 60),
           (infaxData->duration % (60 * 60)) / 60,
           (infaxData->duration % (60 * 60)) % 60);
    printf("Spool number: %s\n", infaxData->spoolNo);
    printf("Accession number: %s\n", infaxData->accNo);
    printf("Catalogue detail: %s\n", infaxData->catDetail);
    printf("Item number: %d\n", infaxData->itemNo);

    if (reader->contentPackageLen > 0)
    {
        read_time_string_at_position(reader, 0, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
        printf("Start VITC: %s\n", vitcStr);
        printf("Start LTC: %s\n", ltcStr);
    }

    if (showPSEFailures)
    {
        // get total violations
        for (i = 0; i <= MAX_HIST; i++)
        {
            redHist[i] = 0;
            spatialHist[i] = 0;
            luminanceHist[i] = 0;
        }
        redCount = 0;
        spatialCount = 0;
        luminanceCount = 0;
        extendedCount = 0;
        if (mxf_get_list_length(&reader->pseFailures) > 0)
        {
            mxf_initialise_list_iter(&iter, &reader->pseFailures);
            while (mxf_next_list_iter_element(&iter))
            {
                pseFailure = (PSEFailure*)mxf_get_iter_element(&iter);
                if (pseFailure->redFlash >= 500)
                    redCount++;
                if (pseFailure->spatialPattern >= 500)
                    spatialCount++;
                if (pseFailure->luminanceFlash >= 500)
                    luminanceCount++;
                if (pseFailure->extendedFailure != 0)
                    extendedCount++;
                i = pseFailure->redFlash / 100;
                if (i > MAX_HIST)
                    i = MAX_HIST;
                redHist[i]++;
                i = pseFailure->spatialPattern / 100;
                if (i > MAX_HIST)
                    i = MAX_HIST;
                spatialHist[i]++;
                i = pseFailure->luminanceFlash / 100;
                if (i > MAX_HIST)
                    i = MAX_HIST;
                luminanceHist[i]++;
            }
        }
        printf("PSE Status: ");
        if (redCount + spatialCount + luminanceCount + extendedCount > 0)
            printf("FAILED\n");
        else
            printf("PASSED\n");
        printf("\nRed Flash violations: %d\n", redCount);
        printf("Spatial Pattern violations: %d\n", spatialCount);
        printf("Luminance Flash violations: %d\n", luminanceCount);
        printf("Extended Failure violations: %d\n", extendedCount);
        if (mxf_get_list_length(&reader->pseFailures) > 0)
        {
            // list worst 100 or so warnings or violations
            threshold = MAX_HIST + 1;
            count = 0;
            while (threshold > 1 && count < 100)
            {
                threshold--;
                count += redHist[threshold] + spatialHist[threshold] +
                         luminanceHist[threshold];
            }
            threshold = threshold * 100;
            printf("\nDetail table threshold: %d\n", threshold);
            if (noSourceTimecode)
            {
                printf("Frame   red  spat   lum   ext\n");
            }
            else
            {
                printf("Frame            vitc             ltc   red  spat   lum   ext\n");
            }
            mxf_initialise_list_iter(&iter, &reader->pseFailures);
            while (mxf_next_list_iter_element(&iter))
            {
                pseFailure = (PSEFailure*)mxf_get_iter_element(&iter);
                if (pseFailure->redFlash >= threshold ||
                    pseFailure->spatialPattern >= threshold ||
                    pseFailure->luminanceFlash >= threshold ||
                    pseFailure->extendedFailure != 0)
                {
                    printf("%5d", (int)pseFailure->position);
                    if (!noSourceTimecode)
                    {
                        read_time_string_at_position(reader, pseFailure->position, vitcStr, sizeof(vitcStr), ltcStr, sizeof(ltcStr));
                        printf("%16s%16s", vitcStr, ltcStr);
                    }
                    printf("%6.1f", pseFailure->redFlash / 1000.0);
                    printf("%6.1f", pseFailure->spatialPattern / 1000.0);
                    printf("%6.1f", pseFailure->luminanceFlash / 1000.0);
                    printf("%6s\n", pseFailure->extendedFailure != 0 ? "X" : "");
                }
            }
        }
    }

    if (showVTRErrors)
    {
        write_vtr_errors(reader, noSourceTimecode);
    }

    if (showDigiBetaDropouts)
    {
        write_digibeta_dropouts(reader, noSourceTimecode);
    }

    if (showTimecodeBreaks)
    {
        write_timecode_breaks(reader, noSourceTimecode);
    }


    return 1;
}


static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <<options>> <mxf filename>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help               display this usage message\n");
    fprintf(stderr, "  -v, --show-vtr-errors    show detailed VTR errors\n");
    fprintf(stderr, "  -p, --show-pse-failures  show detailed PSE failures\n");
    fprintf(stderr, "  -d, --show-digi-dropouts show detailed DigiBeta dropouts\n");
    fprintf(stderr, "  -b, --show-tc-breaks     show detailed timecode breaks\n");
    fprintf(stderr, "  -s, --summary-info       show summary (omit detail)\n");
    fprintf(stderr, "  -t, --no-src-tc          don't search for source VITC and LTC timecodes\n");
    fprintf(stderr, "  --check-issues           exit with error if file contains known issues\n");
}

int main(int argc, const char *argv[])
{
    Reader *reader = NULL;
    int showVTRErrors = 0;
    int showPSEFailures = 0;
    int showDigiBetaDropouts = 0;
    int showTimecodeBreaks = 0;
    int summary = 0;
    int cmdlnIndex = 1;
    const char *mxfFilename = NULL;
    int noSourceTimecode = 0;
    int checkIssues = 0;

    if (argc == 2 &&
        (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }

    while (cmdlnIndex < argc - 1)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdlnIndex], "-v") == 0 ||
                 strcmp(argv[cmdlnIndex], "--show-vtr-errors") == 0)
        {
            showVTRErrors = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "-p") == 0 ||
                 strcmp(argv[cmdlnIndex], "--show-pse-failures") == 0)
        {
            showPSEFailures = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "-d") == 0 ||
                 strcmp(argv[cmdlnIndex], "--show-digi-dropouts") == 0)
        {
            showDigiBetaDropouts = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "-b") == 0 ||
                 strcmp(argv[cmdlnIndex], "--show-tc-breaks") == 0)
        {
            showTimecodeBreaks = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "-s") == 0 ||
                 strcmp(argv[cmdlnIndex], "--summary-info") == 0)
        {
            summary = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "-t") == 0 ||
                 strcmp(argv[cmdlnIndex], "--no-src-tc") == 0)
        {
            noSourceTimecode = 1;
            cmdlnIndex += 1;
        }
        else if (strcmp(argv[cmdlnIndex], "--check-issues") == 0)
        {
            checkIssues = 1;
            cmdlnIndex += 1;
        }
        else
        {
            usage(argv[0]);
            fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlnIndex]);
            return 1;
        }
    }

    if (cmdlnIndex > argc - 1)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing mxf filename\n");
        return 1;
    }
    mxfFilename = argv[cmdlnIndex];



    CHK_OFAIL(create_reader(&reader));

    reader->mxfFilename = mxfFilename;

    if (strstr(mxfFilename, "%d") != NULL)
    {
        MXFPageFile *mxfPageFile;
        if (!mxf_page_file_open_read(mxfFilename, &mxfPageFile))
        {
            mxf_log_error("Failed to open page file '%s'\n", mxfFilename);
            goto fail;
        }
        reader->mxfFile = mxf_page_file_get_file(mxfPageFile);
    }
    else
    {
        if (!mxf_disk_file_open_read(mxfFilename, &reader->mxfFile))
        {
            mxf_log_error("Failed to open disk file '%s'\n", mxfFilename);
            goto fail;
        }
    }

    if (!get_info(reader, checkIssues, showPSEFailures, showVTRErrors, showDigiBetaDropouts, showTimecodeBreaks))
    {
        mxf_log_error("Failed to extract info from '%s'\n", mxfFilename);
        goto fail;
    }

    if (!noSourceTimecode && reader->contentPackageLen <= 0)
    {
        mxf_log_warn("Not including source timecodes - no essence data?\n");
        noSourceTimecode = 1;
    }

    if (summary)
    {
        if (!write_summary(reader, showPSEFailures, showVTRErrors, showDigiBetaDropouts, showTimecodeBreaks, noSourceTimecode))
        {
            mxf_log_error("Failed to write summary info\n");
            goto fail;
        }
    }
    else
    {
        if (!write_info(reader, showPSEFailures, showVTRErrors, showDigiBetaDropouts, showTimecodeBreaks, noSourceTimecode))
        {
            mxf_log_error("Failed to write info\n");
            goto fail;
        }
    }

    free_reader(&reader);
    return 0;

fail:
    free_reader(&reader);
    return 1;
}

