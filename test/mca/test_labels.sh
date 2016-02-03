#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/mcalabels_temp$$
sampledir=/tmp/


create_file()
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
        -o $3 \
        --track-map $1 \
        --track-mca-labels as11 $2 \
        --audio-layout as11-mode-0 \
        $tmpdir/input.mxf \
        >/dev/null
}


check()
{
    create_file $1 $2 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/mcalabels_$3.md5
}

create_data()
{
    create_file $1 $2 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/mcalabels_$3.md5
}

create_sample()
{
    create_file $1 $2 $sampledir/mcalabels_$3.mxf
}


check_all()
{
    check stereo $base/stereo.txt stereo &&
        check "0,1;2-7" $base/stereop51.txt stereop51 &&
        check "0,1;2-7" $base/mess.txt mess
}

create_data_all()
{
    create_data stereo $base/stereo.txt stereo &&
        create_data "0,1;2-7" $base/stereop51.txt stereop51 &&
        create_data "0,1;2-7" $base/mess.txt mess
}

create_sample_all()
{
    create_sample stereo $base/stereo.txt stereo &&
        create_sample "0,1;2-7" $base/stereop51.txt stereop51 &&
        create_sample "0,1;2-7" $base/mess.txt mess
}


mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data_all
elif test "$1" = "create_sample" ; then
    create_sample_all
else
    check_all
fi
res=$?

rm -Rf $tmpdir

exit $res
