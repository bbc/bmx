/*
 * MXF OP-Atom reader
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

#include "mxf_reader_int.h"
#include <mxf/mxf_avid.h>
#include "mxf_opatom_reader.h"
#include "mxf_essence_helper.h"
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_macros.h>



struct EssenceReaderData
{
    MXFPartition *headerPartition;
    MXFHeaderMetadata *headerMetadata;
    int haveFooterMetadata;

    uint64_t essenceStartPos;
    uint64_t essenceDataSize;

    mxfPosition currentPosition;

    int64_t *avidFrameOffsets;
    int64_t numAvidFrameOffsets;
};


static int64_t get_audio_essence_element_frame_count(uint64_t essenceDataSize, EssenceTrack *essenceTrack)
{
    if (essenceDataSize <= 0 || essenceTrack->frameSize == 0)
    {
        return 0;
    }
    else if (essenceTrack->frameSize > 0)
    {
        return (int64_t)(essenceDataSize / essenceTrack->frameSize);
    }
    else /* essenceTrack->frameSize < 0 */
    {
        int i;
        int64_t numSeqs;
        uint64_t remainder;
        int64_t numFrames;

        numSeqs = (int64_t)(essenceDataSize / essenceTrack->frameSeqSize);
        numFrames = numSeqs * essenceTrack->frameSizeSeqSize;

        remainder = essenceDataSize - numSeqs * essenceTrack->frameSeqSize;
        for (i = 0; i < essenceTrack->frameSizeSeqSize; i++)
        {
            if (remainder < essenceTrack->frameSizeSeq[i])
            {
                break;
            }
            remainder -= essenceTrack->frameSizeSeq[i];
            numFrames++;
        }

        return numFrames;
    }
}

static uint64_t get_audio_file_offset(EssenceReaderData *data, EssenceTrack *essenceTrack, int64_t frameNumber)
{
    if (essenceTrack->frameSize == 0)
    {
        return 0;
    }
    else if (essenceTrack->frameSize > 0)
    {
        return data->essenceStartPos + frameNumber * essenceTrack->frameSize;
    }
    else /* essenceTrack->frameSize < 0 */
    {
        int i;
        int seqIndex;
        uint64_t offset;

        offset = (frameNumber / essenceTrack->frameSizeSeqSize) * essenceTrack->frameSeqSize;

        seqIndex = frameNumber % essenceTrack->frameSizeSeqSize;
        for (i = 0; i < seqIndex; i++)
        {
            offset += essenceTrack->frameSizeSeq[i];
        }

        return data->essenceStartPos + offset;
    }
}

static int64_t get_audio_frame_size(EssenceTrack *essenceTrack, int64_t frameNumber)
{
    if (essenceTrack->frameSize == 0)
    {
        return 0;
    }
    else if (essenceTrack->frameSize > 0)
    {
        return essenceTrack->frameSize;
    }
    else /* essenceTrack->frameSize < 0 */
    {
        return essenceTrack->frameSizeSeq[frameNumber % essenceTrack->frameSizeSeqSize];
    }
}

static int get_data_def_ul(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *trackSet, mxfUL *dataDefUL)
{
    CHK_ORET(mxf_uu_get_track_datadef(trackSet, dataDefUL));
    if (!mxf_is_picture(dataDefUL) && !mxf_is_sound(dataDefUL) && !mxf_is_timecode(dataDefUL))
    {
        /* some Avid files have a weak reference to a DataDefinition instead of a UL */
        mxf_avid_get_data_def(headerMetadata, (mxfUUID*)dataDefUL, dataDefUL);
    }

    return 1;
}

static int add_mjpeg_index_entry(void *data, uint32_t numEntries, MXFIndexTableSegment *segment, int8_t temporalOffset,
    int8_t keyFrameOffset, uint8_t flags, uint64_t streamOffset, uint32_t *sliceOffset, mxfRational *posTable)
{
    MXFReader *reader = (MXFReader*)data;

    (void)segment;
    (void)temporalOffset;
    (void)keyFrameOffset;
    (void)flags;
    (void)sliceOffset;
    (void)posTable;

    if (reader->essenceReader->data->avidFrameOffsets == NULL)
    {
        CHK_MALLOC_ARRAY_ORET(reader->essenceReader->data->avidFrameOffsets, int64_t, numEntries);
        reader->essenceReader->data->numAvidFrameOffsets = 0;
    }

    reader->essenceReader->data->avidFrameOffsets[reader->essenceReader->data->numAvidFrameOffsets] = streamOffset;
    reader->essenceReader->data->numAvidFrameOffsets++;

    return 1;
}

static int read_avid_mjpeg_index_segment(MXFReader *reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFIndexTableSegment *newSegment = NULL;

    SAFE_FREE(reader->essenceReader->data->avidFrameOffsets);
    reader->essenceReader->data->numAvidFrameOffsets = 0;

    /* search for index table key and then read */
    while (1)
    {
        CHK_OFAIL(mxf_read_next_nonfiller_kl(reader->mxfFile, &key, &llen, &len));
        if (mxf_is_index_table_segment(&key))
        {
            CHK_OFAIL(mxf_avid_read_index_table_segment_2(reader->mxfFile, len, mxf_default_add_delta_entry, NULL,
                add_mjpeg_index_entry, (void*)reader, &newSegment));

            mxf_free_index_table_segment(&newSegment);
            return 1;
        }
        else
        {
            CHK_OFAIL(mxf_skip(reader->mxfFile, len));
        }
    }

fail:
    SAFE_FREE(reader->essenceReader->data->avidFrameOffsets);
    reader->essenceReader->data->numAvidFrameOffsets = 0;
    mxf_free_index_table_segment(&newSegment);
    return 0;
}

static int get_avid_mjpeg_frame_info(MXFReader *reader, int64_t frameNumber, int64_t *offset, int64_t *frameSize)
{
    CHK_ORET(frameNumber < reader->essenceReader->data->numAvidFrameOffsets - 1);

    *offset = reader->essenceReader->data->avidFrameOffsets[frameNumber];
    *frameSize = reader->essenceReader->data->avidFrameOffsets[frameNumber + 1] - *offset;

    return 1;
}

static int read_avid_imx_frame_size(MXFReader *reader, int64_t *frameSize)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFIndexTableSegment *segment = NULL;

    while (1)
    {
        CHK_ORET(mxf_read_next_nonfiller_kl(reader->mxfFile, &key, &llen, &len));
        if (mxf_is_index_table_segment(&key))
        {
            CHK_ORET(mxf_read_index_table_segment(reader->mxfFile, len, &segment));
            *frameSize = segment->editUnitByteCount;

            mxf_free_index_table_segment(&segment);
            return 1;
        }
        else
        {
            CHK_ORET(mxf_skip(reader->mxfFile, len));
        }
    }

    return 0;
}

static int opatom_set_frame_rate(MXFReader *reader, const mxfRational *frameRate)
{
    EssenceTrack *essenceTrack;
    MXFTrack *track;
    int64_t numEssenceElementFrames;

    /* can only call this function when reader positioned before first frame */
    CHK_ORET(reader->essenceReader->data->currentPosition == 0);

    CHK_ORET((track = get_mxf_track(reader, 0)) != NULL);

    essenceTrack = get_essence_track(reader->essenceReader, 0);

    if (essenceTrack->isVideo)
    {
        if (memcmp(&essenceTrack->frameRate, frameRate, sizeof(*frameRate)) != 0)
        {
            /* can't change the frame rate for video essence */
            return 0;
        }
    }
    else
    {
        CHK_ORET((frameRate->numerator == 25 && frameRate->denominator == 1) ||
                 (frameRate->numerator == 50 && frameRate->denominator == 1) ||
                 (frameRate->numerator == 30000 && frameRate->denominator == 1001));

        essenceTrack->playoutDuration = mxfr_convert_length(&essenceTrack->frameRate, essenceTrack->playoutDuration, frameRate);
        essenceTrack->frameRate = *frameRate;

        if ((essenceTrack->frameRate.numerator == 25 && essenceTrack->frameRate.denominator == 1) ||
            (essenceTrack->frameRate.numerator == 50 && essenceTrack->frameRate.denominator == 1))
        {
            essenceTrack->frameSize = (uint32_t)(track->audio.blockAlign *
                track->audio.samplingRate.numerator * essenceTrack->frameRate.denominator /
                (double)(track->audio.samplingRate.denominator * essenceTrack->frameRate.numerator));

            reader->clip.duration = mxfr_convert_length(&reader->clip.frameRate, reader->clip.duration, frameRate);
            reader->clip.frameRate = *frameRate;
        }
        else
        {
            /* currently support 48kHz only */
            CHK_ORET(mxfr_is_prof_sampling_rate(&track->audio.samplingRate));

            essenceTrack->frameSize = -1;
            essenceTrack->frameSizeSeq[0] = 1602 * track->audio.blockAlign;
            essenceTrack->frameSizeSeq[1] = 1601 * track->audio.blockAlign;
            essenceTrack->frameSizeSeq[2] = 1602 * track->audio.blockAlign;
            essenceTrack->frameSizeSeq[3] = 1601 * track->audio.blockAlign;
            essenceTrack->frameSizeSeq[4] = 1602 * track->audio.blockAlign;
            essenceTrack->frameSizeSeqSize = 5;
            essenceTrack->frameSeqSize = (1602 * 3 + 1601 * 2) * track->audio.blockAlign;

            reader->clip.duration = mxfr_convert_length(&reader->clip.frameRate, reader->clip.duration, frameRate);
            reader->clip.frameRate = *frameRate;
        }

        /* adjust the clip duration if the essence data size is known at this point and it contains less frames */
        if (reader->essenceReader->data->essenceDataSize > 0)
        {
            numEssenceElementFrames = get_audio_essence_element_frame_count(reader->essenceReader->data->essenceDataSize, essenceTrack);
            if (reader->clip.duration > numEssenceElementFrames)
            {
                reader->clip.duration = numEssenceElementFrames;
            }
        }
    }

    return 1;
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
    MXFMetadataSet *materialPackageSet = NULL;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *descriptorSet;
    MXFArrayItemIterator arrayIter;
    mxfUL dataDefUL;
    int haveVideoOrAudioTrack;
    MXFTrack *track;
    EssenceTrack *essenceTrack;
    mxfUMID sourcePackageUID;
    mxfUMID packageUID;
    uint32_t trackID;


    CHK_ORET(add_track(reader, &track));
    CHK_ORET(add_essence_track(essenceReader, &essenceTrack));

    /* use this essence container label rather than the one in the FileDescriptor
     which is sometimes a weak reference to a ContainerDefinition in Avid files,
     and the ContainerDefinition is not of much use */
    CHK_ORET(mxf_get_list_length(&partition->essenceContainers) == 1);
    track->essenceContainerLabel = *(mxfUL*)mxf_get_list_element(&partition->essenceContainers, 0);


    /* load Avid extensions to the data model */

    CHK_ORET(mxf_avid_load_extensions(reader->dataModel));
    CHK_ORET(mxf_finalise_data_model(reader->dataModel));


    /* create and read the header metadata (filter out meta-dictionary and dictionary except data defs) */

    CHK_ORET(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_is_header_metadata(&key));
    CHK_ORET(mxf_create_header_metadata(&data->headerMetadata, reader->dataModel));
    CHK_ORET(mxf_avid_read_filtered_header_metadata(mxfFile, 0, data->headerMetadata,
        partition->headerByteCount, &key, llen, len));


    /* get the body and index SID */

    CHK_ORET(mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(EssenceContainerData), &essContainerDataSet));
    CHK_ORET(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), &essenceTrack->bodySID));
    if (mxf_have_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID)))
    {
        CHK_ORET(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), &essenceTrack->indexSID));
    }
    else
    {
        essenceTrack->indexSID = 0;
    }


    /* get the top level file source package */

    CHK_ORET(mxf_uu_get_top_file_package(data->headerMetadata, &sourcePackageSet));


    /* get the id and number of the material track referencing the file source package */
    /* Note: is will equal 0 if no material package is present */

    track->materialTrackID = 0;
    CHK_ORET(mxf_get_umid_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &sourcePackageUID));
    if (mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet))
    {
        CHK_ORET(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
        while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &materialPackageTrackSet))
        {
            if (mxf_uu_get_track_reference(materialPackageTrackSet, &packageUID, &trackID))
            {
                if (mxf_equals_umid(&sourcePackageUID, &packageUID))
                {
                    if (mxf_have_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
                    {
                        CHK_ORET(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber),
                            &track->materialTrackNumber));
                    }
                    else
                    {
                        track->materialTrackNumber = 0;
                    }
                    CHK_ORET(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID),
                        &track->materialTrackID));
                    break;
                }
            }

        }
    }


    /* get the track info for the audio or video track */

    haveVideoOrAudioTrack = 0;
    CHK_ORET(mxf_uu_get_package_tracks(sourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_ORET(get_data_def_ul(data->headerMetadata, sourcePackageTrackSet, &dataDefUL));

        if (mxf_is_picture(&dataDefUL) || mxf_is_sound(&dataDefUL))
        {
            CHK_ORET(!haveVideoOrAudioTrack);

            if (mxf_have_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
            {
                CHK_ORET(mxf_get_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &essenceTrack->trackNumber));
            }
            else
            {
                essenceTrack->trackNumber = 0;
            }
            CHK_ORET(mxf_get_rational_item(sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &essenceTrack->frameRate));
            CHK_ORET(mxf_uu_get_track_duration(sourcePackageTrackSet, &essenceTrack->playoutDuration));
            if (mxf_is_picture(&dataDefUL))
            {
                track->video.frameRate = essenceTrack->frameRate;
                essenceTrack->isVideo = 1;
                track->isVideo = 1;
            }
            else
            {
                essenceTrack->isVideo = 0;
                track->isVideo = 0;
            }

            haveVideoOrAudioTrack = 1;
            break;
        }
    }
    CHK_ORET(haveVideoOrAudioTrack);


    /* get the info from the descriptor */

    CHK_ORET(mxf_get_strongref_item(sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));

    if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(CDCIEssenceDescriptor)))
    {
        CHK_ORET(process_cdci_descriptor(descriptorSet, track, essenceTrack));
    }
    else if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(WaveAudioDescriptor)))
    {
        CHK_ORET(process_wav_descriptor(descriptorSet, track, essenceTrack));
    }
    else
    {
        mxf_log_error("Unsupported file descriptor" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }


    /* get the clip duration */

    if (!get_clip_duration(data->headerMetadata, &reader->clip, 1))
    {
        /* no clip info could be read from the material package - get clip info from the source package */
        if (track->isVideo)
        {
            reader->clip.frameRate = essenceTrack->frameRate;
            reader->clip.duration = essenceTrack->playoutDuration;
        }
        else
        {
            reader->clip.frameRate = g_palFrameRate;
            reader->clip.duration = mxfr_convert_length(&essenceTrack->frameRate, essenceTrack->playoutDuration, &g_palFrameRate);
        }
    }


    /* set audio track duration, frame rate and frame size(s) using clip info */

    if (!track->isVideo)
    {
        opatom_set_frame_rate(reader, &reader->clip.frameRate);
    }


    /* initialise the playout timecode */

    if (materialPackageSet != NULL)
    {
        if (!initialise_playout_timecode(reader, materialPackageSet))
        {
            CHK_ORET(initialise_default_playout_timecode(reader));
        }
    }
    else
    {
        CHK_ORET(initialise_default_playout_timecode(reader));
    }


    /* initialise the source timecodes */

    initialise_source_timecodes(reader, sourcePackageSet);


    return 1;
}

static void opatom_close(MXFReader *reader)
{
    if (reader->essenceReader == NULL || reader->essenceReader->data == NULL)
    {
        return;
    }

    mxf_free_header_metadata(&reader->essenceReader->data->headerMetadata);
    mxf_free_partition(&reader->essenceReader->data->headerPartition);
    SAFE_FREE(reader->essenceReader->data->avidFrameOffsets);

    SAFE_FREE(reader->essenceReader->data);
}

static int opatom_position_at_frame(MXFReader *reader, int64_t frameNumber)
{
    MXFFile *mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceReaderData *data = reader->essenceReader->data;
    EssenceTrack *essenceTrack;
    int64_t frameSize;
    int64_t fileOffset;

    CHK_ORET(mxf_file_is_seekable(mxfFile));

    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);

    essenceTrack = get_essence_track(reader->essenceReader, 0);
    if (essenceTrack->frameSize < 0)
    {
        if (essenceTrack->isVideo)
        {
            CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
            CHK_OFAIL(get_avid_mjpeg_frame_info(reader, frameNumber, &fileOffset, &frameSize));
            CHK_OFAIL(mxf_file_seek(mxfFile, data->essenceStartPos + fileOffset, SEEK_SET));
        }
        else /* audio frame sequence */
        {
            CHK_OFAIL(mxf_file_seek(mxfFile, get_audio_file_offset(data, essenceTrack, frameNumber), SEEK_SET));
        }
    }
    else
    {
        CHK_OFAIL(mxf_file_seek(mxfFile,
            data->essenceStartPos + essenceTrack->frameSize * frameNumber,
            SEEK_SET));
    }

    data->currentPosition = frameNumber;

    return 1;

fail:
    CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    return 0;
}

static int64_t opatom_get_last_written_frame_number(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReaderData *data = reader->essenceReader->data;
    EssenceTrack *essenceTrack;
    int64_t fileSize;
    int64_t targetPosition;

    if (reader->clip.duration < 0)
    {
        /* not supported because we could return a position past the end into the footer */
        return -1;
    }
    if ((fileSize = mxf_file_size(mxfFile)) < 0)
    {
        return -1;
    }

    essenceTrack = get_essence_track(reader->essenceReader, 0);

    if (essenceTrack->frameSize < 0)
    {
        if (essenceTrack->isVideo)
        {
            /* if the index table wasn't read then we don't know the length */
            if (reader->essenceReader->data->avidFrameOffsets == NULL)
            {
                return -1;
            }
            targetPosition = reader->clip.duration - 1;
        }
        else /* audio frame sequence */
        {
            targetPosition = get_audio_essence_element_frame_count(fileSize - data->essenceStartPos, essenceTrack) - 1;
            if (reader->clip.duration >= 0 && targetPosition >= reader->clip.duration)
            {
                targetPosition = reader->clip.duration - 1;
            }
        }
    }
    else
    {
        /* get file size and calculate the furthest position */
        targetPosition = (fileSize - data->essenceStartPos) / essenceTrack->frameSize - 1;
        if (reader->clip.duration >= 0 && targetPosition >= reader->clip.duration)
        {
            targetPosition = reader->clip.duration - 1;
        }
    }

    return targetPosition;
}

static int opatom_skip_next_frame(MXFReader *reader)
{
    MXFFile *mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceTrack *essenceTrack;
    int64_t frameSize;
    int64_t fileOffset;

    essenceTrack = get_essence_track(reader->essenceReader, 0);

    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);

    if (essenceTrack->frameSize < 0)
    {
        if (essenceTrack->isVideo)
        {
            CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
            CHK_OFAIL(get_avid_mjpeg_frame_info(reader, reader->essenceReader->data->currentPosition, &fileOffset, &frameSize));
            CHK_OFAIL(mxf_skip(mxfFile, frameSize));
        }
        else /* audio frame sequence */
        {
            frameSize = get_audio_frame_size(essenceTrack, reader->essenceReader->data->currentPosition);
            CHK_OFAIL(mxf_skip(mxfFile, frameSize));
        }
    }
    else
    {
        CHK_OFAIL(mxf_skip(mxfFile, essenceTrack->frameSize));
    }

    reader->essenceReader->data->currentPosition++;

    return 1;

fail:
    if (mxf_file_is_seekable(mxfFile))
    {
        CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }
    /* TODO: recovery when file is not seekable; eg. store state and try recover
       when next frame is read or position is set */
    return 0;
}

static int opatom_read_next_frame(MXFReader *reader, MXFReaderListener *listener)
{
    MXFFile *mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceTrack *essenceTrack;
    uint8_t *buffer = NULL;
    uint64_t bufferSize = 0;
    int64_t frameSize = 0;
    int64_t fileOffset = 0;

    essenceTrack = get_essence_track(reader->essenceReader, 0);

    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);

    if (essenceTrack->frameSize < 0)
    {
        if (essenceTrack->isVideo)
        {
            CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
            CHK_OFAIL(get_avid_mjpeg_frame_info(reader, reader->essenceReader->data->currentPosition, &fileOffset, &frameSize));
            if (accept_frame(listener, 0))
            {
                CHK_OFAIL(read_frame(reader, listener, 0, frameSize, &buffer, &bufferSize));
                CHK_OFAIL(send_frame(reader, listener, 0, buffer, bufferSize));
            }
            else
            {
                CHK_OFAIL(mxf_skip(mxfFile, frameSize));
            }
        }
        else /* audio frame sequence */
        {
            frameSize = get_audio_frame_size(essenceTrack, reader->essenceReader->data->currentPosition);

            if (accept_frame(listener, 0))
            {
                CHK_OFAIL(read_frame(reader, listener, 0, frameSize, &buffer, &bufferSize));
                CHK_OFAIL(send_frame(reader, listener, 0, buffer, bufferSize));
            }
            else
            {
                CHK_OFAIL(mxf_skip(mxfFile, frameSize));
            }
        }
    }
    else
    {
        if (accept_frame(listener, 0))
        {
            CHK_OFAIL(read_frame(reader, listener, 0, essenceTrack->frameSize, &buffer, &bufferSize));
            CHK_OFAIL(send_frame(reader, listener, 0, buffer, bufferSize));
        }
        else
        {
            CHK_OFAIL(mxf_skip(mxfFile, essenceTrack->frameSize));
        }
    }

    reader->essenceReader->data->currentPosition++;

    return 1;

fail:
    if (mxf_file_is_seekable(mxfFile))
    {
        CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }
    /* TODO: recovery when file is not seekable; eg. store state and try recover
       when next frame is read or position is set */
    return 0;
}

static int64_t opatom_get_next_frame_number(MXFReader *reader)
{
    return reader->essenceReader->data->currentPosition;
}

static MXFHeaderMetadata *opatom_get_header_metadata(MXFReader *reader)
{
    return reader->essenceReader->data->headerMetadata;
}

static int opatom_have_footer_metadata(MXFReader *reader)
{
    return reader->essenceReader->data->haveFooterMetadata;
}

int opa_is_supported(MXFPartition *headerPartition)
{
    mxfUL *label;

    if (mxf_get_list_length(&headerPartition->essenceContainers) != 1)
    {
        return 0;
    }
    label = (mxfUL*)mxf_get_list_element(&headerPartition->essenceContainers, 0);

    if (!mxf_is_op_atom(&headerPartition->operationalPattern))
    {
        if (mxf_is_op_1a(&headerPartition->operationalPattern) &&
                (mxf_equals_ul(label, &MXF_EC_L(BWFClipWrapped)) ||
                    mxf_equals_ul(label, &MXF_EC_L(AES3ClipWrapped))))
        {
            /* pretend OP-1A file with clip-wrapped audio is an OP-Atom file */
            /* TODO: separate clip/frame wrapping from OP-1A/Atom */
            return 1;
        }

        return 0;
    }

    if (mxf_equals_ul(label, &MXF_EC_L(IECDV_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(IECDV_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_50_525_60_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_100_720_50_P_ClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(DVBased_100_720_60_P_ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(BWFClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(AES3ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_25p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_50p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_2997p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_30p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_5994p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_60p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_25p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_2997p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_30p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(AvidMJPEGClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(DNxHD1080i1242ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD1080i1243ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD1080p1253ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD1080p1237ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD1080p1238ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD720p1252ClipWrapped)) ||
             mxf_equals_ul(label, &MXF_EC_L(DNxHD720p1251ClipWrapped)))
    {
        return 1;
    }
    else if (is_d10_picture_essence(label))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(AvidIMX30_625_50)) ||
             mxf_equals_ul(label, &MXF_EC_L(AvidIMX40_625_50)) ||
             mxf_equals_ul(label, &MXF_EC_L(AvidIMX50_625_50)) ||
             mxf_equals_ul(label, &MXF_EC_L(AvidIMX30_525_60)) ||
             mxf_equals_ul(label, &MXF_EC_L(AvidIMX40_525_60)) ||
             mxf_equals_ul(label, &MXF_EC_L(AvidIMX50_525_60)))
    {
        return 1;
    }
    else if (mxf_is_avc_ec(label, 0))
    {
        return 1;
    }


    return 0;
}

int opa_initialise_reader(MXFReader *reader, MXFPartition **headerPartition)
{
    MXFFile *mxfFile = reader->mxfFile;
    EssenceReader *essenceReader = reader->essenceReader;
    EssenceReaderData *data;
    EssenceTrack *essenceTrack;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int64_t filePos;

    essenceReader->data = NULL;


    /* init essence reader */

    CHK_MALLOC_OFAIL(essenceReader->data, EssenceReaderData);
    memset(essenceReader->data, 0, sizeof(EssenceReaderData));
    essenceReader->data->headerPartition = *headerPartition;
    essenceReader->close = opatom_close;
    essenceReader->position_at_frame = opatom_position_at_frame;
    essenceReader->skip_next_frame = opatom_skip_next_frame;
    essenceReader->read_next_frame = opatom_read_next_frame;
    essenceReader->get_next_frame_number = opatom_get_next_frame_number;
    essenceReader->get_last_written_frame_number = opatom_get_last_written_frame_number;
    essenceReader->get_header_metadata = opatom_get_header_metadata;
    essenceReader->have_footer_metadata = opatom_have_footer_metadata;
    essenceReader->set_frame_rate = opatom_set_frame_rate;

    data = essenceReader->data;


    /* process the header metadata */

    CHK_OFAIL(process_metadata(reader, data->headerPartition));
    CHK_OFAIL(get_num_essence_tracks(essenceReader) == 1);
    essenceTrack = get_essence_track(essenceReader, 0);


    /* read the index table for Avid MJPEG files */

    if (mxf_equals_ul(&MXF_EC_L(AvidMJPEGClipWrapped), &get_mxf_track(reader, 0)->essenceContainerLabel))
    {
        CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
        CHK_OFAIL(read_avid_mjpeg_index_segment(reader));
        CHK_OFAIL(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }

    /* read the Avid IMX's index table edit unit byte count to set the frame size */

    if (mxf_equals_ul(&MXF_EC_L(AvidIMX30_625_50), &get_mxf_track(reader, 0)->essenceContainerLabel) ||
        mxf_equals_ul(&MXF_EC_L(AvidIMX40_625_50), &get_mxf_track(reader, 0)->essenceContainerLabel) ||
        mxf_equals_ul(&MXF_EC_L(AvidIMX50_625_50), &get_mxf_track(reader, 0)->essenceContainerLabel) ||
        mxf_equals_ul(&MXF_EC_L(AvidIMX30_525_60), &get_mxf_track(reader, 0)->essenceContainerLabel) ||
        mxf_equals_ul(&MXF_EC_L(AvidIMX40_525_60), &get_mxf_track(reader, 0)->essenceContainerLabel) ||
        mxf_equals_ul(&MXF_EC_L(AvidIMX50_525_60), &get_mxf_track(reader, 0)->essenceContainerLabel))
    {
        CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
        CHK_OFAIL(read_avid_imx_frame_size(reader, &essenceTrack->frameSize));
        CHK_OFAIL(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }


    /* move to start of essence container in the body partition */

    while (1)
    {
        CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
        if (mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key))
        {
            break;
        }
        CHK_OFAIL(mxf_skip(mxfFile, len));
    }

    data->essenceDataSize = len;

    CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
    data->essenceStartPos = filePos + essenceTrack->avidFirstFrameOffset;
    data->currentPosition = 0;


    *headerPartition = NULL; /* take ownership */
    return 1;

fail:
    reader->essenceReader->data->headerPartition = NULL; /* release ownership */
    /* essenceReader->close() will be called when closing the reader */
    return 0;
}

