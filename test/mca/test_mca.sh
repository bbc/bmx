#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/mca_temp$$
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
        -o $2 \
        --track-map $1 \
        $tmpdir/input.mxf \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $3 \
        --mca-detail \
        $2
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
        -o $2 \
        --track-map $1 \
        --avci100_1080i $tmpdir/video \
        --locked true --wave $tmpdir/input.wav \
        -q 24 --locked true --pcm $tmpdir/audio6 \
        -q 24 --locked true --pcm $tmpdir/audio7 \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $3 \
        --mca-detail \
        $2
}


check()
{
    create_file_bmxtranswrap $1 $tmpdir/test1.mxf $tmpdir/test1.xml &&
        $md5tool < $tmpdir/test1.mxf > $tmpdir/test1.md5 &&
        diff $tmpdir/test1.md5 $base/mca_$2.md5 &&
        diff $tmpdir/test1.xml $base/mca_$2.xml &&
    create_file_raw2bmx $1 $tmpdir/test2.mxf $tmpdir/test2.xml &&
        $md5tool < $tmpdir/test2.mxf > $tmpdir/test2.md5 &&
        diff $tmpdir/test2.md5 $base/mca_$2.md5 &&
        diff $tmpdir/test2.xml $base/mca_$2.xml
}

create_data()
{
    create_file_bmxtranswrap $1 $tmpdir/test.mxf $base/mca_$2.xml &&
        $md5tool < $tmpdir/test.mxf > $base/mca_$2.md5
}

create_sample()
{
    create_file_bmxtranswrap $1 $sampledir/mca_$2.mxf $sampledir/mca_$2.xml
}


check_all()
{
    check mono mono &&
        check stereo stereo &&
        check singlemca singlemca &&
        check mx monorem &&
        check "0-1;x" stereoprem &&
        check "0-1;s2,2-5" stereops2p4 &&
        check "0-1;0-1;mx" stereopstereopmonorem &&
        check "7;6;5;4;3;2;1;0" reorder_mono &&
        check "7,6;5,4;3,2;1,0" reorder_stereo
}

create_data_all()
{
    create_data mono mono &&
        create_data stereo stereo &&
        create_data singlemca singlemca &&
        create_data mx monorem &&
        create_data "0-1;x" stereoprem &&
        create_data "0-1;s2,2-5" stereops2p4 &&
        create_data "0-1;0-1;mx" stereopstereopmonorem &&
        create_data "7;6;5;4;3;2;1;0" reorder_mono &&
        create_data "7,6;5,4;3,2;1,0" reorder_stereo
}

create_sample_all()
{
    create_sample mono mono &&
        create_sample stereo stereo &&
        create_sample singlemca singlemca &&
        create_sample mx monorem &&
        create_sample "0-1;x" stereoprem &&
        create_sample "0-1;s2,2-5" stereops2p4 &&
        create_sample "0-1;0-1;mx" stereopstereopmonorem &&
        create_sample "7;6;5;4;3;2;1;0" reorder_mono &&
        create_sample "7,6;5,4;3,2;1,0" reorder_stereo
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
