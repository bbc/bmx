#!/bin/sh

MD5TOOL=../file_md5


OUTPUT=/tmp/rdd9test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t rdd9 -o $OUTPUT -y 10:11:12:13 --clip test --part 12 "
if [ "$5" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $5 "
fi


# create essence data
../create_test_essence -t 1 -d $2 /tmp/pcm.raw
../create_test_essence -t $3 -d $2 /tmp/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND -a 16:9 --$4 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < $OUTPUT > $1/$4$5.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw $OUTPUT


exit $RESULT

