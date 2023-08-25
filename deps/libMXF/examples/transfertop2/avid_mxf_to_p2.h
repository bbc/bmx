/*
 * Transfers Avid MXF files to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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

#ifndef AVID_MXF_TO_P2_H_
#define AVID_MXF_TO_P2_H_

#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>


typedef void (*progress_callback) (float percentCompleted);

typedef int (*insert_timecode_callback) (uint8_t *frame, uint32_t frameSize,
    mxfPosition timecode, int drop);


typedef struct
{
    char *filename;

    MXFFile *mxfFile;

    MXFPartition *headerPartition;
    MXFDataModel *dataModel;
    MXFHeaderMetadata *headerMetadata;
    MXFEssenceElement *essenceElement;
    MXFList *aList;

    /* these are references and should not be free'd */
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *contentStorageSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *sourceClipSet;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *descriptorSet;
    MXFMetadataSet *cdciDescriptorSet;
    MXFMetadataSet *bwfDescriptorSet;
    MXFMetadataSet *videoMaterialPackageTrackSet;
    MXFMetadataSet *videoSequenceSet;
} AvidMXFFile;


typedef struct
{
    char *filename;

    MXFFile *mxfFile;

    MXFDataModel *dataModel;
    MXFFilePartitions *partitions;
    MXFHeaderMetadata *headerMetadata;
    MXFEssenceElement *essenceElement;
    MXFIndexTableSegment *indexSegment;

    /* these are references and should not be free'd */
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *contentStorageSet;
    MXFMetadataSet *identSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *sourceClipSet;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *cdciDescriptorSet;
    MXFMetadataSet *aes3DescriptorSet;
    MXFMetadataSet *timecodeComponentSet;


    /* video */
    char codecString[256];
    char frameRateString[256];
    mxfRational frameRate;
    mxfRational aspectRatio;
    uint32_t verticalSubsampling;
    uint32_t horizSubsampling;
    mxfUL pictureEssenceCoding;
    uint32_t storedHeight;
    uint32_t storedWidth;
    uint32_t videoLineMap[2];
    uint32_t frameSize;

    /* audio */
    mxfRational samplingRate;
    uint32_t bitsPerSample;
    uint16_t blockAlign;
    uint32_t avgBps;

    /* audio and video */
    mxfRational editRate;
    mxfLength duration;
    uint64_t startByteOffset;
    uint64_t dataSize;
    mxfUL essenceContainerLabel;
    mxfUL dataDef;
    mxfKey essElementKey;
    int isPicture;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackNumber;
    uint32_t sourceTrackID;
    uint32_t materialTrackNumber;
    uint32_t materialTrackID;
    mxfLength containerDuration;


    uint64_t essenceBytesLength;
    float percentCompletedContribution;
} P2MXFFile;


typedef struct
{
    /* set after calling prepare_transfer() */

    AvidMXFFile inputs[17];
    P2MXFFile outputs[17];
    int numInputs;

    insert_timecode_callback insertTimecode;
    progress_callback progress;

    mxfUMID globalClipID;
    int pictureOutputIndex;
    mxfLength duration;
    mxfRational editUnit;
    mxfPosition timecodeStart;
    int dropFrameFlag;

    uint64_t totalStorageSizeEstimate; /* not an exact size */


    /* set after calling transfer_avid_mxf_to_p2() */

    char clipName[7];
    const char *p2path;
    mxfTimestamp now;


    /* progress % */
    float percentCompleted;
    float lastCallPercentCompleted;

} AvidMXFToP2Transfer;



int prepare_transfer(char *inputFilenames[17], int numInputs,
    int64_t timecodeStart, int dropFrameFlag,
    progress_callback progress, insert_timecode_callback insertTimecode,
    AvidMXFToP2Transfer **transfer);

int transfer_avid_mxf_to_p2(const char *p2path, AvidMXFToP2Transfer *transfer, int *isComplete);

void free_transfer(AvidMXFToP2Transfer **transfer);



#ifdef __cplusplus
}
#endif


#endif

