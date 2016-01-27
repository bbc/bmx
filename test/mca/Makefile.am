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
	mcalabels_stereo.md5 \
	mcalabels_stereop51.md5 \
	mcalabels_mess.md5 \
	stereo.txt \
	stereop51.txt \
	mess.txt \
	test_mca.sh \
	test_labels.sh


.PHONY: create-data
create-data:
	${srcdir}/test_mca.sh create_data
	${srcdir}/test_labels.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test_mca.sh create_sample
	${srcdir}/test_labels.sh create_sample

