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
	samples.sh


.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 7 avci100_1080i
	${srcdir}/create.sh ${srcdir} 11 d10_50


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh ${srcdir} 7 avci100_1080i
	${srcdir}/samples.sh ${srcdir} 11 d10_50

