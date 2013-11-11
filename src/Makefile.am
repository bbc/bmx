SUBDIRS = common frame essence_parser writer_helper mxf_helper st436 \
	mxf_reader as02 mxf_op1a d10_mxf as11 avid_mxf rdd9_mxf \
	wave clip_writer apps


lib_LTLIBRARIES = libbmx-@BMX_MAJORMINOR@.la

libbmx_@BMX_MAJORMINOR@_la_SOURCES =
libbmx_@BMX_MAJORMINOR@_la_LIBTOOLFLAGS = --tag=CXX
libbmx_@BMX_MAJORMINOR@_la_LDFLAGS = \
	$(BMX_LDFLAGS) -version-info $(BMX_LIBVERSION)
libbmx_@BMX_MAJORMINOR@_la_LIBADD = \
	apps/libapps.la \
	common/libcommon.la \
	frame/libframe.la \
	essence_parser/libessenceparser.la \
	writer_helper/libwriterhelper.la \
	st436/libst436.la \
	mxf_helper/libmxfhelper.la \
	mxf_reader/libmxfreader.la \
	as02/libas02.la \
	mxf_op1a/libmxfop1a.la \
	avid_mxf/libavidmxf.la \
	d10_mxf/libd10mxf.la \
	as11/libas11.la \
	rdd9_mxf/librdd9mxf.la \
	wave/libwave.la \
	clip_writer/libclipwriter.la \
	$(BMX_LIBADDLIBS)



