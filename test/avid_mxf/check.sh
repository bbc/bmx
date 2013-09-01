#!/bin/sh

MD5TOOL=../file_md5

DURATION=$1
CREATE_TYPE=$2
INPUT_TYPE=$3
shift 3
EXTRA_OPTS=
MD5S_NAME=$INPUT_TYPE
if [ "$1" = "gf" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --avid-gf "
  MD5S_NAME=$MD5S_NAME"_gf"
  shift
elif [ "$1" = "gfp" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --regtest-end $2 "
  MD5S_NAME=$MD5S_NAME"_gfp"
  shift 2
elif [ "$1" != "" ]; then
  EXTRA_OPTS="$EXTRA_OPTS -f $1 "
  MD5S_NAME=$MD5S_NAME$1
  shift
fi
if [ "$INPUT_TYPE" = "unc" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --height 576 "
fi



BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t avid -o /tmp/avidmxftest -y 10:11:12:13 --clip test --tape testtape $EXTRA_OPTS"


# create essence data
../create_test_essence -t 1 -d $DURATION /tmp/pcm.raw
../create_test_essence -t $CREATE_TYPE -d $DURATION /tmp/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$INPUT_TYPE /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# calculate md5sum and compare with expected value
$MD5TOOL < /tmp/avidmxftest_v1.mxf > /tmp/test.md5s
$MD5TOOL < /tmp/avidmxftest_a1.mxf >> /tmp/test.md5s
$MD5TOOL < /tmp/avidmxftest_a2.mxf >> /tmp/test.md5s
if diff /tmp/test.md5s ${srcdir}/$MD5S_NAME.md5s
then
	RESULT=0
else
	echo "*** ERROR: $MD5S_NAME regression"
	RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw /tmp/avidmxftest_v1.mxf /tmp/avidmxftest_a1.mxf /tmp/avidmxftest_a2.mxf /tmp/test.md5s


exit $RESULT
