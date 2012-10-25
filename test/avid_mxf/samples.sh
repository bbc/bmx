#!/bin/sh

BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t avid -o /tmp/avidmxfsample_$3$4 -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi
if [ "$3" = "unc" ]; then
  BASE_COMMAND="$BASE_COMMAND --height 576 "
fi


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw
../create_test_essence -t $2 -d $1 /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

