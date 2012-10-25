TESTS = wave_write.test wave_read.test


EXTRA_DIST = \
	wave_write.test \
	wave_read.test \
	wave_write.md5 \
	wave_read.md5 \
	check_write.sh \
	check_read.sh \
	create_write.sh \
	create_read.sh \
	samples.sh



.PHONY: create-data
create-data:
	${srcdir}/create_write.sh ${srcdir} 25 wave_write
	${srcdir}/create_read.sh ${srcdir} 25 wave_read


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 25 wave

