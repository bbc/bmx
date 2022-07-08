#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/mcalabels_temp$$
sampledir=/tmp/


create_test_ess()
{
    $testdir/create_test_essence -t 42 -d 3 -s 0 $tmpdir/audio0
    $testdir/create_test_essence -t 42 -d 3 -s 1 $tmpdir/audio1
    $testdir/create_test_essence -t 42 -d 3 -s 2 $tmpdir/audio2
    $testdir/create_test_essence -t 42 -d 3 -s 3 $tmpdir/audio3
    $testdir/create_test_essence -t 42 -d 3 -s 4 $tmpdir/audio4
    $testdir/create_test_essence -t 42 -d 3 -s 5 $tmpdir/audio5
    $testdir/create_test_essence -t 42 -d 3 -s 6 $tmpdir/audio6
    $testdir/create_test_essence -t 42 -d 3 -s 7 $tmpdir/audio7
    $testdir/create_test_essence -t 7 -d 3 $tmpdir/video
}

create_file_bmxtranswrap()
{
    create_test_ess

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $tmpdir/input.mxf \
        --avci100_1080i $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio0 \
        -q 24 --locked true --pcm $tmpdir/audio1 \
        -q 24 --locked true --pcm $tmpdir/audio2 \
        -q 24 --locked true --pcm $tmpdir/audio3 \
        -q 24 --locked true --pcm $tmpdir/audio4 \
        -q 24 --locked true --pcm $tmpdir/audio5 \
        -q 24 --locked true --pcm $tmpdir/audio6 \
        -q 24 --locked true --pcm $tmpdir/audio7 \
        >/dev/null

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $4 \
        --track-map $1 \
        --track-mca-labels $2 $3 \
        --audio-layout as11-mode-0 \
        $tmpdir/input.mxf \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $5 \
        --mca-detail \
        $4
}

create_file_raw2bmx()
{
    create_test_ess

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t wave \
        -o $tmpdir/input.wav \
        -q 24 --locked true --pcm $tmpdir/audio0 \
        -q 24 --locked true --pcm $tmpdir/audio1 \
        -q 24 --locked true --pcm $tmpdir/audio2 \
        -q 24 --locked true --pcm $tmpdir/audio3 \
        -q 24 --locked true --pcm $tmpdir/audio4 \
        -q 24 --locked true --pcm $tmpdir/audio5 \
        >/dev/null

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $4 \
        --track-map $1 \
        --track-mca-labels $2 $3 \
        --audio-layout as11-mode-0 \
        --avci100_1080i $tmpdir/video \
        --locked true --wave $tmpdir/input.wav \
        -q 24 --locked true --pcm $tmpdir/audio6 \
        -q 24 --locked true --pcm $tmpdir/audio7 \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $5 \
        --mca-detail \
        $4
}


check()
{
    create_file_bmxtranswrap $1 $2 $3 $tmpdir/test1.mxf $tmpdir/test1.xml &&
        $md5tool < $tmpdir/test1.mxf > $tmpdir/test1.md5 &&
        diff $tmpdir/test1.md5 $base/mcalabels_$4.md5 &&
        diff $tmpdir/test1.xml $base/mcalabels_$4.xml &&
    create_file_raw2bmx $1 $2 $3 $tmpdir/test2.mxf $tmpdir/test2.xml &&
        $md5tool < $tmpdir/test2.mxf > $tmpdir/test2.md5 &&
        diff $tmpdir/test2.md5 $base/mcalabels_$4.md5 &&
        diff $tmpdir/test2.xml $base/mcalabels_$4.xml
}

create_data()
{
    create_file_bmxtranswrap $1 $2 $3 $tmpdir/test.mxf $base/mcalabels_$4.xml &&
        $md5tool < $tmpdir/test.mxf > $base/mcalabels_$4.md5
}

create_samples()
{
    create_file_bmxtranswrap $1 $2 $3 $sampledir/mca_labels_$4.mxf $sampledir/mca_labels_$4.xml
}


check_all()
{
    check stereo as11 $base/stereo.txt stereo &&
        check mono as11 $base/mono.txt mono &&
        check "0,1;2-7" as11 $base/stereop51.txt stereop51 &&
        check "0,1;2-7" as11 $base/mess.txt mess &&
        check mono as11 $base/mononochan.txt mononochan &&
        check "0,1;2-7" any $base/imf.txt imf &&
        check "0,1;2-7" any $base/mixed.txt mixed &&
        check "0,1;2-7" any $base/properties.txt properties
}

create_data_all()
{
    create_data stereo as11 $base/stereo.txt stereo &&
        create_data mono as11 $base/mono.txt mono &&
        create_data "0,1;2-7" as11 $base/stereop51.txt stereop51 &&
        create_data "0,1;2-7" as11 $base/mess.txt mess &&
        create_data mono as11 $base/mononochan.txt mononochan &&
        create_data "0,1;2-7" any $base/imf.txt imf &&
        create_data "0,1;2-7" any $base/mixed.txt mixed &&
        create_data "0,1;2-7" any $base/properties.txt properties
}

create_samples_all()
{
    create_samples stereo as11 $base/stereo.txt stereo &&
        create_samples mono as11 $base/mono.txt mono &&
        create_samples "0,1;2-7" as11 $base/stereop51.txt stereop51 &&
        create_samples "0,1;2-7" as11 $base/mess.txt mess &&
        create_samples mono as11 $base/mononochan.txt mononochan &&
        create_samples "0,1;2-7" any $base/imf.txt imf &&
        create_samples "0,1;2-7" any $base/mixed.txt mixed &&
        create_samples "0,1;2-7" any $base/properties.txt properties
}


mkdir -p $tmpdir

if test "$1" = "create-data" ; then
    create_data_all
elif test "$1" = "create-samples" ; then
    create_samples_all
else
    check_all
fi
res=$?

rm -Rf $tmpdir

exit $res
