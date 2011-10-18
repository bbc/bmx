TESTS = \
	avci50_1080i.test \
	avci50_1080p.test \
	avci50_720p25.test \
	avci50_720p50.test \
	avci100_1080i.test \
	avci100_1080p.test \
	avci100_720p25.test \
	avci100_720p50.test \
	d10_30.test \
	d10_40.test \
	d10_50.test \
	dv50.test \
	dv100_1080i.test \
	dv100_720p25.test \
	dv100_720p50.test \
	dvbased25.test \
	iecdv25.test \
	unc.test \
	unc_720p.test \
	unc_1080i.test \
	unc_1080p.test \
	mpeg2lg_422p_hl.test \
	mpeg2lg_mp_hl.test \
	mpeg2lg_mp_h14.test



EXTRA_DIST = \
	avci100_1080i.test \
	avci100_1080p.test \
	avci100_720p25.test \
	avci100_720p50.test \
	avci50_1080i.test \
	avci50_1080p.test \
	avci50_720p25.test \
	avci50_720p50.test \
	d10_30.test \
	d10_40.test \
	d10_50.test \
	dv100_1080i.test \
	dv100_720p25.test \
	dv100_720p50.test \
	dv50.test \
	dvbased25.test \
	iecdv25.test \
	unc_1080i.test \
	unc_1080p.test \
	unc_720p.test \
	unc.test \
	mpeg2lg_422p_hl.test \
	mpeg2lg_mp_hl.test \
	mpeg2lg_mp_h14.test \
	avci100_1080i.md5s \
	avci100_1080p.md5s \
	avci100_720p25.md5s \
	avci100_720p50.md5s \
	avci50_1080i.md5s \
	avci50_1080p.md5s \
	avci50_720p25.md5s \
	avci50_720p50.md5s \
	d10_30.md5s \
	d10_40.md5s \
	d10_50.md5s \
	dv100_1080i.md5s \
	dv100_720p25.md5s \
	dv100_720p50.md5s \
	dv50.md5s \
	dvbased25.md5s \
	iecdv25.md5s \
	unc_1080i.md5s \
	unc_1080p.md5s \
	unc_720p.md5s \
	unc.md5s \
	mpeg2lg_422p_hl.md5s \
	mpeg2lg_mp_hl.md5s \
	mpeg2lg_mp_h14.md5s \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 2 iecdv25
	${srcdir}/create.sh ${srcdir} 3 dvbased25
	${srcdir}/create.sh ${srcdir} 4 dv50
	${srcdir}/create.sh ${srcdir} 5 dv100_1080i
	${srcdir}/create.sh ${srcdir} 6 dv100_720p 25
	${srcdir}/create.sh ${srcdir} 6 dv100_720p 50
	${srcdir}/create.sh ${srcdir} 7 avci100_1080i
	${srcdir}/create.sh ${srcdir} 8 avci100_1080p
	${srcdir}/create.sh ${srcdir} 29 avci100_720p 25
	${srcdir}/create.sh ${srcdir} 29 avci100_720p 50
	${srcdir}/create.sh ${srcdir} 9 avci50_1080i
	${srcdir}/create.sh ${srcdir} 10 avci50_1080p
	${srcdir}/create.sh ${srcdir} 30 avci50_720p 25
	${srcdir}/create.sh ${srcdir} 30 avci50_720p 50
	${srcdir}/create.sh ${srcdir} 11 d10_50
	${srcdir}/create.sh ${srcdir} 12 d10_40
	${srcdir}/create.sh ${srcdir} 13 d10_30
	${srcdir}/create.sh ${srcdir} 14 mpeg2lg_422p_hl
	${srcdir}/create.sh ${srcdir} 15 mpeg2lg_mp_h14
	${srcdir}/create.sh ${srcdir} 16 mpeg2lg_mp_hl
	${srcdir}/create.sh ${srcdir} 17 unc
	${srcdir}/create.sh ${srcdir} 18 unc_1080i
	${srcdir}/create.sh ${srcdir} 19 unc_1080p
	${srcdir}/create.sh ${srcdir} 20 unc_720p



.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 2 iecdv25
	${srcdir}/samples.sh 3 dvbased25
	${srcdir}/samples.sh 4 dv50
	${srcdir}/samples.sh 5 dv100_1080i
	${srcdir}/samples.sh 6 dv100_720p 25
	${srcdir}/samples.sh 6 dv100_720p 50
	${srcdir}/samples.sh 7 avci100_1080i
	${srcdir}/samples.sh 8 avci100_1080p
	${srcdir}/samples.sh 29 avci100_720p 25
	${srcdir}/samples.sh 29 avci100_720p 50
	${srcdir}/samples.sh 9 avci50_1080i
	${srcdir}/samples.sh 10 avci50_1080p
	${srcdir}/samples.sh 30 avci50_720p 25
	${srcdir}/samples.sh 30 avci50_720p 50
	${srcdir}/samples.sh 11 d10_50
	${srcdir}/samples.sh 12 d10_40
	${srcdir}/samples.sh 13 d10_30
	${srcdir}/samples.sh 14 mpeg2lg_422p_hl
	${srcdir}/samples.sh 15 mpeg2lg_mp_h14
	${srcdir}/samples.sh 16 mpeg2lg_mp_hl
	${srcdir}/samples.sh 17 unc
	${srcdir}/samples.sh 18 unc_1080i
	${srcdir}/samples.sh 19 unc_1080p
	${srcdir}/samples.sh 20 unc_720p


