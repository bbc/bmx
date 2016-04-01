TESTS = avci100_1080i50.sh rdd9_422p_1080i50.sh

EXTRA_DIST = \
	avci100_1080i50.sh \
	avci100_1080i50.md5 \
	rdd9_422p_1080i50.sh \
	rdd9_422p_1080i50.md5

.PHONY: create-data
create-data:
	${srcdir}/avci100_1080i50.sh create_data
	${srcdir}/rdd9_422p_1080i50.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/avci100_1080i50.sh create_samples
	${srcdir}/rdd9_422p_1080i50.sh create_samples
