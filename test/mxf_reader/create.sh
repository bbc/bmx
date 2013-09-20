#!/bin/sh

MD5TOOL=../file_md5
TEMP_DIR=/tmp/readertest_temp$$

mkdir -p ${TEMP_DIR}


AS02_BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t as02 -o ${TEMP_DIR}/as02test -y 10:11:12:13 --clip test "
if [ "$5" != "" ]; then
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f $5 "
else
  AS02_BASE_COMMAND="$AS02_BASE_COMMAND -f 25 "
fi
READ_COMMAND="../../apps/mxf2raw/mxf2raw --regtest --info --info-format xml --track-chksum md5 ${TEMP_DIR}/as02test/as02test.mxf"


# create essence data
../create_test_essence -t 1 -d $2 ${TEMP_DIR}/pcm.raw
../create_test_essence -t $3 -d $2 ${TEMP_DIR}/test_in.raw

# write, read and calculate md5sum
if $AS02_BASE_COMMAND -a 16:9 --$4 ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw >/dev/null
then
  if $READ_COMMAND | sed "s:${TEMP_DIR}:/tmp:g" >${TEMP_DIR}/mxfreadertest_stdout
  then
    $MD5TOOL < ${TEMP_DIR}/mxfreadertest_stdout > $1/$4$5.md5
    RESULT=0
  else
    RESULT=1
  fi
else
  RESULT=1
fi

  
# clean-up
rm -Rf ${TEMP_DIR}


exit $RESULT

