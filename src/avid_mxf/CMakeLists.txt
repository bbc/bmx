list(APPEND bmx_sources
    avid_mxf/AvidAVCITrack.cpp
    avid_mxf/AvidAVCTrack.cpp
    avid_mxf/AvidAlphaTrack.cpp
    avid_mxf/AvidClip.cpp
    avid_mxf/AvidD10Track.cpp
    avid_mxf/AvidDVTrack.cpp
    avid_mxf/AvidIndexTable.cpp
    avid_mxf/AvidInfo.cpp
    avid_mxf/AvidMJPEGTrack.cpp
    avid_mxf/AvidMPEG2LGTrack.cpp
    avid_mxf/AvidPCMTrack.cpp
    avid_mxf/AvidPictureTrack.cpp
    avid_mxf/AvidTrack.cpp
    avid_mxf/AvidUncTrack.cpp
    avid_mxf/AvidVC3Track.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
