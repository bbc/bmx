#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/test_$2.wav
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d 25 /tmp/pcm.raw

# write and calculate md5sum
if $BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < $OUTPUT > $1/$2.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -f /tmp/pcm.raw $OUTPUT


exit $RESULT

