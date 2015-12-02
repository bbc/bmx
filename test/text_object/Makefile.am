TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	utf8.xml \
	utf16le.xml \
	utf16be.xml \
	other.xml \
	test.md5 \
	info.xml \
	utf8_noprolog.xml

.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data

.PHONY: create-sample
create-sample:
	${srcdir}/test.sh create_sample
