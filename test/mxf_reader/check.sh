#!/bin/sh

if command -v md5sum >/dev/null 2>&1; then
  MD5TOOL=md5sum
elif command -v md5 >/dev/null 2>&1; then
  MD5TOOL=md5
else
  echo "ERROR: require md5/md5sum tool"
  exit 1
fi


AS02_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o /tmp/as02test -y 10:11:12:13 --clip test "
if [ "$3" != "" ]; then
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f $3 "
else
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f 25 "
fi
READ_COMMAND="../../apps/mxf2raw/mxf2raw -i --md5 /tmp/as02test/as02test.mxf"

# create essence data
../create_test_essence -t 1 -d 24 /tmp/pcm.raw
../create_test_essence -t $1 -d 24 /tmp/test_in.raw

# write
$AS02_BASE_COMMAND -a 16:9 --$2 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw >/dev/null

# read
$READ_COMMAND >/tmp/mxfreadertest_stdout

# calculate md5sum and compare with expected value
$MD5TOOL < /tmp/mxfreadertest_stdout | sed 's/\([a-f0-9]\)$/\1\ \ -/g' > /tmp/test.md5
if diff /tmp/test.md5 ${srcdir}/$2$3.md5
then
	RESULT=0
else
	echo "*** ERROR: $2$3 regression"
	RESULT=1
fi

# clean-up
rm -Rf /tmp/pcm.raw /tmp/test_in.raw /tmp/as02test /tmp/mxfreadertest_stdout /tmp/test.md5


exit $RESULT
