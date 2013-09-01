TESTS = \
	d10_30.test \
	d10_40.test \
	d10_50.test \
	dv50.test \
	dv100_1080i.test \
	dv100_720p25.test \
	dvbased25.test \
	iecdv25.test \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
	avci100_1080i.test \
	avci100_1080i_gf.test \
	avci100_1080i_gfp.test \
	avci100_1080p.test \
	avci100_720p25.test \
	avci100_720p50.test \
	avci50_1080i.test \
	avci50_1080p.test \
	avci50_720p25.test \
	avci50_720p50.test \
	unc25.test \
	unc_1080i25.test \
	unc_1080p25.test \
	unc_720p50.test \
	vc3_1080p_1235.test \
	vc3_1080p_1237.test \
	vc3_1080p_1238.test \
	vc3_1080i_1241.test \
	vc3_1080i_1242.test \
	vc3_1080i_1243.test \
	vc3_720p_1250.test \
	vc3_720p_1251.test \
	vc3_720p_1252.test \
	vc3_1080p_1253.test \
	avid_alpha_1080i25.test


EXTRA_DIST = \
	d10_30.test \
	d10_40.test \
	d10_50.test \
	dv100_1080i.test \
	dv100_720p25.test \
	dv50.test \
	dvbased25.test \
	iecdv25.test \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
	avci100_1080i.test \
	avci100_1080i_gf.test \
	avci100_1080i_gfp.test \
	avci100_1080p.test \
	avci100_720p25.test \
	avci100_720p50.test \
	avci50_1080i.test \
	avci50_1080p.test \
	avci50_720p25.test \
	avci50_720p50.test \
	unc25.test \
	unc_1080i25.test \
	unc_1080p25.test \
	unc_720p50.test \
	vc3_1080p_1235.test \
	vc3_1080p_1237.test \
	vc3_1080p_1238.test \
	vc3_1080i_1241.test \
	vc3_1080i_1242.test \
	vc3_1080i_1243.test \
	vc3_720p_1250.test \
	vc3_720p_1251.test \
	vc3_720p_1252.test \
	vc3_1080p_1253.test \
	avid_alpha_1080i25.test \
	d10_30.md5s \
	d10_40.md5s \
	d10_50.md5s \
	dv100_1080i.md5s \
	dv100_720p25.md5s \
	dv50.md5s \
	dvbased25.md5s \
	iecdv25.md5s \
	mpeg2lg_422p_hl_1080i.md5s \
	mpeg2lg_mp_hl_1920_1080i.md5s \
	mpeg2lg_mp_h14_1080i.md5s \
	avci100_1080i.md5s \
	avci100_1080i_gf.md5s \
	avci100_1080i_gfp.md5s \
	avci100_1080p.md5s \
	avci100_720p25.md5s \
	avci100_720p50.md5s \
	avci50_1080i.md5s \
	avci50_1080p.md5s \
	avci50_720p25.md5s \
	avci50_720p50.md5s \
	unc25.md5s \
	unc_1080i25.md5s \
	unc_1080p25.md5s \
	unc_720p50.md5s \
	vc3_1080p_1235.md5s \
	vc3_1080p_1237.md5s \
	vc3_1080p_1238.md5s \
	vc3_1080i_1241.md5s \
	vc3_1080i_1242.md5s \
	vc3_1080i_1243.md5s \
	vc3_720p_1250.md5s \
	vc3_720p_1251.md5s \
	vc3_720p_1252.md5s \
	vc3_1080p_1253.md5s \
	avid_alpha_1080i25.md5s \
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
	${srcdir}/create.sh ${srcdir} 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/create.sh ${srcdir} 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/create.sh ${srcdir} 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/create.sh ${srcdir} 3 11 d10_50
	${srcdir}/create.sh ${srcdir} 3 12 d10_40
	${srcdir}/create.sh ${srcdir} 3 13 d10_30
	${srcdir}/create.sh ${srcdir} 3 7 avci100_1080i
	${srcdir}/create.sh ${srcdir} 3 7 avci100_1080i gf
	${srcdir}/create.sh ${srcdir} 3 7 avci100_1080i gfp 2
	${srcdir}/create.sh ${srcdir} 3 8 avci100_1080p
	${srcdir}/create.sh ${srcdir} 3 29 avci100_720p 25
	${srcdir}/create.sh ${srcdir} 3 29 avci100_720p 50
	${srcdir}/create.sh ${srcdir} 3 9 avci50_1080i
	${srcdir}/create.sh ${srcdir} 3 10 avci50_1080p
	${srcdir}/create.sh ${srcdir} 3 30 avci50_720p 25
	${srcdir}/create.sh ${srcdir} 3 30 avci50_720p 50
	${srcdir}/create.sh ${srcdir} 3 17 unc 25
	${srcdir}/create.sh ${srcdir} 3 18 unc_1080i 25
	${srcdir}/create.sh ${srcdir} 3 19 unc_1080p 25
	${srcdir}/create.sh ${srcdir} 3 20 unc_720p 50
	${srcdir}/create.sh ${srcdir} 3 31 vc3_1080p_1235
	${srcdir}/create.sh ${srcdir} 3 32 vc3_1080p_1237
	${srcdir}/create.sh ${srcdir} 3 33 vc3_1080p_1238
	${srcdir}/create.sh ${srcdir} 3 34 vc3_1080i_1241
	${srcdir}/create.sh ${srcdir} 3 35 vc3_1080i_1242
	${srcdir}/create.sh ${srcdir} 3 36 vc3_1080i_1243
	${srcdir}/create.sh ${srcdir} 3 37 vc3_720p_1250
	${srcdir}/create.sh ${srcdir} 3 38 vc3_720p_1251
	${srcdir}/create.sh ${srcdir} 3 39 vc3_720p_1252
	${srcdir}/create.sh ${srcdir} 3 40 vc3_1080p_1253
	${srcdir}/create.sh ${srcdir} 3 41 avid_alpha_1080i 25


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 3 2 iecdv25
	${srcdir}/samples.sh 3 3 dvbased25
	${srcdir}/samples.sh 3 4 dv50
	${srcdir}/samples.sh 3 5 dv100_1080i
	${srcdir}/samples.sh 3 6 dv100_720p 25
	${srcdir}/samples.sh 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/samples.sh 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/samples.sh 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/samples.sh 3 11 d10_50
	${srcdir}/samples.sh 3 12 d10_40
	${srcdir}/samples.sh 3 13 d10_30
	${srcdir}/samples.sh 3 7 avci100_1080i
	${srcdir}/samples.sh 3 7 avci100_1080i gf
	${srcdir}/samples.sh 3 7 avci100_1080i gfp 2
	${srcdir}/samples.sh 3 8 avci100_1080p
	${srcdir}/samples.sh 3 29 avci100_720p 25
	${srcdir}/samples.sh 3 29 avci100_720p 50
	${srcdir}/samples.sh 3 9 avci50_1080i
	${srcdir}/samples.sh 3 10 avci50_1080p
	${srcdir}/samples.sh 3 30 avci50_720p 25
	${srcdir}/samples.sh 3 30 avci50_720p 50
	${srcdir}/samples.sh 3 17 unc 25
	${srcdir}/samples.sh 3 18 unc_1080i 25
	${srcdir}/samples.sh 3 19 unc_1080p 25
	${srcdir}/samples.sh 3 20 unc_720p 50
	${srcdir}/samples.sh 3 31 vc3_1080p_1235
	${srcdir}/samples.sh 3 32 vc3_1080p_1237
	${srcdir}/samples.sh 3 33 vc3_1080p_1238
	${srcdir}/samples.sh 3 34 vc3_1080i_1241
	${srcdir}/samples.sh 3 35 vc3_1080i_1242
	${srcdir}/samples.sh 3 36 vc3_1080i_1243
	${srcdir}/samples.sh 3 37 vc3_720p_1250
	${srcdir}/samples.sh 3 38 vc3_720p_1251
	${srcdir}/samples.sh 3 39 vc3_720p_1252
	${srcdir}/samples.sh 3 40 vc3_1080p_1253
	${srcdir}/samples.sh 3 41 avid_alpha_1080i 25

