#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/readertest_temp$$

mkdir -p ${TEMP_DIR}


AS02_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o ${TEMP_DIR}/as02test -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f $4 "
else
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f 25 "
fi
READ_COMMAND="../../apps/mxf2raw/mxf2raw --regtest --info --info-format xml --track-chksum md5 ${TEMP_DIR}/as02test/as02test.mxf"

# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$AS02_BASE_COMMAND -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# read
$READ_COMMAND | sed "s:${TEMP_DIR}:/tmp:g" >${TEMP_DIR}/mxfreadertest_stdout

# calculate md5sum and compare with expected value
$MD5TOOL < ${TEMP_DIR}/mxfreadertest_stdout > ${TEMP_DIR}/test.md5
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
