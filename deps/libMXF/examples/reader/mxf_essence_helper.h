/*
 * Utilities for processing essence data and associated metadata
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

#ifndef MXF_ESSENCE_HELPER_H_
#define MXF_ESSENCE_HELPER_H_


int is_d10_picture_essence(const mxfUL *label);
int is_d10_essence(const mxfUL *label);

int process_cdci_descriptor(MXFMetadataSet *descriptorSet, MXFTrack *track, EssenceTrack *essenceTrack);
int process_wav_descriptor(MXFMetadataSet *descriptorSet, MXFTrack *track, EssenceTrack *essenceTrack);
int process_sound_descriptor(MXFMetadataSet *descriptorSet, MXFTrack *track, EssenceTrack *essenceTrack);

int convert_aes_to_pcm(uint32_t channelCount, uint32_t bitsPerSample,
    uint8_t *buffer, uint64_t aesDataLen, uint64_t *pcmDataLen);

int accept_frame(MXFReaderListener *listener, int trackIndex);
int read_frame(MXFReader *reader, MXFReaderListener *listener, int trackIndex,
    uint64_t frameSize, uint8_t **buffer, uint64_t *bufferSize);
int send_frame(MXFReader *reader, MXFReaderListener *listener, int trackIndex,
    uint8_t *buffer, uint64_t dataLen);

int element_is_known_system_item(const mxfKey *key);
int extract_system_item_info(MXFReader *reader, const mxfKey *key, uint64_t len, mxfPosition position);


#endif

