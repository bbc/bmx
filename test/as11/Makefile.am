TESTS = test.sh


EXTRA_DIST = \
	avci100_1080i.md5 \
	avci100_1080i.xml \
	d10_50.md5 \
	d10_50.xml \
	mpeg2lg_422p_hl_1080i.md5 \
	mpeg2lg_422p_hl_1080i.xml \
	test.sh \
	as11_core_framework.txt \
	as11_segmentation_framework.txt \
	ukdpp_framework.txt


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples

