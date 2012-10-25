#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/as11test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as11op1a -o $OUTPUT -y 10:11:12:00 --dm-file as11 $1/as11_core_framework.txt --dm-file dpp $1/ukdpp_framework.txt --seg $1/as11_segmentation_framework.txt "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $2 -d 24 /tmp/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND --afd 10 -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < $OUTPUT > $1/$3$4.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw $OUTPUT


exit $RESULT

