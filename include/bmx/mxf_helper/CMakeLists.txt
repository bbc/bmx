list(APPEND bmx_headers
    bmx/mxf_helper/ADMCHNAMXFDescriptorHelper.h
    bmx/mxf_helper/ANCDataMXFDescriptorHelper.h
    bmx/mxf_helper/AVCIMXFDescriptorHelper.h
    bmx/mxf_helper/AVCMXFDescriptorHelper.h
    bmx/mxf_helper/D10MXFDescriptorHelper.h
    bmx/mxf_helper/DVMXFDescriptorHelper.h
    bmx/mxf_helper/DataMXFDescriptorHelper.h
    bmx/mxf_helper/EssenceValidator.h
    bmx/mxf_helper/JPEG2000MXFDescriptorHelper.h
    bmx/mxf_helper/JPEGXSMXFDescriptorHelper.h
    bmx/mxf_helper/MGAMXFDescriptorHelper.h
    bmx/mxf_helper/MJPEGMXFDescriptorHelper.h
    bmx/mxf_helper/MPEG2LGMXFDescriptorHelper.h
    bmx/mxf_helper/MPEG2Validator.h
    bmx/mxf_helper/MXFDescriptorHelper.h
    bmx/mxf_helper/MXFFileFactory.h
    bmx/mxf_helper/MXFMCALabelHelper.h
    bmx/mxf_helper/PictureMXFDescriptorHelper.h
    bmx/mxf_helper/RDD36MXFDescriptorHelper.h
    bmx/mxf_helper/SoundMXFDescriptorHelper.h
    bmx/mxf_helper/TimedTextMXFDescriptorHelper.h
    bmx/mxf_helper/TimedTextMXFResourceProvider.h
    bmx/mxf_helper/TimedTextManifest.h
    bmx/mxf_helper/UncCDCIMXFDescriptorHelper.h
    bmx/mxf_helper/UncRGBAMXFDescriptorHelper.h
    bmx/mxf_helper/UniqueIdHelper.h
    bmx/mxf_helper/VBIDataMXFDescriptorHelper.h
    bmx/mxf_helper/VC2MXFDescriptorHelper.h
    bmx/mxf_helper/VC3MXFDescriptorHelper.h
    bmx/mxf_helper/WaveMXFDescriptorHelper.h
)

set(bmx_headers ${bmx_headers} PARENT_SCOPE)
