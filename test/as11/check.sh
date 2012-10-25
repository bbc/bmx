#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/as11test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as11op1a -o $OUTPUT -y 10:11:12:00 --dm-file as11 ${srcdir}/as11_core_framework.txt --dm-file dpp ${srcdir}/ukdpp_framework.txt --seg ${srcdir}/as11_segmentation_framework.txt "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw
../create_test_essence -t $2 -d $1 /tmp/test_in.raw

# write
$BASE_COMMAND --afd 10 -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < $OUTPUT > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$3$4.md5
then
	RESULT=0
else
	echo "*** ERROR: $3$4 regression"
	RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw $OUTPUT /tmp/test.md5


exit $RESULT
