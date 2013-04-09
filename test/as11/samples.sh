#!/bin/sh

OUTPUT=/tmp/as11_$4$5.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as11op1a -o $OUTPUT -y 09:58:00:00 --dm-file as11 $1/as11_core_framework.txt --dm-file dpp $1/ukdpp_framework.txt --seg $1/as11_segmentation_framework.txt "
if [ "$5" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $5 "
fi


# create essence data
../create_test_essence -t 42 -d $2 /tmp/pcm.raw
../create_test_essence -t $3 -d $2 /tmp/test_in.raw

# write
$BASE_COMMAND --afd 10 -a 16:9 --$4 /tmp/test_in.raw -q 24 --locked true --pcm /tmp/pcm.raw -q 24 --locked true --pcm /tmp/pcm.raw -q 24 --locked true --pcm /tmp/pcm.raw -q 24 --locked true --pcm /tmp/pcm.raw >/dev/null

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

