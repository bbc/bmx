#!/bin/sh

TEMP_DIR=/tmp/as02_temp$$
OUTPUT=/tmp/sample_$3$4

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o ${OUTPUT} -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
else
  BASE_COMMAND="$BASE_COMMAND -f 25 "
fi


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $2 -d $1 ${TEMP_DIR}/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw

# clean-up
rm -Rf ${TEMP_DIR}

