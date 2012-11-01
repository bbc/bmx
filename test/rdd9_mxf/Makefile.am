TESTS = \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test


EXTRA_DIST = \
	mpeg2lg_422p_hl_1080i.test \
	mpeg2lg_mp_hl_1920_1080i.test \
	mpeg2lg_mp_h14_1080i.test \
	mpeg2lg_422p_hl_1080i.md5 \
	mpeg2lg_mp_hl_1920_1080i.md5 \
	mpeg2lg_mp_h14_1080i.md5 \
	check.sh \
	create.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create.sh ${srcdir} 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/create.sh ${srcdir} 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/create.sh ${srcdir} 24 16 mpeg2lg_mp_hl_1920_1080i


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 24 14 mpeg2lg_422p_hl_1080i
	${srcdir}/samples.sh 24 15 mpeg2lg_mp_h14_1080i
	${srcdir}/samples.sh 24 16 mpeg2lg_mp_hl_1920_1080i

