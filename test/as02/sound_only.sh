#!/bin/sh

# Tests creating sound-only AS-02 from inputs with a non-video frame rate multiple of samples

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/as02soundonly_temp$$
sampledir=/tmp/


create_raw2bmx_sound_only()
{
    # Create a sound only MXF file from 2 raw PCM inputs, one longer than the other.
    # Expect the output to have the shortest duration, 5123 samples.

    $testdir/create_test_essence -t 57 -d 5123 $tmpdir/test_pcm_1.raw
    $testdir/create_test_essence -t 57 -d 6123 $tmpdir/test_pcm_2.raw

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t as02 \
        -o $1 \
        -q 16 \
        --pcm $tmpdir/test_pcm_1.raw \
        -q 16 \
        --pcm $tmpdir/test_pcm_2.raw \
        >/dev/null
}

create_bmxtranswrap_sound_only()
{
    # Transwrap to another AS02.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t as02 \
        -o $2 \
        $1 \
        >/dev/null
}


check()
{
    create_raw2bmx_sound_only $tmpdir/as02test1 &&
        $md5tool < $tmpdir/as02test1/as02test1.mxf > $tmpdir/test1.md5s &&
        $md5tool < $tmpdir/as02test1/media/as02test1_a0.mxf >> $tmpdir/test1.md5s &&
        $md5tool < $tmpdir/as02test1/media/as02test1_a1.mxf >> $tmpdir/test1.md5s &&
        diff $tmpdir/test1.md5s $base/sound_only_from_raw.md5s &&
    create_bmxtranswrap_sound_only $tmpdir/as02test1/as02test1.mxf $tmpdir/as02test2 &&
        $md5tool < $tmpdir/as02test2/as02test2.mxf > $tmpdir/test2.md5s &&
        $md5tool < $tmpdir/as02test2/media/as02test2_a0.mxf >> $tmpdir/test2.md5s &&
        $md5tool < $tmpdir/as02test2/media/as02test2_a1.mxf >> $tmpdir/test2.md5s &&
        diff $tmpdir/test2.md5s $base/sound_only_transwrap.md5s
}

create_data()
{
    create_raw2bmx_sound_only $tmpdir/as02test1 &&
        $md5tool < $tmpdir/as02test1/as02test1.mxf > $base/sound_only_from_raw.md5s &&
        $md5tool < $tmpdir/as02test1/media/as02test1_a0.mxf >> $base/sound_only_from_raw.md5s &&
        $md5tool < $tmpdir/as02test1/media/as02test1_a1.mxf >> $base/sound_only_from_raw.md5s &&
    create_bmxtranswrap_sound_only $tmpdir/as02test1/as02test1.mxf $tmpdir/as02test2 &&
        $md5tool < $tmpdir/as02test2/as02test2.mxf > $base/sound_only_transwrap.md5s &&
        $md5tool < $tmpdir/as02test2/media/as02test2_a0.mxf >> $base/sound_only_transwrap.md5s &&
        $md5tool < $tmpdir/as02test2/media/as02test2_a1.mxf >> $base/sound_only_transwrap.md5s
}

create_samples()
{
    create_raw2bmx_sound_only $sampledir/sample_sound_only_from_raw &&
    create_bmxtranswrap_sound_only $sampledir/sample_sound_only_from_raw/sample_sound_only_from_raw.mxf $sampledir/sample_sound_only_transwrap
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

