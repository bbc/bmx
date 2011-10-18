#!/bin/sh

BASE_COMMAND="../../apps/raw2mxfop1a/raw2mxfop1a --regtest -y 10:11:12:13 --clip test "
if [ "$3" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $3 "
fi
OUTPUT=/tmp/op1a_$2$3.mxf


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $1 -d 24 /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$2 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw $OUTPUT >/dev/null

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

