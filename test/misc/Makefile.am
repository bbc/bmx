TESTS =	test_desc_props.sh


EXTRA_DIST = \
	desc_props_raw2bmx.md5 \
	desc_props_bmxtranswrap.md5 \
	test_desc_props.sh


.PHONY: create-data
create-data:
	${srcdir}/test_desc_props.sh create_data


.PHONY: create-samples
create-samples:
	${srcdir}/test_desc_props.sh create_sample

