TESTS = test.sh

EXTRA_DIST = \
	sample.mxf \
	test.sh \
	test_1.md5 \
	test_2.md5

.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples
