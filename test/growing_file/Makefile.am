TESTS = growing_file.sh


EXTRA_DIST = growing_file.sh growing_file.md5


.PHONY: create-data
create-data:
	${srcdir}/growing_file.sh create_data

.PHONY: create-samples
create-samples:
	${srcdir}/growing_file.sh create_samples sample.txt

