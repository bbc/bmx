#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/wavereadtest_temp$$
OUTPUT1=${TEMP_DIR}/test_1_$3.wav
OUTPUT2=${TEMP_DIR}/test_2_$3.wav

mkdir -p ${TEMP_DIR}


WRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT1 -f 25 -y 10:11:12:13 --orig regtest "
REWRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT2 -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d $2 ${TEMP_DIR}/pcm.raw


# write, read and calculate md5sum
if $WRITE_BASE_COMMAND -q 16 --pcm ${TEMP_DIR}/pcm.raw -q 16 --pcm ${TEMP_DIR}/pcm.raw >/dev/null
then
  if $REWRITE_BASE_COMMAND --wave $OUTPUT1 >/dev/null
  then
    $MD5TOOL < $OUTPUT2 >$1/$3.md5
    RESULT=0
  else
    RESULT=1
  fi
else
  RESULT=1
fi


# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

