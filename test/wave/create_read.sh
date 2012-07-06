#!/bin/sh

if command -v md5sum >/dev/null 2>&1; then
  MD5TOOL=md5sum
elif command -v md5 >/dev/null 2>&1; then
  MD5TOOL=md5
else
  echo "ERROR: require md5/md5sum tool"
  exit 1
fi


OUTPUT1=/tmp/test_1_$2.wav
OUTPUT2=/tmp/test_2_$2.wav
WRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT1 -f 25 -y 10:11:12:13 --orig regtest "
REWRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT2 -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d 25 /tmp/pcm.raw


# write, read and calculate md5sum
if $WRITE_BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null
then
  if $REWRITE_BASE_COMMAND --wave $OUTPUT1 >/dev/null
  then
    $MD5TOOL < $OUTPUT2 | sed 's/\([a-f0-9]\)$/\1\ \ -/g' >$1/$2.md5
    RESULT=0
  else
    RESULT=1
  fi
else
  RESULT=1
fi


# clean-up
rm -f /tmp/pcm.raw $OUTPUT1 $OUTPUT2


exit $RESULT

