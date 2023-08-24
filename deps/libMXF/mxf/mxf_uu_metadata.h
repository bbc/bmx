/*
 * Utility functions for processing header metadata
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

#ifndef MXF_UU_METADATA_H_
#define MXF_UU_METADATA_H_


#ifdef __cplusplus
extern "C"
{
#endif



/*
* Get the top level file source package assuming that there is only a single EssenceContainer.
*/
int mxf_uu_get_top_file_package(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **filePackageSet);


/*
* Get package tracks
*/
int mxf_uu_get_package_tracks(MXFMetadataSet *packageSet, MXFArrayItemIterator *iter);


/*
* Get next track in item array
* return 1 on success, 0 when track not found (dark set or invalid reference) or end of array
*/
int mxf_uu_next_track(MXFHeaderMetadata *headerMetadata, MXFArrayItemIterator *iter, MXFMetadataSet **trackSet);


/*
* Get track info held in the sequence
*/
int mxf_uu_get_track_duration(MXFMetadataSet *trackSet, mxfLength *duration);
int mxf_uu_get_track_duration_at_rate(MXFMetadataSet *trackSet, mxfRational *editRate, mxfLength *duration);
int mxf_uu_get_track_datadef(MXFMetadataSet *trackSet, mxfUL *dataDef);


/*
* Get the source package reference for a single SourceClip contained in track
*/
int mxf_uu_get_track_reference(MXFMetadataSet *trackSet, mxfUMID *sourcePackageUID, uint32_t *sourceTrackID);


/*
* Get the package referenced
*/
int mxf_uu_get_referenced_package(MXFHeaderMetadata *headerMetadata, mxfUMID *sourcePackageUID,
                                  MXFMetadataSet **packageSet);


/*
* Get the track referenced
*/
int mxf_uu_get_referenced_track(MXFHeaderMetadata *headerMetadata, mxfUMID *sourcePackageUID, uint32_t sourceTrackID,
                                MXFMetadataSet **sourceTrackSet);


/*
* Get descriptor linked with the track; if source package descriptor is not a multiple descriptor
* then that descriptor is returned is linked track ID matches or is NULL
*/
int mxf_uu_get_track_descriptor(MXFMetadataSet *sourcePackageSet, uint32_t trackID,
                                MXFMetadataSet **linkedDescriptorSet);

/*
* Get the string item and allocate the required memory
*/
int mxf_uu_get_utf16string_item(MXFMetadataSet *set, const mxfKey *itemKey, mxfUTF16Char **value);


#ifdef __cplusplus
}
#endif


#endif



