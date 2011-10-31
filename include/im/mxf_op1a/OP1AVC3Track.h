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

#ifndef __IM_OP1A_VC3_TRACK_H__
#define __IM_OP1A_VC3_TRACK_H__

#include <im/mxf_op1a/OP1APictureTrack.h>



namespace im
{


class OP1AVC3Track : public OP1APictureTrack
{
public:
    OP1AVC3Track(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                 mxfRational frame_rate, OP1AEssenceType essence_type);
    virtual ~OP1AVC3Track();
};


};



#endif

