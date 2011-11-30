#!/bin/sh

OUTPUT=/tmp/d10mxf_$2_$3.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t d10 -o $OUTPUT -y 10:11:12:13 --clip test -f $3 "


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $1 -d 24 /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$2 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

