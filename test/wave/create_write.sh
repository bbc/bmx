#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/wavewritetest_temp$$
OUTPUT=${TEMP_DIR}/test_1.wav

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d $2 ${TEMP_DIR}/pcm.raw

# write and calculate md5sum
if $BASE_COMMAND -q 16 --pcm ${TEMP_DIR}/pcm.raw -q 16 --pcm ${TEMP_DIR}/pcm.raw >/dev/null
then
  $MD5TOOL < $OUTPUT > $1/$3.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

