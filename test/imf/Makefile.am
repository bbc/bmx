TESTS = test.sh

EXTRA_DIST = \
	test.sh \
	audio_labels.txt \
	test_audio_1.md5 \
	test_audio_2.md5 \
	test_audio_with_labels_1.md5 \
	test_jpeg2000_1.md5 \
	test_jpeg2000_2.md5 \
	test_rdd36_1.md5 \
	test_rdd36_2.md5
	test_timed_text_1.md5 \
	test_timed_text_2.md5

.PHONY: create-data
create-data:
	${srcdir}/test.sh create-data

.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create-samples
