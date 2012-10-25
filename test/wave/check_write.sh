#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/test_1.wav
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw

# write
$BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$2.md5
then
	RESULT=0
else
	echo "*** ERROR: $2 regression"
	RESULT=1
fi

# clean-up
rm -f /tmp/pcm.raw $OUTPUT /tmp/test.md5


exit $RESULT
