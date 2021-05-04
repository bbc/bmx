SUBDIRS = . as02 as11 mxf_op1a rdd9_mxf d10_mxf avid_mxf mxf_reader \
	wave growing_file rdd6 ard_zdf_hdf text_object bmxtranswrap mca \
	as10 misc timed_text jpeg2000

if ENABLE_BBCARCH_CHECK
SUBDIRS += bbcarchive
endif

check_PROGRAMS = create_test_essence file_truncate file_md5

create_test_essence_SOURCES = create_test_essence.cpp
create_test_essence_CXXFLAGS = $(BMX_CFLAGS)

file_truncate_SOURCES = file_truncate.cpp
file_truncate_CXXFLAGS = $(BMX_CFLAGS)

file_md5_SOURCES = file_md5.cpp
file_md5_CXXFLAGS = $(BMX_CFLAGS)
