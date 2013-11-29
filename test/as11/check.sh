#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/as11test_temp$$

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as11op1a -o ${TEMP_DIR}/as11test.mxf -y 09:58:00:00 --dm-file as11 ${srcdir}/as11_core_framework.txt --dm-file dpp ${srcdir}/ukdpp_framework.txt --seg ${srcdir}/as11_segmentation_framework.txt "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 42 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$BASE_COMMAND --afd 10 -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < ${TEMP_DIR}/as11test.mxf > ${TEMP_DIR}/test.md5
if diff ${TEMP_DIR}/test.md5 ${srcdir}/$3$4.md5
then
	RESULT=0
else
	echo "*** ERROR: $3$4 regression"
	RESULT=1
fi

# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT
