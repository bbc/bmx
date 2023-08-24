//=---------------------------------------------------------------------=
//
// $Id$ $Name$
//
// The contents of this file are subject to the AAF SDK Public Source
// License Agreement Version 2.0 (the "License"); You may not use this
// file except in compliance with the License.  The License is available
// in AAFSDKPSL.TXT, or you may obtain a copy of the License from the
// Advanced Media Workflow Association, Inc., or its successor.
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
// the License for the specific language governing rights and limitations
// under the License.  Refer to Section 3.3 of the License for proper use
// of this Exhibit.
//
// WARNING:  Please contact the Advanced Media Workflow Association,
// Inc., for more information about any additional licenses to
// intellectual property covering the AAF Standard that may be required
// to create and distribute AAF compliant products.
// (http://www.amwa.tv/policies).
//
// Copyright Notices:
// The Original Code of this file is Copyright 1998-2009, licensor of the
// Advanced Media Workflow Association.  All rights reserved.
//
// The Initial Developer of the Original Code of this file and the
// licensor of the Advanced Media Workflow Association is
// Avid Technology.
// All rights reserved.
//
//=---------------------------------------------------------------------=

// Labels are 16 octets in size, octet1 .. octet16
//

// Octets 1-7 are assumed as follows - 06.0e.2b.34.04.01.01

// MXF_ESSENCE_CONTAINER_VERSION(octet8)
//
//  Define the registry version
//
//     octet8      = version of registry at time of registration
//
// MXF_ESSENCE_CONTAINER_SPACE(octet9, octet10, octet11, octet12, description)
//
//   Define a number space for labels
//
//     octet9      = octet  9 of the label
//     octet10     = octet 10 of the label
//     octet11     = octet 11 of the label
//     octet12     = octet 12 of the label
//     description = description shared by all labels in this part of the space
//
// MXF_ESSENCE_CONTAINER_NODE(octet13, octet14, description)
//
//   Define a node within a number space
//
//     octet13     = octet 13 of the label
//     octet14     = octet 14 of the label
//     description = description shared by all labels with this prefix
//
// MXF_ESSENCE_CONTAINER_LABEL(octet15, octet16, description)
//
//   Define a label
//
//     octet13     = octet 15 of the label
//     octet14     = octet 16 of the label
//     description = description of the label
//
// MXF_ESSENCE_CONTAINER_END()
//
//   End the definition of a node within a number space
//
// MXF_ESSENCE_CONTAINER_END_SPACE()
//
//   End the definition of a number space for labels
//
// MXF_ESSENCE_CONTAINER_END_VERSION()
//
//   End the definition of the registry version
//

// Default empty definitions so that you only have to define
// those macros you actually want to use.
//
#if !defined(MXF_ESSENCE_CONTAINER_VERSION)
#define MXF_ESSENCE_CONTAINER_VERSION(octet8)
#endif

#if !defined(MXF_ESSENCE_CONTAINER_SPACE)
#define MXF_ESSENCE_CONTAINER_SPACE(octet9, octet10, \
                                    octet11, octet12, description)
#endif

#if !defined(MXF_ESSENCE_CONTAINER_NODE)
#define MXF_ESSENCE_CONTAINER_NODE(octet13, octet14, description)
#endif

#if !defined(MXF_ESSENCE_CONTAINER_LABEL)
#define MXF_ESSENCE_CONTAINER_LABEL(octet15, octet16, description)
#endif

#if !defined(MXF_ESSENCE_CONTAINER_END)
#define MXF_ESSENCE_CONTAINER_END()
#endif

#if !defined(MXF_ESSENCE_CONTAINER_END_SPACE)
#define MXF_ESSENCE_CONTAINER_END_SPACE()
#endif

#if !defined(MXF_ESSENCE_CONTAINER_END_VERSION)
#define MXF_ESSENCE_CONTAINER_END_VERSION()
#endif

// MXF Essence container labels

// This file is hand maintained. It is based on RP224. Eventually this file,
// or one like it, should be automatically generated from the SMPTE registry.
//

MXF_ESSENCE_CONTAINER_VERSION(0x01)

MXF_ESSENCE_CONTAINER_SPACE(0x0d, 0x01, 0x03, 0x01, "SPMTE registered labels")

// Generic container (deprecated)
//
MXF_ESSENCE_CONTAINER_NODE(0x01, 0x01, "Generic container (deprecated)")
// None yet.
MXF_ESSENCE_CONTAINER_END()

// D-10
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x01, "D-10")

  // 50Mbps 625/50i
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x01, "50Mbps 625/50i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x02, "50Mbps 625/50i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x7f, "50Mbps 625/50i (picture only)")
  // 50Mbps 525/60i
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x01, "50Mbps 525/60i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x02, "50Mbps 525/60i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x7f, "50Mbps 525/60i (picture only)")
  // 40Mbps 625/50i
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x01, "40Mbps 625/50i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x02, "40Mbps 625/50i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x7f, "40Mbps 625/50i (picture only)")
  // 40Mbps 525/60i
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x01, "40Mbps 525/60i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x02, "40Mbps 525/60i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x7f, "40Mbps 525/60i (picture only)")
  // 30Mbps 625/50i
MXF_ESSENCE_CONTAINER_LABEL(0x05, 0x01, "30Mbps 625/50i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x05, 0x02, "30Mbps 625/50i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x05, 0x7f, "30Mbps 625/50i (picture only)")
  // 30Mbps 525/60i
MXF_ESSENCE_CONTAINER_LABEL(0x06, 0x01, "30Mbps 525/60i (defined template)")
MXF_ESSENCE_CONTAINER_LABEL(0x06, 0x02, "30Mbps 525/60i (extended template)")
MXF_ESSENCE_CONTAINER_LABEL(0x06, 0x7f, "30Mbps 525/60i (picture only)")

MXF_ESSENCE_CONTAINER_END()

// DV
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x02, "DV")

  // 25Mbps 525/60i and 625/50i
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x01, "IEC 25Mbps 525/60i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x02, "IEC 25Mbps 525/60i (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x01, "IEC 25Mbps 625/50i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x02, "IEC 25Mbps 625/50i (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x01, "IEC 25Mbps 525/60i DVCAM (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x02, "IEC 25Mbps 525/60i DVCAM (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x01, "IEC 25Mbps 625/50i DVCAM (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x02, "IEC 25Mbps 625/50i DVCAM (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x3f, 0x01, "Undefined IEC DV (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x3f, 0x02, "Undefined IEC DV (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x40, 0x01, "25Mbps 525/60i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x40, 0x02, "25Mbps 525/60i (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x41, 0x01, "25Mbps 625/50i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x41, 0x02, "25Mbps 625/50i (clip wrapped)")
  // 50Mbps 525/60i
MXF_ESSENCE_CONTAINER_LABEL(0x50, 0x01, "50Mbps 525/60i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x50, 0x02, "50Mbps 525/60i (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x51, 0x01, "50Mbps 625/50i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x51, 0x02, "50Mbps 625/50i (clip wrapped)")
  // 100Mbps 1080/60i
MXF_ESSENCE_CONTAINER_LABEL(0x60, 0x01, "100Mbps 1080/60i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x60, 0x02, "100Mbps 1080/60i (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x61, 0x01, "100Mbps 1080/50i (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x61, 0x02, "100Mbps 1080/50i (clip wrapped)")
  // 100Mbps 720/60p
MXF_ESSENCE_CONTAINER_LABEL(0x62, 0x01, "100Mbps 720/60p (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x62, 0x02, "100Mbps 720/60p (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x63, 0x01, "100Mbps 720/50p (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x63, 0x02, "100Mbps 720/50p (clip wrapped)")
  // undefined
MXF_ESSENCE_CONTAINER_LABEL(0x7f, 0x01, "undefined (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x7f, 0x02, "undefined (clip wrapped)")

MXF_ESSENCE_CONTAINER_END()

// D-11
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x03, "D-11")
// None yet.
MXF_ESSENCE_CONTAINER_END()

// MPEG Elementary Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x04, "MPEG Elementary Stream")

  // ISO/IEC 13818-1 stream id (0x00 - 0x70) (frame wrapped)
//MXF_ESSENCE_CONTAINER_LABEL(0xXX, 0x01, "stream id 0xXX (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x60, 0x01, "stream id 0x60 (frame wrapped)")

  // ISO/IEC 13818-1 stream id (0x00 - 0x70) (clip wrapped)
//MXF_ESSENCE_CONTAINER_LABEL(0xXX, 0x02, "stream id 0xXX (clip wrapped)")

MXF_ESSENCE_CONTAINER_END()

// Uncompressed Picture
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x05, "Uncompressed Picture")

MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x01, "525 60i 422 13.5MHz (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x02, "525 60i 422 13.5MHz (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x03, "525 60i 422 13.5MHz (line wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x05, "625 50i 422 13.5MHz (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x06, "625 50i 422 13.5MHz (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x07, "625 50i 422 13.5MHz (line wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x19, "525 60p 422 27MHz (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x1A, "525 60p 422 27MHz (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x1B, "525 60p 422 27MHz (line wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x1D, "625 50p 422 27MHz (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x1E, "625 50p 422 27MHz (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x1F, "625 50p 422 27MHz (line wrapped)")

MXF_ESSENCE_CONTAINER_END()

// AES3/BWF
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x06, "AES3/BWF")

MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x00, "BWF (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x00, "BWF (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x00, "AES3 (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x04, 0x00, "AES3 (clip wrapped)")

MXF_ESSENCE_CONTAINER_END()

#define MXF_ESSENCE_CONTAINER_MPEG_WRAPPING_SCHEMES(octet15) \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x01, "stream id " #octet15 " (frame wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x02, "stream id " #octet15 " (clip wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x03, "stream id " #octet15 " (custom stripe wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x04, "stream id " #octet15 " (custom PES wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x05, "stream id " #octet15 " (custom fixed audio size wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x06, "stream id " #octet15 " (custom splice wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x07, "stream id " #octet15 " (custom closed GOP wrapped)") \
MXF_ESSENCE_CONTAINER_LABEL(octet15, 0x7f, "stream id " #octet15 " (custom unconstrained wrapped )")

// MPEG Packetized Elementary Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x07, "MPEG Packetized Elementary Stream")
// None yet.
MXF_ESSENCE_CONTAINER_END()

// MPEG Programme Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x08, "MPEG Programme Stream")
// None yet.
MXF_ESSENCE_CONTAINER_END()

// MPEG Transport Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x09, "MPEG Transport Stream")
// None yet.
MXF_ESSENCE_CONTAINER_END()

// AVC NAL Unit Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x0f, "AVC NAL Unit Stream")

  // AVC NAL Unit Stream, stream id 0x60
MXF_ESSENCE_CONTAINER_MPEG_WRAPPING_SCHEMES(0x60)

MXF_ESSENCE_CONTAINER_END()

// AVC Byte Stream
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x10, "AVC Byte Stream")

  // AVC Byte Stream, stream id 0x60
MXF_ESSENCE_CONTAINER_MPEG_WRAPPING_SCHEMES(0x60)

MXF_ESSENCE_CONTAINER_END()

#undef MXF_ESSENCE_CONTAINER_MPEG_WRAPPING_SCHEMES

// A-Law sound element mapping
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x0A, "A-Law Sound Element")

MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x00, "A-Law Audio (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x00, "A-Law Audio (clip wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x00, "A-Law Audio (custom wrapped)")

MXF_ESSENCE_CONTAINER_END()

// JPEG 2000 picture element mapping
//
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x0C, "JPEG 2000 Picture Element")

MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x00, "JPEG 2000 (frame wrapped)")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x00, "JPEG 2000 (clip wrapped)")

MXF_ESSENCE_CONTAINER_END()

MXF_ESSENCE_CONTAINER_END_SPACE()

#if 0
MXF_ESSENCE_CONTAINER_SPACE(0x0e, 0x04, 0x03, 0x01, "Avid private labels")

// Avid Compressed High Definition
MXF_ESSENCE_CONTAINER_NODE(0x02, 0x06, "AvidHD")

// 1080p 
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x01, "X_6_1_1080p")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x02, "8_7_1_1080p")
MXF_ESSENCE_CONTAINER_LABEL(0x01, 0x03, "8_4_1_1080p")

// 1080i
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x01, "X_6_1_1080i")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x02, "8_7_1_1080i")
MXF_ESSENCE_CONTAINER_LABEL(0x02, 0x03, "8_4_1_1080i")

// 720p
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x01, "X_5_1_720p")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x02, "8_4_1_720p")
MXF_ESSENCE_CONTAINER_LABEL(0x03, 0x03, "8_6_1_720p")

MXF_ESSENCE_CONTAINER_END()

MXF_ESSENCE_CONTAINER_END_SPACE()
#endif

MXF_ESSENCE_CONTAINER_END_VERSION()

// Undefine all macros
//
#undef MXF_ESSENCE_CONTAINER_VERSION

#undef MXF_ESSENCE_CONTAINER_SPACE

#undef MXF_ESSENCE_CONTAINER_NODE

#undef MXF_ESSENCE_CONTAINER_LABEL

#undef MXF_ESSENCE_CONTAINER_END

#undef MXF_ESSENCE_CONTAINER_END_SPACE

#undef MXF_ESSENCE_CONTAINER_END_VERSION
