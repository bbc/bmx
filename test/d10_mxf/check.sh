#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/d10test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t d10 -o $OUTPUT -y 10:11:12:13 --clip test -f $4 "


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw
../create_test_essence -t $2 -d $1 /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$3_$4.md5
then
	RESULT=0
else
	echo "*** ERROR: $3_$4 regression"
	RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw $OUTPUT /tmp/test.md5


exit $RESULT
