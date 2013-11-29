#!/bin/sh

TEMP_DIR=/tmp/op1a_temp$$
OUTPUT=/tmp/op1a_$3$4.mxf

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t op1a -o $OUTPUT -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# clean-up
rm -Rf ${TEMP_DIR}

