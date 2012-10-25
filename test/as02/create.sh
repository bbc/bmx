#!/bin/sh

MD5TOOL=../file_md5


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o /tmp/as02test -y 10:11:12:13 --clip test "
if [ "$5" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $5 "
else
  BASE_COMMAND="$BASE_COMMAND -f 25 "
fi


# create essence data
../create_test_essence -t 1 -d $2 /tmp/pcm.raw
../create_test_essence -t $3 -d $2 /tmp/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND -a 16:9 --$4 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < /tmp/as02test/as02test.mxf > $1/$4$5.md5s
  $MD5TOOL < /tmp/as02test/media/as02test_v0.mxf >> $1/$4$5.md5s
  $MD5TOOL < /tmp/as02test/media/as02test_a0.mxf >> $1/$4$5.md5s
  $MD5TOOL < /tmp/as02test/media/as02test_a1.mxf >> $1/$4$5.md5s
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw /tmp/as02test


exit $RESULT

