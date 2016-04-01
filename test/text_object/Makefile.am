TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	utf8.xml \
	utf16le.xml \
	utf16be.xml \
	other.xml \
	test_d10.md5 \
	test_op1a.md5 \
	test_rdd9.md5 \
	info_d10.xml \
	info_op1a.xml \
	info_rdd9.xml \
	utf8_noprolog.xml

.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples
