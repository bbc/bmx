TESTS = \
	d10_30_25.test \
	d10_40_25.test \
	d10_50_25.test \
	d10_30_30.test \
	d10_40_30.test \
	d10_50_30.test



EXTRA_DIST = \
	d10_30_25.test \
	d10_40_25.test \
	d10_50_25.test \
	d10_30_30.test \
	d10_40_30.test \
	d10_50_30.test \
	d10_30_25.md5 \
	d10_40_25.md5 \
	d10_50_25.md5 \
	d10_30_30.md5 \
	d10_40_30.md5 \
	d10_50_30.md5 \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 11 d10_50 25
	${srcdir}/create.sh ${srcdir} 12 d10_40 25
	${srcdir}/create.sh ${srcdir} 13 d10_30 25
	${srcdir}/create.sh ${srcdir} 11 d10_50 30
	${srcdir}/create.sh ${srcdir} 12 d10_40 30
	${srcdir}/create.sh ${srcdir} 13 d10_30 30


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 11 d10_50 25
	${srcdir}/samples.sh 12 d10_40 25
	${srcdir}/samples.sh 13 d10_30 25
	${srcdir}/samples.sh 11 d10_50 30
	${srcdir}/samples.sh 12 d10_40 30
	${srcdir}/samples.sh 13 d10_30 30

