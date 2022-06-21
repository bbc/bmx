#!/bin/sh

# Tests creating sound-only OP1a from inputs with a non-video frame rate multiple of samples

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/op1asoundonly_temp$$
sampledir=/tmp/


create_raw2bmx_sound_only()
{
    # Create a clip-wrapped sound only MXF file that includes all 5123 input samples.

    $testdir/create_test_essence -t 57 -d 5123 $tmpdir/test_pcm_1.raw

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $1 \
        --clip-wrap \
        -q 16 \
        --pcm $tmpdir/test_pcm_1.raw \
        >/dev/null
}

create_bmxtranswrap_sound_only()
{
    # Transwrap to another OP1a.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $2 \
        --clip-wrap \
        $1 \
        >/dev/null
}


check()
{
    create_raw2bmx_sound_only $tmpdir/test1.mxf &&
        $md5tool < $tmpdir/test1.mxf > $tmpdir/test1.md5 &&
        diff $tmpdir/test1.md5 $base/sound_only_from_raw.md5 &&
    create_bmxtranswrap_sound_only $tmpdir/test1.mxf $tmpdir/test2.mxf &&
        $md5tool < $tmpdir/test2.mxf > $tmpdir/test2.md5 &&
        diff $tmpdir/test2.md5 $base/sound_only_transwrap.md5
}

create_data()
{
    create_raw2bmx_sound_only $tmpdir/test1.mxf &&
        $md5tool < $tmpdir/test1.mxf > $base/sound_only_from_raw.md5 &&
    create_bmxtranswrap_sound_only $tmpdir/test1.mxf $tmpdir/test2.mxf &&
        $md5tool < $tmpdir/test2.mxf > $base/sound_only_transwrap.md5
}

create_samples()
{
    create_raw2bmx_sound_only $sampledir/op1a_sound_only_from_raw.mxf &&
    create_bmxtranswrap_sound_only $sampledir/op1a_sound_only_from_raw.mxf $sampledir/op1a_sound_only_transwrap.mxf
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

