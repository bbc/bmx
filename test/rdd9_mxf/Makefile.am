TESTS = \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
	anc.sh \
	vbi.sh


EXTRA_DIST = \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
	mpeg2lg_422p_hl_1080i.md5 \
	mpeg2lg_mp_hl_1920_1080i.md5 \
	mpeg2lg_mp_h14_1080i.md5 \
	anc.sh \
	vbi.sh \
	anc.md5 \
	vbi.md5 \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/create.sh ${srcdir} 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/create.sh ${srcdir} 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/anc.sh create_data
	${srcdir}/vbi.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/samples.sh 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/samples.sh 24 16 mpeg2lg_mp_hl_1920_1080i
	${srcdir}/anc.sh create_sample
	${srcdir}/vbi.sh create_sample

