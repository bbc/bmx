#!/bin/sh

AS02_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 --mic-type md5 -o /tmp/sample_$3$4 -y 10:11:12:13 --clip test "
if [ "$4" != "" ]; then
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f $4 "
else
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f 25 "
fi
READ_COMMAND="../../apps/mxf2raw/mxf2raw -i --md5 /tmp/sample_$3$4/sample_$3$4.mxf"


# create essence data
../create_test_essence -t 1 -d $1 /tmp/pcm.raw
../create_test_essence -t $2 -d $1 /tmp/test_in.raw

# write
$AS02_BASE_COMMAND -a 16:9 --$3 /tmp/test_in.raw -q 16 --locked true --pcm /tmp/pcm.raw -q 16 --locked true --pcm /tmp/pcm.raw

# read
$READ_COMMAND >/tmp/sample_stdout_$3$4

# clean-up
rm /tmp/pcm.raw /tmp/test_in.raw

