noinst_LTLIBRARIES = libessenceparser.la

libessenceparser_la_SOURCES = \
	AVCIEssenceParser.cpp \
	AVCIRawEssenceReader.cpp \
	DVEssenceParser.cpp \
	EssenceParserUtils.cpp \
	EssenceParserUtils.h \
	MJPEGEssenceParser.cpp \
	MPEG2EssenceParser.cpp \
	RawEssenceReader.cpp \
	SoundConversion.cpp \
	VC3EssenceParser.cpp

libessenceparser_la_CXXFLAGS = $(BMX_CFLAGS)



noinst_PROGRAMS = test_dvparser test_mjpegparser test_mpeg2parser test_vc3parser

test_dvparser_SOURCES = test_dvparser.cpp
test_dvparser_CXXFLAGS = $(BMX_CFLAGS)
test_dvparser_LDADD = libessenceparser.la ../common/libcommon.la

test_mjpegparser_SOURCES = test_mjpegparser.cpp
test_mjpegparser_CXXFLAGS = $(BMX_CFLAGS)
test_mjpegparser_LDADD = libessenceparser.la ../common/libcommon.la

test_mpeg2parser_SOURCES = test_mpeg2parser.cpp
test_mpeg2parser_CXXFLAGS = $(BMX_CFLAGS)
test_mpeg2parser_LDADD = libessenceparser.la ../common/libcommon.la

test_vc3parser_SOURCES = test_vc3parser.cpp
test_vc3parser_CXXFLAGS = $(BMX_CFLAGS)
test_vc3parser_LDADD = libessenceparser.la ../common/libcommon.la

