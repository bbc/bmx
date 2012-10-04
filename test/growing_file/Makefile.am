TESTS = growing_file.sh


EXTRA_DIST = growing_file.sh growing_file.md5


.PHONY: create-data
create-data:
	${srcdir}/growing_file.sh create_data

.PHONY: create-sample
create-sample:
	${srcdir}/growing_file.sh create_sample sample.txt

