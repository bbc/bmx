/*
 * MXF OP-1A reader
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>

#include "mxf_reader_int.h"
#include "mxf_op1a_reader.h"
#include "mxf_essence_helper.h"
#include "mxf_index_helper.h"
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_macros.h>


/* TODO: check assumptions (ie. limitation) are met */
/* TODO: check for best effort distinguished values in incomplete header metadata; also, these items could be missing */
/* TODO: combine multiple audio tracks into 1 multi-channel track (check tracknumber->channel assignments) */
/* TODO: disable audio in DV essence if audio tracks are present (eg. add option to VLC rawdv demux to disable audio) */
/* TODO: read index tables; get content package size from index table */
/* TODO: use body offset to calculate first frame number ? */
/* TODO: handle default 0 values for partition->previousPartition or partition->footerPartion */

typedef struct
{
    MXFTrack *track;

    int isVideo;
    uint32_t trackID;
    uint32_t trackNumber;
    mxfRational editRate;
    mxfLength duration;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackID;

    uint32_t sortedTrackIndex;
} WrappedTrack;

typedef struct
{
    mxfPosition currentPosition;

    mxfKey startContentPackageKey;
    uint64_t contentPackageLen;

    mxfKey nextKey;
    uint8_t nextLLen;
    uint64_t nextLen;
} NSFileIndex;


struct EssenceReaderData
{
    MXFPartition *headerPartition;
    MXFHeaderMetadata *headerMetadata;
    int haveFooterMetadata;

    MXFList partitions;

    uint32_t indexSID;
    uint32_t bodySID;

    FileIndex *index;
    NSFileIndex nsIndex; /* file index for non-seekable file */
};



static void free_partition_in_list(void *data)
{
    MXFPartition *partition;

    if (data == NULL)
    {
        return;
    }

    partition = (MXFPartition*)data;
    mxf_free_partition(&partition);
}

static int ns_end_of_essence(NSFileIndex *nsIndex)
{
    return !mxf_equals_key(&nsIndex->nextKey, &nsIndex->startContentPackageKey);
}

static void ns_set_next_kl(NSFileIndex *nsIndex, const mxfKey *key, uint8_t llen, uint64_t len)
{
    nsIndex->nextKey = *key;
    nsIndex->nextLLen = llen;
    nsIndex->nextLen = len;
}

static int ns_position_at_first_frame(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;
    NSFileIndex *nsIndex = &data->nsIndex;
    MXFPartition *partition = data->headerPartition;
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    /* move to the first partition that contains essence data */
    while (data->bodySID != partition->bodySID)
    {
        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_OFAIL(mxf_file_eof(mxfFile));
            ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
            return 1;
        }
        while (!mxf_is_partition_pack(&key))
        {
            CHK_OFAIL(mxf_skip(mxfFile, len));
            if (!mxf_read_kl(mxfFile, &key, &llen, &len))
            {
                CHK_OFAIL(mxf_file_eof(mxfFile));
                ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
                return 1;
            }
        }
        if (partition != NULL && partition != data->headerPartition)
        {
            mxf_free_partition(&partition);
        }
        CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &partition));
    }

    /* move to start of essence data in partition */

    /* if we are in the header partition then the file is positioned after the header metadata,
       otherwise we are positioned after the partition pack that was just read */
    if (partition != data->headerPartition)
    {
        /* skip initial filler which is not included in any header or index byte counts */
        if (!mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
            return 1;
        }

        /* skip header metadata */
        if (partition->headerByteCount > 0)
        {
            CHK_ORET(mxf_skip(mxfFile, partition->headerByteCount - mxfKey_extlen - llen));
            if (!mxf_read_kl(mxfFile, &key, &llen, &len))
            {
                CHK_ORET(mxf_file_eof(mxfFile));
                ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
                return 1;
            }
        }
        else
        {
            CHK_ORET(!mxf_is_header_metadata(&key));
        }
    }
    else
    {
        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
            return 1;
        }
    }


    /* skip index table segments. Note: we are not checking for segment key because it
       could be non-standard, eg. MXF V10 indexes */
    if (partition->indexByteCount > 0)
    {
        CHK_ORET(mxf_skip(mxfFile, partition->indexByteCount - mxfKey_extlen - llen));
        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
            return 1;
        }
    }

    /* check the first essence element KL and position after */
    CHK_ORET(mxf_is_gc_essence_element(&key));
    if (mxf_equals_key(&nsIndex->startContentPackageKey, &g_Null_Key))
    {
        nsIndex->startContentPackageKey = key;
        nsIndex->currentPosition = 0;
    }
    else
    {
        CHK_ORET(mxf_equals_key(&nsIndex->startContentPackageKey, &key));
    }
    ns_set_next_kl(nsIndex, &key, llen, len);

    if (partition != NULL && partition != data->headerPartition)
    {
        mxf_free_partition(&partition);
    }
    return 1;

fail:
    if (partition != NULL && partition != data->headerPartition)
    {
        mxf_free_partition(&partition);
    }
    return 0;
}


static int ns_pos_at_next_frame(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;
    NSFileIndex *nsIndex = &data->nsIndex;
    MXFPartition *partition = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int atEOF;


    /* move to next partition if at end of current partition */
    if (mxf_is_partition_pack(&nsIndex->nextKey))
    {
        key = nsIndex->nextKey;
        llen = nsIndex->nextLLen;
        len = nsIndex->nextLen;
        atEOF = 0;
        while (!atEOF)
        {
            /* read partition pack and check if it contains essence */
            CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &partition));
            if (data->bodySID == partition->bodySID)
            {
                CHK_OFAIL(ns_position_at_first_frame(reader));
                mxf_free_partition(&partition);
                break;
            }
            mxf_free_partition(&partition);

            /* skip this partition */
            while (!atEOF)
            {
                if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                {
                    /* if we fail to read then we must be at eof */
                    CHK_OFAIL(mxf_file_eof(mxfFile));
                    atEOF = 1;
                    ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
                    break;
                }
                ns_set_next_kl(nsIndex, &key, llen, len);

                if (mxf_is_partition_pack(&key))
                {
                    break;
                }
                CHK_OFAIL(mxf_skip(mxfFile, len));
            }
        }

        CHK_ORET(atEOF || mxf_equals_key(&key, &nsIndex->startContentPackageKey));
    }

    return 1;

fail:
    mxf_free_partition(&partition);
    return 0;
}

static int process_metadata(MXFReader *reader, MXFPartition *partition)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *descriptorSet;
    MXFArrayItemIterator arrayIter;
    mxfUL dataDefUL;
    MXFTrack *track;
    EssenceTrack *essenceTrack;
    MXFList wrappedTracks;
    MXFList sortedWrappedTracks;
    WrappedTrack *newWrappedTrack = NULL;
    WrappedTrack *wrappedTrack;
    WrappedTrack *sortedWrappedTrack;
    WrappedTrack *prevSortedWrappedTrack;
    WrappedTrack *firstSortedWrappedTrack;
    MXFListIterator listIter;
    MXFListIterator sortedListIter;
    int wasInserted;
    int haveZeroTrackNumber;
    uint32_t trackID;


    mxf_initialise_list(&wrappedTracks, free);
    mxf_initialise_list(&sortedWrappedTracks, NULL);


    /* create and read the header metadata */

    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_header_metadata(&key));
    CHK_OFAIL(mxf_create_header_metadata(&data->headerMetadata, reader->dataModel));
    CHK_OFAIL(mxf_read_header_metadata(mxfFile, data->headerMetadata,
        partition->headerByteCount, &key, llen, len));


    /* check for metadata only files */

    if (!mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(EssenceContainerData), &essContainerDataSet))
    {
        reader->isMetadataOnly = 1;
        return 1;
    }


    /* get the body and index SID from the (single essence container; external essence not supported) */

    CHK_OFAIL(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), &data->bodySID));
    if (mxf_have_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID)))
    {
        CHK_OFAIL(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), &data->indexSID));
    }
    else
    {
        data->indexSID = 0;
    }


    /* get the clip duration */

    CHK_OFAIL(get_clip_duration(data->headerMetadata, &reader->clip, 0));


    /* get the tracks from the (single) material package */

    haveZeroTrackNumber = 0;
    CHK_OFAIL(mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet));
    CHK_OFAIL(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
    while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &materialPackageTrackSet))
    {
        /* CHK_OFAIL(mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDefUL)); */
        /* NOTE: not failing because files from Omneon were found to have a missing DataDefinition item
           in the Sequence and DMSourceClip referenced by a static DM Track */
        if (!mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDefUL))
        {
            continue;
        }

        if (mxf_is_picture(&dataDefUL) || mxf_is_sound(&dataDefUL))
        {
            CHK_MALLOC_OFAIL(newWrappedTrack, WrappedTrack);
            memset(newWrappedTrack, 0, sizeof(WrappedTrack));
            CHK_OFAIL(mxf_append_list_element(&wrappedTracks, newWrappedTrack));
            wrappedTrack = newWrappedTrack;
            newWrappedTrack = NULL;  /* assigned to list so set to NULL so not free'ed in fail */

            CHK_OFAIL(add_track(reader, &track));
            wrappedTrack->track = track;

            if (mxf_have_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
            {
                CHK_OFAIL(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &wrappedTrack->trackNumber));
            }
            else
            {
                wrappedTrack->trackNumber = 0;
            }
            CHK_OFAIL(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), &wrappedTrack->trackID));
            CHK_OFAIL(mxf_get_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &wrappedTrack->editRate));
            CHK_OFAIL(mxf_uu_get_track_duration(materialPackageTrackSet, &wrappedTrack->duration));
            CHK_OFAIL(mxf_uu_get_track_reference(materialPackageTrackSet, &wrappedTrack->sourcePackageUID, &wrappedTrack->sourceTrackID));
            wrappedTrack->isVideo = mxf_is_picture(&dataDefUL);
            track->isVideo = wrappedTrack->isVideo;
            track->materialTrackID = wrappedTrack->trackID;
            track->materialTrackNumber = wrappedTrack->trackNumber;

            if (wrappedTrack->isVideo)
            {
                track->video.frameRate = wrappedTrack->editRate;
            }

            if (wrappedTrack->trackNumber == 0)
            {
                haveZeroTrackNumber = 1;
            }
        }
    }


    /* sort the tracks; use trackNumber if != 0 else use the trackID; video track is always first */

    mxf_initialise_list_iter(&listIter, &wrappedTracks);
    while (mxf_next_list_iter_element(&listIter))
    {
        wrappedTrack = (WrappedTrack*)mxf_get_iter_element(&listIter);

        wasInserted = 0;
        mxf_initialise_list_iter(&sortedListIter, &sortedWrappedTracks);
        while (mxf_next_list_iter_element(&sortedListIter))
        {
            sortedWrappedTrack = (WrappedTrack*)mxf_get_iter_element(&sortedListIter);
            if ((wrappedTrack->track->isVideo && !sortedWrappedTrack->track->isVideo) ||
                (wrappedTrack->track->isVideo == sortedWrappedTrack->track->isVideo &&
                    ((!haveZeroTrackNumber && wrappedTrack->trackNumber < sortedWrappedTrack->trackNumber) ||
                        (haveZeroTrackNumber && wrappedTrack->trackID < sortedWrappedTrack->trackID))))
            {
                CHK_OFAIL(mxf_insert_list_element(&sortedWrappedTracks, mxf_get_list_iter_index(&sortedListIter),
                    1, wrappedTrack));
                wasInserted = 1;
                break;
            }
        }

        if (!wasInserted)
        {
            CHK_OFAIL(mxf_append_list_element(&sortedWrappedTracks, wrappedTrack));
        }
    }
    /* set the MXFTracks to the same order */
    prevSortedWrappedTrack = NULL;
    firstSortedWrappedTrack = NULL;
    mxf_initialise_list_iter(&sortedListIter, &sortedWrappedTracks);
    while (mxf_next_list_iter_element(&sortedListIter))
    {
        sortedWrappedTrack = (WrappedTrack*)mxf_get_iter_element(&sortedListIter);
        if (firstSortedWrappedTrack == NULL)
        {
            firstSortedWrappedTrack = sortedWrappedTrack;
        }
        if (prevSortedWrappedTrack != NULL)
        {
            prevSortedWrappedTrack->track->next = sortedWrappedTrack->track;
        }
        prevSortedWrappedTrack = sortedWrappedTrack;
    }
    if (prevSortedWrappedTrack != NULL)
    {
        prevSortedWrappedTrack->track->next = NULL;
    }
    if (firstSortedWrappedTrack != NULL)
    {
        reader->clip.tracks = firstSortedWrappedTrack->track;
    }


    /* process source package tracks and linked descriptors */

    mxf_initialise_list_iter(&sortedListIter, &sortedWrappedTracks);
    while (mxf_next_list_iter_element(&sortedListIter))
    {
        sortedWrappedTrack = (WrappedTrack*)mxf_get_iter_element(&sortedListIter);

        CHK_OFAIL(mxf_uu_get_referenced_track(data->headerMetadata, &sortedWrappedTrack->sourcePackageUID,
            sortedWrappedTrack->sourceTrackID, &sourcePackageTrackSet));

        CHK_OFAIL(add_essence_track(essenceReader, &essenceTrack));

        essenceTrack->isVideo = sortedWrappedTrack->isVideo;

        if (mxf_have_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
        {
            CHK_OFAIL(mxf_get_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &essenceTrack->trackNumber));
        }
        else
        {
            essenceTrack->trackNumber = 0;
        }
        CHK_OFAIL(mxf_get_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), &trackID));

        essenceTrack->frameRate = reader->clip.frameRate;
        essenceTrack->playoutDuration = reader->clip.duration;

        essenceTrack->indexSID = data->indexSID;
        essenceTrack->bodySID = data->bodySID;

        /* process the descriptor */

        CHK_OFAIL(mxf_uu_get_referenced_package(data->headerMetadata, &sortedWrappedTrack->sourcePackageUID,
            &sourcePackageSet));
        CHK_OFAIL(mxf_uu_get_track_descriptor(sourcePackageSet, trackID, &descriptorSet));

        if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(CDCIEssenceDescriptor)))
        {
            CHK_OFAIL(process_cdci_descriptor(descriptorSet, sortedWrappedTrack->track, essenceTrack));
        }
        else if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(WaveAudioDescriptor)))
        {
            CHK_OFAIL(process_wav_descriptor(descriptorSet, sortedWrappedTrack->track, essenceTrack));
        }
        else if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(GenericSoundEssenceDescriptor)))
        {
            CHK_OFAIL(process_sound_descriptor(descriptorSet, track, essenceTrack));
        }
        else
        {
            mxf_log_error("Unsupported file descriptor" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }


    /* initialise the playout timecode */

    if (!initialise_playout_timecode(reader, materialPackageSet))
    {
        CHK_ORET(initialise_default_playout_timecode(reader));
    }


    /* initialise the source timecodes */

    initialise_source_timecodes(reader, sourcePackageSet);



    mxf_clear_list(&wrappedTracks);
    mxf_clear_list(&sortedWrappedTracks);
    return 1;

fail:
    SAFE_FREE(newWrappedTrack);
    mxf_clear_list(&wrappedTracks);
    mxf_clear_list(&sortedWrappedTracks);
    return 0;
}

static int get_file_partitions(MXFFile *mxfFile, MXFPartition *headerPartition, MXFList *partitions)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition *partition = NULL;
    MXFPartition *partitionRef;
    uint64_t thisPartition;
    MXFRIP rip;
    MXFRIPEntry *ripEntry;
    MXFListIterator iter;

    mxf_initialise_list(partitions, free_partition_in_list);
    memset(&rip, 0, sizeof(MXFRIP));

    /* use the RIP if there is one */
    if (mxf_read_rip(mxfFile, &rip))
    {
        mxf_initialise_list_iter(&iter, &rip.entries);
        while (mxf_next_list_iter_element(&iter))
        {
            ripEntry = (MXFRIPEntry*)mxf_get_iter_element(&iter);

            /* seek to partition and read and add to list */
            CHK_OFAIL(mxf_file_seek(mxfFile, mxf_get_runin_len(mxfFile) + ripEntry->thisPartition,
                SEEK_SET));
            CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
            CHK_OFAIL(mxf_is_partition_pack(&key));
            CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &partition));
            CHK_OFAIL(mxf_append_list_element(partitions, partition));
            partition = NULL; /* owned by list */
        }
    }

    /* start from footer partition and index back to the header partition */
    else
    {
        if (headerPartition->footerPartition == 0)
        {
            /* no footer partition or at unknown position, so we only index the header partition */
            goto fail;
        }

        thisPartition = headerPartition->footerPartition;
        do
        {
            /* seek to partition and read and add to list */
            CHK_OFAIL(mxf_file_seek(mxfFile, mxf_get_runin_len(mxfFile) + thisPartition, SEEK_SET));
            CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
            CHK_OFAIL(mxf_is_partition_pack(&key));
            CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &partition));
            CHK_OFAIL(mxf_prepend_list_element(partitions, partition));
            partitionRef = partition;
            partition = NULL; /* owned by list */

            thisPartition = partitionRef->previousPartition;
        }
        while (partitionRef->thisPartition != partitionRef->previousPartition);
    }


    mxf_clear_rip(&rip);
    return 1;

fail:
    /* if something failed then just add the header partition
       Note: some Omneon files had references to a footer partition which was not
       actually present in the file */

    mxf_clear_list(partitions);
    mxf_free_partition(&partition);
    mxf_clear_rip(&rip);

    /* create copy of header partition pack */
    CHK_ORET(mxf_create_from_partition(headerPartition, &partition));
    partition->key = headerPartition->key;
    partition->majorVersion = headerPartition->majorVersion;
    partition->minorVersion = headerPartition->minorVersion;
    partition->kagSize = headerPartition->kagSize;
    partition->thisPartition = headerPartition->thisPartition;
    partition->previousPartition = headerPartition->previousPartition;
    partition->footerPartition = headerPartition->footerPartition;
    partition->headerByteCount = headerPartition->headerByteCount;
    partition->indexByteCount = headerPartition->indexByteCount;
    partition->indexSID = headerPartition->indexSID;
    partition->bodyOffset = headerPartition->bodyOffset;
    partition->bodySID = headerPartition->bodySID;

    /* add partition to list */
    if (!mxf_append_list_element(partitions, partition))
    {
        mxf_free_partition(&partition);
        mxf_log_error("Failed to append header partition to list" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    return 1;
}

static void op1a_close(MXFReader *reader)
{
    if (reader->essenceReader == NULL || reader->essenceReader->data == NULL)
    {
        return;
    }

    mxf_free_header_metadata(&reader->essenceReader->data->headerMetadata);
    mxf_free_partition(&reader->essenceReader->data->headerPartition);

    free_index(&reader->essenceReader->data->index);

    mxf_clear_list(&reader->essenceReader->data->partitions);

    SAFE_FREE(reader->essenceReader->data);
}

static int read_content_package(MXFReader *reader, int skip, MXFReaderListener *listener)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;
    FileIndex *index = data->index;
    EssenceTrack *essenceTrack;
    uint8_t *buffer;
    uint64_t bufferSize;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint64_t cpLen;
    uint64_t cpCount;
    int trackIndex;

    get_next_kl(index, &key, &llen, &len);
    cpLen = get_cp_len(index);

    cpCount = mxfKey_extlen + llen;

    /* process essence elements in content package */
    while (cpCount <= cpLen)
    {
        if (!skip && mxf_is_gc_essence_element(&key))
        {
            /* send data to listener */
            if (get_essence_track_with_tracknumber(essenceReader, mxf_get_track_number(&key), &essenceTrack,
                    &trackIndex))
            {
                if (accept_frame(listener, trackIndex))
                {
                    CHK_ORET(read_frame(reader, listener, trackIndex, len, &buffer, &bufferSize));
                    CHK_ORET(send_frame(reader, listener, trackIndex, buffer, bufferSize));
                }
                else
                {
                    CHK_ORET(mxf_skip(mxfFile, len));
                }
                cpCount += len;
            }
            else if (element_is_known_system_item(&key))
            {
                CHK_ORET(extract_system_item_info(reader, &key, len,
                    get_current_position(reader->essenceReader->data->index)));
                cpCount += len;
            }
            else
            {
                CHK_ORET(mxf_skip(mxfFile, len));
                cpCount += len;
            }
        }
        else
        {
            CHK_ORET(mxf_skip(mxfFile, len));
            cpCount += len;
        }

        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            set_next_kl(index, &g_Null_Key, 0, 0);
            break;
        }
        set_next_kl(index, &key, llen, len);
        cpCount += mxfKey_extlen + llen;
    }
    CHK_ORET(cpCount == cpLen + mxfKey_extlen + llen);

    return 1;
}

static int ns_read_content_package(MXFReader *reader, int skip, MXFReaderListener *listener)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;
    NSFileIndex *nsIndex = &data->nsIndex;
    EssenceTrack *essenceTrack;
    uint8_t *buffer;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint64_t cpCount;
    int trackIndex;

    /* must be positioned at start of a content package */
    CHK_ORET(mxf_equals_key(&nsIndex->nextKey, &nsIndex->startContentPackageKey));

    /* get KL read previously */
    key = nsIndex->nextKey;
    llen = nsIndex->nextLLen;
    len = nsIndex->nextLen;

    /* The implementation below only supports a 32-bit size */
    CHK_ORET(len <= UINT32_MAX);

    cpCount = mxfKey_extlen + llen;

    /* process essence elements in content package */
    while (nsIndex->contentPackageLen == 0 || cpCount <= nsIndex->contentPackageLen)
    {
        if (!skip && mxf_is_gc_essence_element(&key))
        {
            if (get_essence_track_with_tracknumber(essenceReader, mxf_get_track_number(&key), &essenceTrack,
                    &trackIndex))
            {
                /* send data to listener */
                if (accept_frame(listener, trackIndex))
                {
                    if (listener && listener->allocate_buffer(listener, trackIndex, &buffer, (uint32_t)len))
                    {
                        CHK_ORET(mxf_file_read(mxfFile, buffer, (uint32_t)len) == len);
                        CHK_ORET(send_frame(reader, listener, trackIndex, buffer, len));
                    }
                    else
                    {
                        CHK_ORET(mxf_skip(mxfFile, len));
                    }
                }
                else
                {
                    CHK_ORET(mxf_skip(mxfFile, len));
                }

                cpCount += len;
            }
            else if (element_is_known_system_item(&key))
            {
                CHK_ORET(extract_system_item_info(reader, &key, len,
                    reader->essenceReader->data->nsIndex.currentPosition));
                cpCount += len;
            }
            else
            {
                CHK_ORET(mxf_skip(mxfFile, len));
                cpCount += len;
            }
        }
        else
        {
            CHK_ORET(mxf_skip(mxfFile, len));
            cpCount += len;
        }

        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            ns_set_next_kl(nsIndex, &g_Null_Key, 0, 0);
            break;
        }
        ns_set_next_kl(nsIndex, &key, llen, len);
        cpCount += mxfKey_extlen + llen;

        /* if don't know the length of the content package then look out for the first element key
           or the start of a new partition */
        if (nsIndex->contentPackageLen == 0 &&
            (mxf_is_partition_pack(&key) || mxf_equals_key(&key, &nsIndex->startContentPackageKey)))
        {
            break;
        }
    }
    CHK_ORET(nsIndex->contentPackageLen == 0 || cpCount == nsIndex->contentPackageLen + mxfKey_extlen + llen);


    /* set content package length if it was unknown */
    if (nsIndex->contentPackageLen == 0)
    {
        nsIndex->contentPackageLen = cpCount - mxfKey_extlen - llen;
    }

    return 1;
}

static int op1a_position_at_frame(MXFReader *reader, int64_t frameNumber)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;

    CHK_ORET(mxf_file_is_seekable(mxfFile));
    CHK_ORET(set_position(mxfFile, data->index, frameNumber));

    return 1;
}

static int64_t op1a_get_last_written_frame_number(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;

    return ix_get_last_written_frame_number(mxfFile, data->index, reader->clip.duration);
}

static int op1a_skip_next_frame(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;


    /* set position at start of next content package */

    if (mxf_file_is_seekable(mxfFile))
    {
        if (end_of_essence(data->index))
        {
            return -1;
        }
        CHK_ORET(set_position(mxfFile, data->index, get_current_position(data->index) + 1));
        if (end_of_essence(data->index))
        {
            return -1;
        }
    }
    else
    {
        if (ns_end_of_essence(&data->nsIndex))
        {
            return -1;
        }
        CHK_ORET(ns_pos_at_next_frame(reader));
        if (ns_end_of_essence(&data->nsIndex))
        {
            return -1;
        }

        CHK_ORET(ns_read_content_package(reader, 1 /* skip */, NULL));

        data->nsIndex.currentPosition++;
    }

    return 1;

}

static int op1a_read_next_frame(MXFReader *reader, MXFReaderListener *listener)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data = essenceReader->data;

    /* set position at start of the current content package */

    if (mxf_file_is_seekable(mxfFile))
    {
        if (end_of_essence(data->index))
        {
            return -1;
        }
        CHK_ORET(set_position(mxfFile, data->index, get_current_position(data->index)));
        if (end_of_essence(data->index))
        {
            return -1;
        }

        CHK_ORET(read_content_package(reader, 0, listener));

        increment_current_position(data->index);
    }
    else
    {
        if (ns_end_of_essence(&data->nsIndex))
        {
            return -1;
        }
        CHK_ORET(ns_pos_at_next_frame(reader));
        if (ns_end_of_essence(&data->nsIndex))
        {
            return -1;
        }

        CHK_ORET(ns_read_content_package(reader, 0, listener));

        data->nsIndex.currentPosition++;
    }

    return 1;
}

static int64_t op1a_get_next_frame_number(MXFReader *reader)
{
    if (mxf_file_is_seekable(reader->mxfFile))
    {
        return get_current_position(reader->essenceReader->data->index);
    }
    else
    {
        return reader->essenceReader->data->nsIndex.currentPosition;
    }
}

static MXFHeaderMetadata* op1a_get_header_metadata(MXFReader *reader)
{
    return reader->essenceReader->data->headerMetadata;
}

static int op1a_have_footer_metadata(MXFReader *reader)
{
    return reader->essenceReader->data->haveFooterMetadata;
}

static int op1a_set_frame_rate(MXFReader *reader, const mxfRational *frameRate)
{
     /* avoid compiler warnings */
    (void) reader;
    (void) frameRate;

    /* currently only support the frame rate defined in the file */
    return 0;
}

int op1a_is_supported(MXFPartition *headerPartition)
{
    MXFListIterator iter;
    mxfUL *label;
    int supportCount = 0;

    if (!mxf_is_op_1a(&headerPartition->operationalPattern))
    {
        return 0;
    }

    if (mxf_get_list_length(&headerPartition->essenceContainers) == 0)
    {
        /* metadata only */
        return 1;
    }

    if (mxf_get_list_length(&headerPartition->essenceContainers) == 1)
    {
        label = (mxfUL*)mxf_get_list_element(&headerPartition->essenceContainers, 0);

        if (is_d10_essence(label))
        {
            return 1;
        }
    }

    mxf_initialise_list_iter(&iter, &headerPartition->essenceContainers);
    while (mxf_next_list_iter_element(&iter))
    {
        label = (mxfUL*)mxf_get_iter_element(&iter);

        if (mxf_equals_ul(label, &MXF_EC_L(MultipleWrappings)))
        {
            supportCount++;
        }
        else if (is_d10_picture_essence(label))
        {
            supportCount++;
        }
        else if (mxf_equals_ul(label, &MXF_EC_L(IECDV_25_525_60_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(IECDV_25_625_50_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_25_525_60_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_25_625_50_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_50_525_60_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_50_625_50_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_100_1080_50_I_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_100_1080_60_I_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_100_720_50_P_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(DVBased_100_720_60_P_FrameWrapped)))
        {
            supportCount++;
        }
        else if (mxf_equals_ul(label, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_25p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_50p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_2997p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_30p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_5994p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_60p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_25p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_2997p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_30p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_50p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_5994p_422_FrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_60p_422_FrameWrapped)))
        {
            supportCount++;
        }
        else if (mxf_equals_ul(label, &MXF_EC_L(BWFFrameWrapped)) ||
                 mxf_equals_ul(label, &MXF_EC_L(AES3FrameWrapped)))
        {
            supportCount++;
        }
        else if (mxf_is_avc_ec(label, 1))
        {
            supportCount++;
        }
    }

    return supportCount > 0;
}

int op1a_initialise_reader(MXFReader *reader, MXFPartition **headerPartition)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data;
    size_t i;
    size_t numPartitions;
    MXFPartition *partition;
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    essenceReader->data = NULL;


    /* init essence reader */
    CHK_MALLOC_OFAIL(essenceReader->data, EssenceReaderData);
    memset(essenceReader->data, 0, sizeof(EssenceReaderData));
    essenceReader->data->headerPartition = *headerPartition;
    essenceReader->close = op1a_close;
    essenceReader->position_at_frame = op1a_position_at_frame;
    essenceReader->skip_next_frame = op1a_skip_next_frame;
    essenceReader->read_next_frame = op1a_read_next_frame;
    essenceReader->get_next_frame_number = op1a_get_next_frame_number;
    essenceReader->get_last_written_frame_number = op1a_get_last_written_frame_number;
    essenceReader->get_header_metadata = op1a_get_header_metadata;
    essenceReader->have_footer_metadata = op1a_have_footer_metadata;
    essenceReader->set_frame_rate = op1a_set_frame_rate;

    data = essenceReader->data;


    if (mxf_file_is_seekable(mxfFile))
    {
        /* get the file partitions */
        CHK_OFAIL(get_file_partitions(mxfFile, data->headerPartition, &data->partitions));


        /* process the last instance of header metadata */
        numPartitions = mxf_get_list_length(&data->partitions);
        for (i = numPartitions; i > 0; i--)
        {
            partition = (MXFPartition*)mxf_get_list_element(&data->partitions, i - 1);
            if (partition->headerByteCount != 0)
            {
                if (!mxf_partition_is_closed(&partition->key))
                {
                    mxf_log_warn("No closed partition with header metadata found" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                }
                if (!mxf_partition_is_complete(&partition->key))
                {
                    mxf_log_warn("No complete partition with header metadata found" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                }

                /* seek to start of partition and skip the partition pack */
                CHK_OFAIL(mxf_file_seek(mxfFile, partition->thisPartition, SEEK_SET));
                CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
                CHK_OFAIL(mxf_skip(mxfFile, len));

                CHK_OFAIL(process_metadata(reader, partition));
                if (mxf_is_footer_partition_pack(&partition->key))
                {
                    data->haveFooterMetadata = 1;
                }
                break;
            }
        }
        if (i == 0)
        {
            mxf_log_error("No partition with header metadata found" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            goto fail;
        }


        if (!reader->isMetadataOnly)
        {
            /* create file index */
            CHK_OFAIL(create_index(mxfFile, &data->partitions, data->indexSID, data->bodySID, &data->index));


            /* position at start of essence */
            CHK_OFAIL(set_position(mxfFile, data->index, 0));
        }
    }
    else
    {
        essenceReader->data->nsIndex.currentPosition = -1;

        /* process the header metadata */
        if (!mxf_partition_is_closed(&data->headerPartition->key))
        {
            mxf_log_warn("Header partition is not closed" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        }
        if (!mxf_partition_is_complete(&data->headerPartition->key))
        {
            mxf_log_warn("Header partition is incomplete" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        }
        CHK_OFAIL(process_metadata(reader, data->headerPartition));


        if (!reader->isMetadataOnly)
        {
            /* position at start of essence */
            CHK_OFAIL(ns_position_at_first_frame(reader));
        }
    }


    *headerPartition = NULL; /* take ownership */
    return 1;

fail:
    reader->essenceReader->data->headerPartition = NULL; /* release ownership */
    /* essenceReader->close() will be called when closing the reader */
    return 0;
}

