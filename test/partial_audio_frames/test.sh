#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/audio_padding_temp$$
sampledir=/tmp/
samplemxf=$base/sample.mxf


create_precharge_padding()
{
    # Create 12 frames of PCM and MPEG-2 LG
    $testdir/create_test_essence -t 1 -d 12 $tmpdir/pcm.raw
    $testdir/create_test_essence -t 24 -d 12 $tmpdir/m2v.raw

    # Wrap in MXF
    $appsdir/raw2bmx/raw2bmx \
        -t op1a \
        -f 25 \
        -o $tmpdir/pass1.mxf \
        --audio-chan 1 \
        -q 16 \
        --pcm $tmpdir/pcm.raw \
        --mpeg2lg_422p_hl_1080i $tmpdir/m2v.raw

    # Transwrap to Avid with 1 frame start offset, resulting in video with 1 frame pre-charge
    $appsdir/bmxtranswrap/bmxtranswrap \
        -t avid \
        -o $tmpdir/pass2 \
        --start 1 \
        $tmpdir/pass1.mxf

    # Transwrap both Avid files to OP-1A, which should add 1 frame of PCM padding for the pre-charge
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $1 \
        --clip test \
        -y 23:58:30:00 \
        $tmpdir/pass2_a1.mxf \
        $tmpdir/pass2_v1.mxf
}

create_wave()
{
    # Create 960 samples of PCM
    $testdir/create_test_essence -t 57 -d 960 $tmpdir/pcm.raw

    # Clip wrap in MXF, using -f 50 to ensure 960 samples are transferred
    $appsdir/raw2bmx/raw2bmx \
        -t op1a \
        -f 50 \
        --clip-wrap \
        -o $tmpdir/pass1.mxf \
        --audio-chan 1 \
        -q 16 \
        --pcm $tmpdir/pcm.raw \

    # Transwrap to wave. This should accept the partial "25Hz frame" containing 960 instead of 1920 samples
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $1 \
        $tmpdir/pass1.mxf
}

check()
{
    create_precharge_padding $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_1.md5 &&
    create_wave $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_2.md5
}

create_data()
{
    create_precharge_padding $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_1.md5
    create_wave $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_2.md5
}

create_samples()
{
    create_precharge_padding $sampledir/test_partial_audio_precharge.mxf &&
    create_wave $sampledir/test_partial_audio_wave.wav
}


mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_samples" ; then
    create_samples
else
    check
fi
res=$?

rm -Rf $tmpdir

exit $res
