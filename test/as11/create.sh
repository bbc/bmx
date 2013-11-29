#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/as11test_temp$$

mkdir -p ${TEMP_DIR}


OUTPUT=/tmp/as11test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as11op1a -o ${TEMP_DIR}/as11test.mxf -y 09:58:00:00 --dm-file as11 $1/as11_core_framework.txt --dm-file dpp $1/ukdpp_framework.txt --seg $1/as11_segmentation_framework.txt "
if [ "$5" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $5 "
fi


# create essence data
../create_test_essence -t 42 -d $2 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $3 -d $2 ${TEMP_DIR}/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND --afd 10 -a 16:9 --$4 ${TEMP_DIR}/test_in.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 24 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null
then
  $MD5TOOL < ${TEMP_DIR}/as11test.mxf > $1/$4$5.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

