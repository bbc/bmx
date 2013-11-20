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
	unc_3840.test \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test



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
	unc_3840.test \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
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
	unc_3840.md5s \
	mpeg2lg_422p_hl_1080i.md5s \
	mpeg2lg_mp_hl_1920_1080i.md5s \
	mpeg2lg_mp_h14_1080i.md5s \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 3 2 iecdv25
	${srcdir}/create.sh ${srcdir} 3 3 dvbased25
	${srcdir}/create.sh ${srcdir} 3 4 dv50
	${srcdir}/create.sh ${srcdir} 3 5 dv100_1080i
	${srcdir}/create.sh ${srcdir} 3 6 dv100_720p 25
	${srcdir}/create.sh ${srcdir} 3 6 dv100_720p 50
	${srcdir}/create.sh ${srcdir} 3 7 avci100_1080i
	${srcdir}/create.sh ${srcdir} 3 8 avci100_1080p
	${srcdir}/create.sh ${srcdir} 3 29 avci100_720p 25
	${srcdir}/create.sh ${srcdir} 3 29 avci100_720p 50
	${srcdir}/create.sh ${srcdir} 3 9 avci50_1080i
	${srcdir}/create.sh ${srcdir} 3 10 avci50_1080p
	${srcdir}/create.sh ${srcdir} 3 30 avci50_720p 25
	${srcdir}/create.sh ${srcdir} 3 30 avci50_720p 50
	${srcdir}/create.sh ${srcdir} 3 11 d10_50
	${srcdir}/create.sh ${srcdir} 3 12 d10_40
	${srcdir}/create.sh ${srcdir} 3 13 d10_30
	${srcdir}/create.sh ${srcdir} 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/create.sh ${srcdir} 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/create.sh ${srcdir} 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/create.sh ${srcdir} 3 17 unc
	${srcdir}/create.sh ${srcdir} 3 18 unc_1080i
	${srcdir}/create.sh ${srcdir} 3 19 unc_1080p
	${srcdir}/create.sh ${srcdir} 3 20 unc_720p
	${srcdir}/create.sh ${srcdir} 3 45 unc_3840



.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 3 2 iecdv25
	${srcdir}/samples.sh 3 3 dvbased25
	${srcdir}/samples.sh 3 4 dv50
	${srcdir}/samples.sh 3 5 dv100_1080i
	${srcdir}/samples.sh 3 6 dv100_720p 25
	${srcdir}/samples.sh 3 6 dv100_720p 50
	${srcdir}/samples.sh 3 7 avci100_1080i
	${srcdir}/samples.sh 3 8 avci100_1080p
	${srcdir}/samples.sh 3 29 avci100_720p 25
	${srcdir}/samples.sh 3 29 avci100_720p 50
	${srcdir}/samples.sh 3 9 avci50_1080i
	${srcdir}/samples.sh 3 10 avci50_1080p
	${srcdir}/samples.sh 3 30 avci50_720p 25
	${srcdir}/samples.sh 3 30 avci50_720p 50
	${srcdir}/samples.sh 3 11 d10_50
	${srcdir}/samples.sh 3 12 d10_40
	${srcdir}/samples.sh 3 13 d10_30
	${srcdir}/samples.sh 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/samples.sh 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/samples.sh 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/samples.sh 3 17 unc
	${srcdir}/samples.sh 3 18 unc_1080i
	${srcdir}/samples.sh 3 19 unc_1080p
	${srcdir}/samples.sh 3 20 unc_720p
	${srcdir}/samples.sh 3 45 unc_3840


