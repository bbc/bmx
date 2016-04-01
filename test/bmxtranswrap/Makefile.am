TESTS = test.sh


EXTRA_DIST = \
	avci100_1080i_op1a_op1a.md5 \
	d10_50_d10_op1a.md5 \
	d10_50_op1a_d10.md5 \
	test.sh


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples

