#!/bin/sh

BASE_COMMAND="../../apps/raw2as11/raw2as11 --regtest -y 10:11:12:00 --dm-file as11 $1/as11_core_framework.txt --dm-file dpp $1/ukdpp_framework.txt --seg $1/as11_segmentation_framework.txt "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi
OUTPUT=/tmp/as11_$3$4.mxf


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $2 -d 24 /tmp/test_in.raw

# write
$BASE_COMMAND --afd 10 -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw $OUTPUT >/dev/null

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

