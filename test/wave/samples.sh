#!/bin/sh

BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o /tmp/sample_$2.wav -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d 25 /tmp/pcm.raw

# write
$BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null

# clean-up
rm /tmp/pcm.raw

