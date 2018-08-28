TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	text_example.xml \
	manifest_1.txt \
	manifest_2.txt \
	font.ttf \
	image.png \
	info_1.xml \
	info_2.xml \
	info_3.xml \
	info_4.xml \
	test_1.md5 \
	test_2.md5 \
	test_3.md5 \
	test_4.md5

.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples
