#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/jpeg2000_temp$$
sampledir=/tmp/


create_jpeg2000()
{
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t imf \
        -o $1 \
        --clip test \
        -f 25 \
        -a 16:9 \
        --frame-layout fullframe \
        --transfer-ch hlg \
        --coding-eq bt2020 \
        --color-prim bt2020 \
        --color-siting cositing \
        --black-level 64 \
        --white-level 940 \
        --color-range 897 \
        --active-width 220 \
        --active-height 115 \
        --active-x-offset 10 \
        --active-y-offset 10 \
        --display-primaries 35400,14600,8500,39850,6550,2300 \
        --display-white-point 15635,16450 \
        --display-max-luma 10000000 \
        --display-min-luma 50 \
        --center-cut-14-9 \
        --fill-pattern-gaps \
        --j2c_cdci $base/../jpeg2000/image_yuv_%d.j2c
        >/dev/null
}


create_rdd36()
{
    $testdir/create_test_essence -t 55 -d 3 $tmpdir/video.rdd36

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t imf \
        -o $1 \
        --clip test \
        --active-width 1910 \
        --active-height 530 \
        --active-x-offset 10 \
        --active-y-offset 10 \
        --display-primaries 35400,14600,8500,39850,6550,2300 \
        --display-white-point 15635,16450 \
        --display-max-luma 10000000 \
        --display-min-luma 50 \
        --center-cut-4-3 \
        --rdd36_422 $tmpdir/video.rdd36 \
        >/dev/null
}


create_timed_text()
{
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t imf \
        -o $1 \
        --clip test \
        -f 25 \
        --dur 100 \
        --tt $base/../timed_text/manifest_2.txt \
        >/dev/null
}


common_create_audio()
{
    $testdir/create_test_essence -t 42 -d 3 -s 0 $tmpdir/audio0
    $testdir/create_test_essence -t 42 -d 3 -s 1 $tmpdir/audio1
    $testdir/create_test_essence -t 42 -d 3 -s 2 $tmpdir/audio2
    $testdir/create_test_essence -t 42 -d 3 -s 3 $tmpdir/audio3
    $testdir/create_test_essence -t 42 -d 3 -s 4 $tmpdir/audio4
    $testdir/create_test_essence -t 42 -d 3 -s 5 $tmpdir/audio5

    if [ -n "$2" ]
    then
        labelsopt="--track-mca-labels any $base/audio_labels.txt"
    fi

    # Note that --ref-image-edit-rate and --ref-audio-align-level are only needed
    # on the first input because it is clip wrapped and the metadata is used from
    # the first input

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t imf \
        -o $1 \
        --clip test \
        $labelsopt \
        --ref-image-edit-rate 30000/1001 \
        --ref-audio-align-level -20 \
        -q 24 --pcm $tmpdir/audio0 \
        -q 24 --pcm $tmpdir/audio1 \
        -q 24 --pcm $tmpdir/audio2 \
        -q 24 --pcm $tmpdir/audio3 \
        -q 24 --pcm $tmpdir/audio4 \
        -q 24 --pcm $tmpdir/audio5 \
        >/dev/null
}

create_audio()
{
    common_create_audio $1
}

create_audio_with_labels()
{
    common_create_audio $1 withlabels
}


transwrap_mxf()
{
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t imf \
        -o $2 \
        --clip test \
        $1 \
        >/dev/null
}


check_format()
{
    create_${1} $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_$1_1.md5

    if [ -z "$2" ]
    then
        transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
            $md5tool < $tmpdir/test_transwrap.mxf > $tmpdir/test_transwrap.md5 &&
            diff $tmpdir/test_transwrap.md5 $base/test_${1}_2.md5
    fi
}

check()
{
    check_format jpeg2000
    check_format rdd36
    check_format timed_text
    check_format audio

    # transfer of audio labels not yet supported
    check_format audio_with_labels notranswrap
}

create_format_data()
{
    create_${1} $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_${1}_1.md5

    if [ -z "$2" ]
    then
        transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
            $md5tool < $tmpdir/test_transwrap.mxf > $base/test_${1}_2.md5
    fi
}

create_data()
{
    create_format_data jpeg2000
    create_format_data rdd36
    create_format_data timed_text
    create_format_data audio

    # transfer of audio labels not yet supported
    create_format audio_with_labels notranswrap
}

create_format_samples()
{
    create_${1} $sampledir/test_${1}_1.mxf

    if [ -z "$2" ]
    then
        transwrap_mxf $sampledir/test_${1}_1.mxf $sampledir/test_${1}_2.mxf
    fi
}

create_samples()
{
    create_format_samples jpeg2000
    create_format_samples rdd36
    create_format_samples timed_text
    create_format_samples audio

    # transfer of audio labels not yet supported
    create_format_samples audio_with_labels notranswrap
}


mkdir -p $tmpdir

if test "$1" = "create-data" ; then
    create_data
elif test "$1" = "create-samples" ; then
    create_samples
else
    check
fi
res=$?

rm -Rf $tmpdir

exit $res
