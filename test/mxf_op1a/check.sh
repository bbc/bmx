#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/op1atest_temp$$

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t op1a -o ${TEMP_DIR}/op1atest.mxf -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < ${TEMP_DIR}/op1atest.mxf > ${TEMP_DIR}/test.md5
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
