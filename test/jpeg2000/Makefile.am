TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	image_rgb_0001.j2c \
	image_rgb_0003.j2c \
	image_yuv_0001.j2c \
	image_yuv_0003.j2c \
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
