TESTS = \
	avci100_1080i.test \
	d10_50.test


EXTRA_DIST = \
	avci100_1080i.test \
	d10_50.test \
	avci100_1080i.md5 \
	d10_50.md5 \
	check.sh \
	create.sh \
	samples.sh \
	as11_core_framework.txt \
	as11_segmentation_framework.txt \
	ukdpp_framework.txt


.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 24 7 avci100_1080i
	${srcdir}/create.sh ${srcdir} 24 11 d10_50


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh ${srcdir} 24 7 avci100_1080i
	${srcdir}/samples.sh ${srcdir} 24 11 d10_50

