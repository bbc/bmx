#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/d10test_temp$$

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t d10 -o ${TEMP_DIR}/d10test.mxf -y 10:11:12:13 --clip test -f $5 "


# create essence data
../create_test_essence -t 1 -d $2 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $3 -d $2 ${TEMP_DIR}/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND -a 16:9 --$4 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null
then
  $MD5TOOL < ${TEMP_DIR}/d10test.mxf > $1/$4_$5.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

