TESTS = \
	d10_30_25.test \
	d10_40_25.test \
	d10_50_25.test \
	d10_30_2997.test \
	d10_40_2997.test \
	d10_50_2997.test



EXTRA_DIST = \
	d10_30_25.test \
	d10_40_25.test \
	d10_50_25.test \
	d10_30_2997.test \
	d10_40_2997.test \
	d10_50_2997.test \
	d10_30_25.md5 \
	d10_40_25.md5 \
	d10_50_25.md5 \
	d10_30_2997.md5 \
	d10_40_2997.md5 \
	d10_50_2997.md5 \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 3 11 d10_50 25
	${srcdir}/create.sh ${srcdir} 3 12 d10_40 25
	${srcdir}/create.sh ${srcdir} 3 13 d10_30 25
	${srcdir}/create.sh ${srcdir} 3 11 d10_50 2997
	${srcdir}/create.sh ${srcdir} 3 12 d10_40 2997
	${srcdir}/create.sh ${srcdir} 3 13 d10_30 2997


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 3 11 d10_50 25
	${srcdir}/samples.sh 3 12 d10_40 25
	${srcdir}/samples.sh 3 13 d10_30 25
	${srcdir}/samples.sh 3 11 d10_50 2997
	${srcdir}/samples.sh 3 12 d10_40 2997
	${srcdir}/samples.sh 3 13 d10_30 2997

