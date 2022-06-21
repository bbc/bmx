#!/bin/sh

# Tests creating Wave from inputs with a non-video frame rate multiple of samples

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/wavesoundonly_temp$$
sampledir=/tmp/


create_raw2bmx_sound_only()
{
    # Create a Wave file that should include all 5123 input samples.

    $testdir/create_test_essence -t 57 -d 5123 $tmpdir/test_pcm.raw

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t wave \
        -o $1 \
        -q 16 \
        --pcm $tmpdir/test_pcm.raw \
        >/dev/null
}

create_bmxtranswrap_sound_only()
{
    # Transwrap from a clip-wrapped MXF to a Wave.

    $testdir/create_test_essence -t 57 -d 5123 $tmpdir/test_pcm.raw

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $tmpdir/test.mxf \
        --clip-wrap \
        -q 16 \
        --pcm $tmpdir/test_pcm.raw \
        >/dev/null

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $1 \
        $tmpdir/test.mxf \
        >/dev/null
}


check()
{
    create_raw2bmx_sound_only $tmpdir/test1.wav &&
        $md5tool < $tmpdir/test1.wav > $tmpdir/test1.md5 &&
        diff $tmpdir/test1.md5 $base/wave_sound_only_from_raw.md5 &&
    create_bmxtranswrap_sound_only $tmpdir/test2.wav &&
        $md5tool < $tmpdir/test2.wav > $tmpdir/test2.md5 &&
        diff $tmpdir/test2.md5 $base/wave_sound_only_transwrap.md5
}

create_data()
{
    create_raw2bmx_sound_only $tmpdir/test1.wav &&
        $md5tool < $tmpdir/test1.wav > $base/wave_sound_only_from_raw.md5 &&
    create_bmxtranswrap_sound_only $tmpdir/test2.wav &&
        $md5tool < $tmpdir/test2.wav > $base/wave_sound_only_transwrap.md5
}

create_samples()
{
    create_raw2bmx_sound_only $sampledir/sample_wave_sound_only_from_raw.wav &&
    create_bmxtranswrap_sound_only $sampledir/sample_wave_sound_only_transwrap.wav
}


mkdir -p $tmpdir

if test -z "$1" || test "$1" = "check" ; then
    check
elif test "$1" = "create-data" ; then
    create_data
elif test "$1" = "create-samples" ; then
    create_samples
fi
res=$?

rm -Rf $tmpdir

exit $res

