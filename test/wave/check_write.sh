#!/bin/sh

if command -v md5sum >/dev/null 2>&1; then
  MD5TOOL=md5sum
elif command -v md5 >/dev/null 2>&1; then
  MD5TOOL=md5
else
  echo "ERROR: require md5/md5sum tool"
  exit 1
fi


OUTPUT=/tmp/test_1.wav
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d 25 /tmp/pcm.raw

# write
$BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT | sed 's/\([a-f0-9]\)$/\1\ \ -/g' > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$1.md5
then
	RESULT=0
else
	echo "*** ERROR: $1 regression"
	RESULT=1
fi

# clean-up
rm -f /tmp/pcm.raw $OUTPUT /tmp/test.md5


exit $RESULT
