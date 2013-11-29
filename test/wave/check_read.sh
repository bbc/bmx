#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/wavereadtest_temp$$
OUTPUT1=${TEMP_DIR}/test_1.wav
OUTPUT2=${TEMP_DIR}/test_2.wav

mkdir -p ${TEMP_DIR}


WRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT1 -f 25 -y 10:11:12:13 --orig regtest "
REWRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT2 -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw

# write
$WRITE_BASE_COMMAND -q 16 --pcm ${TEMP_DIR}/pcm.raw -q 16 --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# re-write
$REWRITE_BASE_COMMAND --wave $OUTPUT1 >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT2 > ${TEMP_DIR}/test.md5
if diff ${TEMP_DIR}/test.md5 ${srcdir}/$2.md5
then
	RESULT=0
else
	echo "*** ERROR: $2 regression"
	RESULT=1
fi

# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

