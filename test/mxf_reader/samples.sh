#!/bin/sh

TEMP_DIR=/tmp/reader_temp$$
OUTPUT=/tmp/sample_$3$4
OUTPUT_STDOUT=/tmp/sample_stdout_$3$4

mkdir -p ${TEMP_DIR}


AS02_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 --mic-type md5 -o ${OUTPUT} -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f $4 "
else
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f 25 "
fi
READ_COMMAND="../../apps/mxf2raw/mxf2raw --regtest --info --info-format xml --track-chksum md5 ${OUTPUT}/sample_$3$4.mxf"


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$AS02_BASE_COMMAND -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw

# read
$READ_COMMAND | sed "s:${TEMP_DIR}:/tmp:g" >${OUTPUT_STDOUT}

# clean-up
rm -Rf ${TEMP_DIR}

