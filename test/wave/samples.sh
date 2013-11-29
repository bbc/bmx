#!/bin/sh

TEMP_DIR=/tmp/wave_temp$$

mkdir -p ${TEMP_DIR}


BASE_COMMAND="../../apps/raw2bmx/raw2bmx --regtest -t wave -o /tmp/sample_$2.wav -f 25 -y 10:11:12:13 --orig regtest "


# create essence data
../create_test_essence -t 1 -d $1 ${TEMP_DIR}/pcm.raw

# write
$BASE_COMMAND -q 16 --pcm ${TEMP_DIR}/pcm.raw -q 16 --pcm ${TEMP_DIR}/pcm.raw >/dev/null

# clean-up
rm -Rf ${TEMP_DIR}

