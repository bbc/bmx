#!/bin/sh

MD5TOOL=../file_md5


OUTPUT1=/tmp/test_1.wav
OUTPUT2=/tmp/test_2.wav
WRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT1 -f 25 -y 10:11:12:13 --orig regtest "
REWRITE_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o $OUTPUT2 -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d 25 /tmp/pcm.raw

# write
$WRITE_BASE_COMMAND -q 16 --pcm /tmp/pcm.raw -q 16 --pcm /tmp/pcm.raw >/dev/null

# re-write
$REWRITE_BASE_COMMAND --wave $OUTPUT1 >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT2 > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$1.md5
then
	RESULT=0
else
	echo "*** ERROR: $1 regression"
	RESULT=1
fi

# clean-up
rm -f /tmp/pcm.raw $OUTPUT1 $OUTPUT2 /tmp/test.md5


exit $RESULT

