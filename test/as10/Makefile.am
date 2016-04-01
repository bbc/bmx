TESTS = test.sh


EXTRA_DIST = \
	high_hd_2014_rb.md5 \
	high_hd_2014_rb.xml \
	high_hd_2014_tw.md5 \
	high_hd_2014_tw.xml \
	test.sh \
	as10_core_framework.txt


.PHONY: create-data
create-data:
	${srcdir}/test.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test.sh create_samples

