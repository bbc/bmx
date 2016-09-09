TESTS = 	test_mca.sh test_labels.sh


EXTRA_DIST = \
	mca_mono.md5 \
	mca_stereo.md5 \
	mca_singlemca.md5 \
	mca_monorem.md5 \
	mca_stereoprem.md5 \
	mca_stereops2p4.md5 \
	mca_stereopstereopmonorem.md5 \
	mca_reorder_mono.md5 \
	mca_reorder_stereo.md5 \
	mca_mono_silence.md5 \
	mca_stereo_silence.md5 \
	mcalabels_stereo.md5 \
	mcalabels_stereop51.md5 \
	mcalabels_mess.md5 \
	mcalabels_mono.md5 \
	mcalabels_mononochan.md5 \
	mca_mono.xml \
	mca_stereo.xml \
	mca_singlemca.xml \
	mca_monorem.xml \
	mca_stereoprem.xml \
	mca_stereops2p4.xml \
	mca_stereopstereopmonorem.xml \
	mca_reorder_mono.xml \
	mca_reorder_stereo.xml \
	mca_mono_silence.xml \
	mca_stereo_silence.xml \
	mcalabels_stereo.xml \
	mcalabels_stereop51.xml \
	mcalabels_mess.xml \
	mcalabels_mono.xml \
	mcalabels_mononochan.xml \
	stereo.txt \
	stereop51.txt \
	mess.txt \
	mono.txt \
	mononochan.txt \
	test_mca.sh \
	test_labels.sh


.PHONY: create-data
create-data:
	${srcdir}/test_mca.sh create_data
	${srcdir}/test_labels.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test_mca.sh create_samples
	${srcdir}/test_labels.sh create_samples

