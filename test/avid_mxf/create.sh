#!/bin/sh

if command -v md5sum >/dev/null 2>&1; then
  MD5TOOL=md5sum
elif command -v md5 >/dev/null 2>&1; then
  MD5TOOL=md5
else
  echo "ERROR: require md5/md5sum tool"
  exit 1
fi


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t avid -o /tmp/avidmxftest -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  BASE_COMMAND="$BASE_COMMAND -f $4 "
fi
if [ "$3" = "unc" ]; then
  BASE_COMMAND="$BASE_COMMAND --height 576 "
fi


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $2 -d 24 /tmp/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < /tmp/avidmxftest_v1.mxf | sed 's/\([a-f0-9]\)$/\1\ \ -/g' > $1/$3$4.md5s
  $MD5TOOL < /tmp/avidmxftest_a1.mxf | sed 's/\([a-f0-9]\)$/\1\ \ -/g' >> $1/$3$4.md5s
  $MD5TOOL < /tmp/avidmxftest_a2.mxf | sed 's/\([a-f0-9]\)$/\1\ \ -/g' >> $1/$3$4.md5s
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw /tmp/avidmxftest


exit $RESULT

