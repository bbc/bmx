TESTS = 	test.sh

noinst_PROGRAMS = test_write_archive_mxf
test_write_archive_mxf_SOURCES = test_write_archive_mxf.c
test_write_archive_mxf_CFLAGS = $(BMX_CFLAGS)
test_write_archive_mxf_LDADD = $(BMX_LDADDLIBS) -lwritearchivemxf-1.0

EXTRA_DIST = \
	bbcarchive_1080_25i.md5 \
	bbcarchive_1080_25i.xml \
	test.sh


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples

