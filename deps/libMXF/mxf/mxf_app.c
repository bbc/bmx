/*
 * Copyright (C) 2012, British Broadcasting Corporation
 * All rights reserved.
 *
 * Author: Philip de Nier
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_app.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_page_file.h>
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_macros.h>


#define CHK_OFAIL_NOMSG(cmd)    \
    do {                        \
        if (!(cmd)) {           \
            goto fail;          \
        }                       \
    } while (0)



static int get_infax_data(MXFMetadataSet *dmFrameworkSet, InfaxData *infaxData)
{
    mxfUTF16Char *tempWString = NULL;

#define GET_STRING_ITEM(name, cName)                                                                                    \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name)))                                           \
    {                                                                                                                   \
        CHK_OFAIL(mxf_uu_get_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &tempWString));    \
        CHK_OFAIL(mxf_utf16_to_utf8(infaxData->cName, tempWString, sizeof(infaxData->cName)) != (size_t)(-1));          \
        infaxData->cName[sizeof(infaxData->cName) - 1] = '\0';                                                          \
        SAFE_FREE(tempWString);                                                                                         \
    }

#define GET_DATE_ITEM(name, cName)                                                                                      \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name)))                                           \
    {                                                                                                                   \
        CHK_OFAIL(mxf_get_timestamp_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName));    \
        infaxData->cName.hour = 0;                                                                                      \
        infaxData->cName.min = 0;                                                                                       \
        infaxData->cName.sec = 0;                                                                                       \
        infaxData->cName.qmsec = 0;                                                                                     \
    }

#define GET_INT64_ITEM(name, cName)                                                                                 \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name)))                                       \
    {                                                                                                               \
        CHK_OFAIL(mxf_get_int64_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName));    \
    }

#define GET_UINT32_ITEM(name, cName)                                                                                \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name)))                                       \
    {                                                                                                               \
        CHK_OFAIL(mxf_get_uint32_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName));   \
    }


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

    SAFE_FREE(tempWString);
    return 1;

fail:
    SAFE_FREE(tempWString);
    return 0;
}

static int parse_infax_user_comments(const MXFList *nameList, const MXFList *valueList, InfaxData *infaxData)
{
    const mxfUTF16Char *value;
    int haveValue = 0;
    char buf[64];
    int year, month, day;

#define LPREF(s)            L ## s
#define COMMENT_NAME(s)     L"_BBC_" LPREF(s)

#define PARSE_STRING_MEMBER(name, member)                                                                   \
    if (mxf_avid_get_user_comment(COMMENT_NAME(#name), nameList, valueList, &value))                        \
    {                                                                                                       \
        CHK_OFAIL(mxf_utf16_to_utf8(infaxData->member, value, sizeof(infaxData->member)) != (size_t)(-1));  \
        infaxData->member[sizeof(infaxData->member) - 1] = '\0';                                            \
        haveValue = 1;                                                                                      \
    }

#define PARSE_DATE_MEMBER(name, member)                                                 \
    if (mxf_avid_get_user_comment(COMMENT_NAME(#name), nameList, valueList, &value))    \
    {                                                                                   \
        CHK_OFAIL(mxf_utf16_to_utf8(buf, value, sizeof(buf)) != (size_t)(-1));          \
        buf[sizeof(buf) - 1] = '\0';                                                    \
        CHK_OFAIL(sscanf(buf, "%d-%d-%d", &year, &month, &day) == 3);                   \
        infaxData->member.year = year;                                                  \
        infaxData->member.month = month;                                                \
        infaxData->member.day = day;                                                    \
        haveValue = 1;                                                                  \
    }

#define PARSE_INTEGER_MEMBER(name, format, member)                                      \
    if (mxf_avid_get_user_comment(COMMENT_NAME(#name), nameList, valueList, &value))    \
    {                                                                                   \
        CHK_OFAIL(mxf_utf16_to_utf8(buf, value, sizeof(buf)) != (size_t)(-1));          \
        buf[sizeof(buf) - 1] = '\0';                                                    \
        CHK_OFAIL(sscanf(buf, format, &infaxData->member) == 1);                        \
        haveValue = 1;                                                                  \
    }

    PARSE_STRING_MEMBER(APP_Format, format);
    PARSE_STRING_MEMBER(APP_ProgrammeTitle, progTitle);
    PARSE_STRING_MEMBER(APP_EpisodeTitle, epTitle);
    PARSE_DATE_MEMBER(APP_TransmissionDate, txDate);
    PARSE_STRING_MEMBER(APP_MagazinePrefix, magPrefix);
    PARSE_STRING_MEMBER(APP_ProgrammeNumber, progNo);
    PARSE_STRING_MEMBER(APP_ProductionCode, prodCode);
    PARSE_STRING_MEMBER(APP_SpoolStatus, spoolStatus);
    PARSE_DATE_MEMBER(APP_StockDate, stockDate);
    PARSE_STRING_MEMBER(APP_SpoolDescriptor, spoolDesc);
    PARSE_STRING_MEMBER(APP_Memo, memo);
    PARSE_INTEGER_MEMBER(APP_Duration, "%" PRId64, duration);
    PARSE_STRING_MEMBER(APP_SpoolNumber, spoolNo);
    PARSE_STRING_MEMBER(APP_AccessionNumber, accNo);
    PARSE_STRING_MEMBER(APP_CatalogueDetail, catDetail);
    PARSE_INTEGER_MEMBER(APP_ItemNumber, "%u", itemNo);

    return haveValue;

fail:
    return 0;
}

static int is_metadata_only_file(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **materialPackageSet)
{
    MXFMetadataSet *prefaceSet;
    uint32_t essenceContainerCount = 0;

    CHK_ORET(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers)))
    {
        CHK_ORET(mxf_get_array_item_count(prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), &essenceContainerCount));
    }

    if (essenceContainerCount == 0)
    {
        CHK_ORET(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MaterialPackage), materialPackageSet));
        return 1;
    }

    return 0;
}

static int archive_mxf_get_package_infax_data(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                              InfaxData *infaxData)
{
    MXFArrayItemIterator arrayIter;
    uint8_t *arrayElement;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet *trackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    int haveInfaxData = 0;

    /* go through the tracks and find the DM track */
    CHK_ORET(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &trackSet))
    {
        CHK_ORET(mxf_uu_get_track_datadef(trackSet, &dataDef));
        if (!mxf_is_descriptive_metadata(&dataDef))
        {
            continue;
        }

        /* get to the single DMSegment */
        CHK_ORET(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
        if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
        {
            CHK_ORET(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
            if (count != 1)
            {
                /* more than one segment */
                continue;
            }

            CHK_ORET(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
            if (!mxf_get_strongref(headerMetadata, arrayElement, &dmSet))
            {
                /* unknown DMSegment sub-class */
                continue;
            }
        }
        else
        {
            dmSet = sequenceSet;
        }

        /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
        if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
        {
            if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
            {
                /* if it is an APP_InfaxFramework then it is the Infax data */
                if (mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet) &&
                    mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_InfaxFramework)))
                {
                    CHK_ORET(get_infax_data(dmFrameworkSet, infaxData));
                    haveInfaxData = 1;
                    break;
                }
            }
        }
    }

    return haveInfaxData;
}

static int archive_mxf_check_issues(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet)
{
    MXFArrayItemIterator arrayIter;
    mxfUL dataDef;
    MXFMetadataSet *packageTrackSet;
    MXFMetadataSet *sequenceSet;

    /* check whether there are any descriptive metadata tracks containing a Sequence set with
       missing required property StructuralComponents */
    CHK_ORET(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &packageTrackSet))
    {
        CHK_ORET(mxf_uu_get_track_datadef(packageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            CHK_ORET(mxf_get_strongref_item(packageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                if (!mxf_have_item(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    return 0;
                }
            }
        }
    }

    return 1;
}

static int archive_mxf_get_package_pse_failures(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                                PSEFailure **failures, size_t *numFailures)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet *packageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFListIterator setsIter;
    PSEFailure *newFailures = NULL;
    size_t countedPSEFailures = 0;
    PSEFailure *tmp;
    int32_t i;


    CHK_OFAIL(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &packageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(packageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(packageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                /* first check required property exists to workaround bug in writer */
                if (!mxf_have_item(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }

            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_PSEAnalysisFramework */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_PSEAnalysisFramework)))
                    {
                        /* go back to the sequence and extract the PSE failures */

                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        countedPSEFailures += count;
                        if (newFailures == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(PSEFailure) * countedPSEFailures)) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with PSE failures - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newFailures, sizeof(PSEFailure) * countedPSEFailures)) != NULL);
                        }
                        newFailures = tmp;

                        /* extract the PSE failures */
                        mxf_initialise_sets_iter(headerMetadata, &setsIter);
                        i = 0;
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            PSEFailure *pseFailure = &newFailures[i];

                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &pseFailure->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_RedFlash), &pseFailure->redFlash));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_SpatialPattern), &pseFailure->spatialPattern));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_LuminanceFlash), &pseFailure->luminanceFlash));
                            CHK_OFAIL(mxf_get_boolean_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_ExtendedFailure), &pseFailure->extendedFailure));
                            i++;
                        }
                        break;
                    }
                }
            }
        }
    }

    *failures = newFailures;
    *numFailures = countedPSEFailures;
    return 1;

fail:
    SAFE_FREE(newFailures);
    return 0;
}

static int archive_mxf_get_package_vtr_errors(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                              VTRErrorAtPos **errors, size_t *numErrors)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet *packageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFListIterator setsIter;
    VTRErrorAtPos *newErrors = NULL;
    size_t totalErrors = 0;
    VTRErrorAtPos *tmp;


    CHK_OFAIL(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &packageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(packageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(packageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                /* first check required property exists to workaround bug in writer */
                if (!mxf_have_item(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }

            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_VTRReplayErrorFramework */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_VTRReplayErrorFramework)))
                    {
                        /* go back to the sequence and extract the VTR errors */

                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (newErrors == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(VTRErrorAtPos) * (totalErrors + count))) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with VTR errors - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newErrors, sizeof(VTRErrorAtPos) * (totalErrors + count))) != NULL);
                        }
                        newErrors = tmp;

                        /* extract the VTR errors */
                        mxf_initialise_sets_iter(headerMetadata, &setsIter);
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            VTRErrorAtPos *vtrError = &newErrors[totalErrors];

                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &vtrError->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_uint8_item(dmFrameworkSet, &MXF_ITEM_K(APP_VTRReplayErrorFramework, APP_VTRErrorCode), &vtrError->errorCode));

                            totalErrors++;
                        }
                    }
                }
            }
        }
    }

    *errors = newErrors;
    *numErrors = totalErrors;
    return 1;

fail:
    SAFE_FREE(newErrors);
    return 0;
}

static int archive_mxf_get_package_digibeta_dropouts(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                                     DigiBetaDropout **digiBetaDropouts, size_t *numDigiBetaDropouts)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet *packageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFListIterator setsIter;
    DigiBetaDropout *newDigiBetaDropouts = NULL;
    size_t totalDropouts = 0;
    DigiBetaDropout *tmp;


    CHK_OFAIL(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &packageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(packageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(packageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                /* first check required property exists to workaround bug in writer */
                if (!mxf_have_item(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }

            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_DigiBetaDropoutFramework */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_DigiBetaDropoutFramework)))
                    {
                        /* go back to the sequence and extract the digibeta dropouts */

                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (newDigiBetaDropouts == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(DigiBetaDropout) * (totalDropouts + count))) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with digibeta dropouts - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newDigiBetaDropouts, sizeof(DigiBetaDropout) * (totalDropouts + count))) != NULL);
                        }
                        newDigiBetaDropouts = tmp;

                        /* extract the digibeta dropouts */
                        mxf_initialise_sets_iter(headerMetadata, &setsIter);
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            DigiBetaDropout *digiBetaDropout = &newDigiBetaDropouts[totalDropouts];

                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &digiBetaDropout->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_int32_item(dmFrameworkSet, &MXF_ITEM_K(APP_DigiBetaDropoutFramework, APP_Strength), &digiBetaDropout->strength));

                            totalDropouts++;
                        }
                    }
                }
            }
        }
    }

    *digiBetaDropouts = newDigiBetaDropouts;
    *numDigiBetaDropouts = totalDropouts;
    return 1;

fail:
    SAFE_FREE(newDigiBetaDropouts);
    return 0;
}

static int archive_mxf_get_package_timecode_breaks(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                                   TimecodeBreak **timecodeBreaks, size_t *numTimecodeBreaks)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet *packageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFListIterator setsIter;
    TimecodeBreak *newTimecodeBreaks = NULL;
    size_t totalBreaks = 0;
    TimecodeBreak *tmp;


    CHK_OFAIL(mxf_uu_get_package_tracks(packageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &packageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(packageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(packageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                /* first check required property exists to workaround bug in writer */
                if (!mxf_have_item(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents)))
                {
                    mxf_log_error("Descriptive metadata track's Sequence set is missing the required StructuralComponents property\n");
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }

                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }

            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_TimecodeBreakFramework */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_TimecodeBreakFramework)))
                    {
                        /* go back to the sequence and extract the timecode breaks */

                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (newTimecodeBreaks == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(TimecodeBreak) * (totalBreaks + count))) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with timecode breaks - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newTimecodeBreaks, sizeof(TimecodeBreak) * (totalBreaks + count))) != NULL);
                        }
                        newTimecodeBreaks = tmp;

                        /* extract the digibeta dropouts */
                        mxf_initialise_sets_iter(headerMetadata, &setsIter);
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            TimecodeBreak *timecodeBreak = &newTimecodeBreaks[totalBreaks];

                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &timecodeBreak->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_uint16_item(dmFrameworkSet, &MXF_ITEM_K(APP_TimecodeBreakFramework, APP_TimecodeType), &timecodeBreak->timecodeType));

                            totalBreaks++;
                        }
                    }
                }
            }
        }
    }

    *timecodeBreaks = newTimecodeBreaks;
    *numTimecodeBreaks = totalBreaks;
    return 1;

fail:
    SAFE_FREE(newTimecodeBreaks);
    return 0;
}



int mxf_app_load_extensions(MXFDataModel *dataModel)
{
#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));

#include <mxf/mxf_app_extensions_data_model.h>

    return 1;
}

int mxf_app_is_app_mxf(MXFHeaderMetadata *headerMetadata)
{
    MXFMetadataSet *prefaceSet;
    MXFArrayItemIterator arrayIter;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    int haveBBCScheme = 0;
    mxfUL ul;

    CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));

    /* check is OP-1A */
    CHK_OFAIL(mxf_get_ul_item(prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &ul));
    if (!mxf_is_op_1a(&ul))
    {
        return 0;
    }


    /* check BBC descriptive metadata scheme */
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, DMSchemes)))
    {
        CHK_OFAIL(mxf_initialise_array_item_iterator(prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), &arrayIter));
        while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
        {
            mxf_get_ul(arrayElement, &ul);
            if (mxf_equals_ul(&ul, &MXF_DM_L(APP_PreservationDescriptiveScheme)))
            {
                haveBBCScheme = 1;
                break;
            }
        }
    }
    if (!haveBBCScheme)
    {
        return 0;
    }


    return 1;

fail:
    return 0;
}

int mxf_app_get_info(MXFHeaderMetadata *headerMetadata, ArchiveMXFInfo *info)
{
    MXFList *list = NULL;
    MXFListIterator iter;
    MXFArrayItemIterator arrayIter;
    uint8_t *arrayElement;
    uint32_t arrayElementLen;
    mxfUTF16Char *tempWString = NULL;
    MXFList *nameList = NULL;
    MXFList *valueList = NULL;
    MXFMetadataSet *identSet;
    MXFMetadataSet *fileSourcePackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *descriptorSet;
    MXFMetadataSet *locatorSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *prefaceSet;

    memset(info, 0, sizeof(*info));


    /* read event counts in Preface */
    CHK_ORET(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));

#define GET_EVENT_COUNT(name, var)                                                              \
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, name)))                                  \
        CHK_ORET(mxf_get_uint32_item(prefaceSet, &MXF_ITEM_K(Preface, name), &var));

    GET_EVENT_COUNT(APP_VTRErrorCount,        info->vtrErrorCount)
    GET_EVENT_COUNT(APP_PSEFailureCount,      info->pseFailureCount)
    GET_EVENT_COUNT(APP_DigiBetaDropoutCount, info->digibetaDropoutCount)
    GET_EVENT_COUNT(APP_TimecodeBreakCount,   info->timecodeBreakCount)


    /* if metadata only then only try reading infax user comments */

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        if (mxf_avid_read_string_user_comments(materialPackageSet, &nameList, &valueList))
        {
            info->haveSourceInfaxData = parse_infax_user_comments(nameList, valueList, &info->sourceInfaxData);

            mxf_free_list(&nameList);
            mxf_free_list(&valueList);
        }

        return 1;
    }


    /* Creation timestamp identification info */

    CHK_OFAIL(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(Identification), &list));
    mxf_initialise_list_iter(&iter, list);
    if (mxf_next_list_iter_element(&iter))
    {
        identSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
        CHK_OFAIL(mxf_get_timestamp_item(identSet, &MXF_ITEM_K(Identification, ModificationDate), &info->creationDate));
    }
    mxf_free_list(&list);


    /* LTO Infax data */

    CHK_OFAIL(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    info->haveLTOInfaxData = archive_mxf_get_package_infax_data(headerMetadata, fileSourcePackageSet,
                                                                &info->ltoInfaxData);


    /* original filename */

    CHK_OFAIL(mxf_get_strongref_item(fileSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators)))
    {
        CHK_OFAIL(mxf_initialise_array_item_iterator(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators), &arrayIter));
        while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
        {
            CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &locatorSet));

            if (mxf_is_subclass_of(headerMetadata->dataModel, &locatorSet->key, &MXF_SET_K(NetworkLocator)))
            {
                CHK_OFAIL(mxf_uu_get_utf16string_item(locatorSet, &MXF_ITEM_K(NetworkLocator, URLString), &tempWString));
                CHK_OFAIL(mxf_utf16_to_utf8(info->filename, tempWString, sizeof(info->filename)) != (size_t)(-1));
                info->filename[sizeof(info->filename) - 1] = '\0';
                SAFE_FREE(tempWString);
                break;
            }
        }
    }


    /* source Infax data */

    CHK_OFAIL(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(SourcePackage), &list));
    mxf_initialise_list_iter(&iter, list);
    while (mxf_next_list_iter_element(&iter))
    {
        sourcePackageSet = (MXFMetadataSet*)(mxf_get_iter_element(&iter));

        /* it is the tape SourcePackage if it has a TapeDescriptor */
        CHK_OFAIL(mxf_get_strongref_item(sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
        if (mxf_is_subclass_of(headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(TapeDescriptor)))
        {
            info->haveSourceInfaxData = archive_mxf_get_package_infax_data(headerMetadata, sourcePackageSet,
                                                                           &info->sourceInfaxData);
            break;
        }
    }
    mxf_free_list(&list);

    /* try alternative locations for source Infax data */
    if (!info->haveSourceInfaxData)
    {
        /* framework in the material package */
        CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet));
        info->haveSourceInfaxData = archive_mxf_get_package_infax_data(headerMetadata, materialPackageSet,
                                                                       &info->sourceInfaxData);

        /* UserComments in the MaterialPackage */
        if (!info->haveSourceInfaxData)
        {
            if (mxf_avid_read_string_user_comments(materialPackageSet, &nameList, &valueList))
            {
                info->haveSourceInfaxData = parse_infax_user_comments(nameList, valueList, &info->sourceInfaxData);

                mxf_free_list(&nameList);
                mxf_free_list(&valueList);
            }
        }
    }


    return info->haveSourceInfaxData;

fail:
    SAFE_FREE(tempWString);
    mxf_free_list(&list);
    mxf_free_list(&nameList);
    mxf_free_list(&valueList);
    return 0;
}

int mxf_app_check_issues(MXFHeaderMetadata *headerMetadata)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *fileSourcePackageSet;

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        return archive_mxf_check_issues(headerMetadata, materialPackageSet);
    }

    CHK_ORET(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    return archive_mxf_check_issues(headerMetadata, fileSourcePackageSet);
}

int mxf_app_get_pse_failures(MXFHeaderMetadata *headerMetadata, PSEFailure **failures, size_t *numFailures)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *fileSourcePackageSet;

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        return archive_mxf_get_package_pse_failures(headerMetadata, materialPackageSet, failures, numFailures);
    }

    CHK_ORET(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    return archive_mxf_get_package_pse_failures(headerMetadata, fileSourcePackageSet, failures, numFailures);
}

int mxf_app_get_vtr_errors(MXFHeaderMetadata *headerMetadata, VTRErrorAtPos **errors, size_t *numErrors)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *fileSourcePackageSet;

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        return archive_mxf_get_package_vtr_errors(headerMetadata, materialPackageSet, errors, numErrors);
    }

    CHK_ORET(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    return archive_mxf_get_package_vtr_errors(headerMetadata, fileSourcePackageSet, errors, numErrors);
}

int mxf_app_get_digibeta_dropouts(MXFHeaderMetadata *headerMetadata, DigiBetaDropout **digiBetaDropouts, size_t *numDigiBetaDropouts)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *fileSourcePackageSet;

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        return archive_mxf_get_package_digibeta_dropouts(headerMetadata, materialPackageSet, digiBetaDropouts, numDigiBetaDropouts);
    }

    CHK_ORET(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    return archive_mxf_get_package_digibeta_dropouts(headerMetadata, fileSourcePackageSet, digiBetaDropouts, numDigiBetaDropouts);
}

int mxf_app_get_timecode_breaks(MXFHeaderMetadata *headerMetadata, TimecodeBreak **timecodeBreaks, size_t *numTimecodeBreaks)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *fileSourcePackageSet;

    if (is_metadata_only_file(headerMetadata, &materialPackageSet))
    {
        return archive_mxf_get_package_timecode_breaks(headerMetadata, materialPackageSet, timecodeBreaks, numTimecodeBreaks);
    }

    CHK_ORET(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    return archive_mxf_get_package_timecode_breaks(headerMetadata, fileSourcePackageSet, timecodeBreaks, numTimecodeBreaks);
}

int mxf_app_read_footer_metadata(const char *filename, MXFDataModel *dataModel, MXFHeaderMetadata **headerMetadata)
{
    MXFPageFile *mxfPageFile = NULL;
    MXFFile *mxfFile = NULL;
    MXFRIP rip;
    MXFRIPEntry *lastRIPEntry = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition *footerPartition = NULL;
    int result = 0;
    MXFHeaderMetadata *newHeaderMetadata = NULL;

    memset(&rip, 0, sizeof(rip));


    /* open MXF file */
    if (strstr(filename, "%d") != NULL)
    {
        CHK_OFAIL_NOMSG(mxf_page_file_open_read(filename, &mxfPageFile));
        mxfFile = mxf_page_file_get_file(mxfPageFile);
    }
    else
    {
        CHK_OFAIL_NOMSG(mxf_disk_file_open_read(filename, &mxfFile));
    }

    /* read the RIP */
    CHK_OFAIL_NOMSG(mxf_read_rip(mxfFile, &rip));

    /* read footer partition pack */
    CHK_OFAIL_NOMSG((lastRIPEntry = (MXFRIPEntry*)mxf_get_last_list_element(&rip.entries)) != NULL);
    CHK_OFAIL_NOMSG(mxf_file_seek(mxfFile, mxf_get_runin_len(mxfFile) + lastRIPEntry->thisPartition, SEEK_SET));
    CHK_OFAIL_NOMSG(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL_NOMSG(mxf_is_partition_pack(&key));
    result = 2; /* the file is complete and the presence, or not, of the header metadata will not change */
    *headerMetadata = NULL;

    CHK_OFAIL_NOMSG(mxf_is_footer_partition_pack(&key));
    CHK_OFAIL_NOMSG(mxf_read_partition(mxfFile, &key, len, &footerPartition));

    /* read the header metadata */
    CHK_OFAIL_NOMSG(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL_NOMSG(mxf_is_header_metadata(&key));
    CHK_OFAIL_NOMSG(mxf_create_header_metadata(&newHeaderMetadata, dataModel));
    CHK_OFAIL_NOMSG(mxf_read_header_metadata(mxfFile, newHeaderMetadata,
        footerPartition->headerByteCount, &key, llen, len));

    mxf_free_partition(&footerPartition);
    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);

    *headerMetadata = newHeaderMetadata;
    newHeaderMetadata = NULL;
    return 1;


fail:
    mxf_free_header_metadata(&newHeaderMetadata);
    mxf_free_partition(&footerPartition);
    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);
    return result;
}

int mxf_app_is_metadata_only(const char *filename)
{
    MXFPageFile *mxfPageFile = NULL;
    MXFFile *mxfFile = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition *headerPartition = NULL;
    int result;

    /* open MXF file */
    if (strstr(filename, "%d") != NULL)
    {
        CHK_OFAIL_NOMSG(mxf_page_file_open_read(filename, &mxfPageFile));
        mxfFile = mxf_page_file_get_file(mxfPageFile);
    }
    else
    {
        CHK_OFAIL_NOMSG(mxf_disk_file_open_read(filename, &mxfFile));
    }

    /* read header partition pack */
    if (!mxf_read_header_pp_kl_with_runin(mxfFile, &key, &llen, &len) ||
        !mxf_read_partition(mxfFile, &key, len, &headerPartition))
    {
        return 0;
    }

    /* check whether there is an essence container label */
    result = (mxf_get_list_length(&headerPartition->essenceContainers) == 0);

    mxf_free_partition(&headerPartition);
    mxf_file_close(&mxfFile);
    return result;

fail:
    mxf_free_partition(&headerPartition);
    mxf_file_close(&mxfFile);
    return 0;
}

