TESTS = \
	wave_write.test \
	wave_read.test \
	sound_only.sh


EXTRA_DIST = \
	wave_write.test \
	wave_read.test \
	wave_write.md5 \
	wave_read.md5 \
	wave_sound_only_from_raw.md5 \
	wave_sound_only_transwrap.md5 \
	check_write.sh \
	check_read.sh \
	create_write.sh \
	create_read.sh \
	samples.sh \
	sound_only.sh



.PHONY: create-data
create-data:
	${srcdir}/create_write.sh ${srcdir} 25 wave_write
	${srcdir}/create_read.sh ${srcdir} 25 wave_read
	${srcdir}/sound_only.sh create-data


.PHONY: create-samples
create-samples:
	${srcdir}/samples.sh 25 wave
	${srcdir}/sound_only.sh create-samples

