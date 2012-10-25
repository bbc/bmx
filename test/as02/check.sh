#!/bin/sh

MD5TOOL=../file_md5


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o /tmp/as02test -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw
../create_test_essence -t $2 -d $1 /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < /tmp/as02test/as02test.mxf > /tmp/test.md5s
$MD5TOOL < /tmp/as02test/media/as02test_v0.mxf >> /tmp/test.md5s
$MD5TOOL < /tmp/as02test/media/as02test_a0.mxf >> /tmp/test.md5s
$MD5TOOL < /tmp/as02test/media/as02test_a1.mxf >> /tmp/test.md5s
if diff /tmp/test.md5s ${srcdir}/$3$4.md5s
then
	RESULT=0
else
	echo "*** ERROR: $3$4 regression" 
	RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw /tmp/as02test /tmp/test.md5s


exit $RESULT
