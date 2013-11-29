#!/bin/sh

DURATION=$1
CREATE_TYPE=$2
INPUT_TYPE=$3
shift 3
EXTRA_OPTS=
TEMP_DIR=/tmp/avid_temp$$
OUTPUT=/tmp/avidmxfsample_$INPUT_TYPE
if [ "$1" = "gf" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --avid-gf "
  OUTPUT=$OUTPUT"_gf"
  shift
elif [ "$1" = "gfp" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --regtest-end $2 "
  OUTPUT=$OUTPUT"_gfp"
  shift 2
elif [ "$1" != "" ]; then
  EXTRA_OPTS="$EXTRA_OPTS -f $1 "
  OUTPUT=$OUTPUT$1
  shift
fi
if [ "$INPUT_TYPE" = "unc" ]; then
  EXTRA_OPTS="$EXTRA_OPTS --height 576 "
fi

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t avid -o $OUTPUT -y 10:11:12:13 --clip test --tape testtape $EXTRA_OPTS"


# create essence data
../create_test_essence -t 1 -d $DURATION ${TEMP_DIR}/pcm.raw
../create_test_essence -t $CREATE_TYPE -d $DURATION ${TEMP_DIR}/test_in.raw

# write
$BASE_COMMAND -a 16:9 --$INPUT_TYPE ${TEMP_DIR}/test_in.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw -q 16 --locked true --pcm ${TEMP_DIR}/pcm.raw

# clean-up
rm -Rf ${TEMP_DIR}

