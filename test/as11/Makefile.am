TESTS = test.sh


EXTRA_DIST = \
	avci100_1080i.md5 \
	d10_50.md5 \
	test.sh \
	as11_core_framework.txt \
	as11_segmentation_framework.txt \
	ukdpp_framework.txt


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_sample

