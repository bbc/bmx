/*
 * Copyright (C) 2011  British Broadcasting Corporation.
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <im/mxf_op1a/OP1AVC3Track.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_VC3_EE_K(0x01, MXF_VC3_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1AVC3Track::OP1AVC3Track(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate, OP1AEssenceType essence_type)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mTrackNumber = MXF_VC3_TRACK_NUM(0x01, MXF_VC3_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

OP1AVC3Track::~OP1AVC3Track()
{
}

