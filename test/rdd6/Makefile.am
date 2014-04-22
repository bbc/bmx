TESTS = test1.sh test2.sh

EXTRA_DIST = \
	common.sh \
	test1.sh \
	test1.xml \
	test1.md5 \
	test2.sh \
	test2.xml \
	test2.md5

.PHONY: create-data
create-data:
	${srcdir}/test1.sh create_data
	${srcdir}/test2.sh create_data

.PHONY: create-sample
create-sample:
	${srcdir}/test1.sh create_sample
	${srcdir}/test2.sh create_sample


noinst_PROGRAMS = test_rdd6_xml

test_rdd6_xml_SOURCES = test_rdd6_xml.cpp
test_rdd6_xml_CXXFLAGS = $(BMX_CFLAGS)
test_rdd6_xml_LDADD = $(BMX_LDADDLIBS)
