TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	text_example.xml \
	manifest_1.txt \
	manifest_2.txt \
	manifest_3.txt \
	font.ttf \
	image.png \
	info_1.xml \
	info_2.xml \
	info_3.xml \
	info_4.xml \
	info_5.xml \
	info_6.xml \
	info_7.xml \
	info_8.xml \
	info_9.xml \
	test_1.md5 \
	test_2.md5 \
	test_3.md5 \
	test_4.md5 \
	test_5.md5 \
	test_6.md5 \
	test_7.md5 \
	test_8.md5 \
	test_9.md5

.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples
