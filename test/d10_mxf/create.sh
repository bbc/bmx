#!/bin/sh

if command -v md5sum >/dev/null 2>&1; then
  MD5TOOL=md5sum
elif command -v md5 >/dev/null 2>&1; then
  MD5TOOL=md5
else
  echo "ERROR: require md5/md5sum tool"
  exit 1
fi


OUTPUT=/tmp/d10test.mxf
BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t d10 -o $OUTPUT -y 10:11:12:13 --clip test -f $4 "


# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $2 -d 24 /tmp/test_in.raw

# write and calculate md5sum
if $BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null
then
  $MD5TOOL < $OUTPUT | sed 's/\([a-f0-9]\)$/\1\ \ -/g' > $1/$3_$4.md5
  RESULT=0
else
  RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw $OUTPUT


exit $RESULT

