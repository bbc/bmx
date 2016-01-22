TESTS = test.sh \
	test_mca.sh


EXTRA_DIST = \
	avci100_1080i_op1a_op1a.md5 \
	d10_50_d10_op1a.md5 \
	d10_50_op1a_d10.md5 \
	mca_mono.md5 \
	mca_stereo.md5 \
	mca_singlemca.md5 \
	mca_monorem.md5 \
	mca_stereoprem.md5 \
	mca_stereops2p4.md5 \
	mca_stereopstereopmonorem.md5 \
	mca_reorder_mono.md5 \
	mca_reorder_stereo.md5 \
	test.sh \
	test_mca.sh


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data
	${srcdir}/test_mca.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_sample
	${srcdir}/test_mca.sh create_sample

