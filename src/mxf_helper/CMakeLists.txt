list(APPEND bmx_sources
    mxf_helper/ADMCHNAMXFDescriptorHelper.cpp
    mxf_helper/ANCDataMXFDescriptorHelper.cpp
    mxf_helper/AVCIMXFDescriptorHelper.cpp
    mxf_helper/AVCMXFDescriptorHelper.cpp
    mxf_helper/D10MXFDescriptorHelper.cpp
    mxf_helper/DVMXFDescriptorHelper.cpp
    mxf_helper/DataMXFDescriptorHelper.cpp
    mxf_helper/JPEG2000MXFDescriptorHelper.cpp
    mxf_helper/JPEGXSMXFDescriptorHelper.cpp
    mxf_helper/MGAMXFDescriptorHelper.cpp
    mxf_helper/MJPEGMXFDescriptorHelper.cpp
    mxf_helper/MPEG2LGMXFDescriptorHelper.cpp
    mxf_helper/MPEG2Validator.cpp
    mxf_helper/MXFDescriptorHelper.cpp
    mxf_helper/MXFFileFactory.cpp
    mxf_helper/MXFMCALabelHelper.cpp
    mxf_helper/PictureMXFDescriptorHelper.cpp
    mxf_helper/RDD36MXFDescriptorHelper.cpp
    mxf_helper/SoundMXFDescriptorHelper.cpp
    mxf_helper/TimedTextMXFDescriptorHelper.cpp
    mxf_helper/TimedTextMXFResourceProvider.cpp
    mxf_helper/TimedTextManifest.cpp
    mxf_helper/UncCDCIMXFDescriptorHelper.cpp
    mxf_helper/UncRGBAMXFDescriptorHelper.cpp
    mxf_helper/UniqueIdHelper.cpp
    mxf_helper/VBIDataMXFDescriptorHelper.cpp
    mxf_helper/VC2MXFDescriptorHelper.cpp
    mxf_helper/VC3MXFDescriptorHelper.cpp
    mxf_helper/WaveMXFDescriptorHelper.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
