#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/as10test_temp$$
sampledir=/tmp/


create_raw2bmx()
{
    $testdir/create_test_essence -t 42 -d 36 $tmpdir/audio
    $testdir/create_test_essence -t 53 -d 36 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t as10 \
        -f 25 \
        -y 22:22:22:22 \
        --single-pass \
        --part 25 \
        -o $1 \
        --dm-file as10 $base/as10_core_framework.txt \
        --shim-name high_hd_2014 \
        --mpeg-checks \
        --print-checks \
        --mpeg2lg_422p_hl_1080i $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $2 \
        --as10 \
        $1
}

create_bmxtranswrap()
{
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t as10 \
        -o $2 \
        --dm-file as10 $base/as10_core_framework.txt \
        --shim-name high_hd_2014 \
        --mpeg-checks \
        --max-same-warnings 2 \
        --print-checks \
        $1

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $3 \
        --as10 \
        $2
}


check()
{
    create_raw2bmx $tmpdir/testrb.mxf $tmpdir/testrb.xml &&
        $md5tool < $tmpdir/testrb.mxf > $tmpdir/testrb.md5 &&
        diff $tmpdir/testrb.md5 $base/$1_rb.md5 &&
        diff $tmpdir/testrb.xml $base/$1_rb.xml &&
    create_bmxtranswrap $tmpdir/testrb.mxf $tmpdir/testtw.mxf $tmpdir/testtw.xml &&
        $md5tool < $tmpdir/testtw.mxf > $tmpdir/testtw.md5 &&
        diff $tmpdir/testtw.md5 $base/$1_tw.md5 &&
        diff $tmpdir/testtw.xml $base/$1_tw.xml
}

create_data()
{
    create_raw2bmx $tmpdir/testrb.mxf $base/$1_rb.xml &&
        $md5tool < $tmpdir/testrb.mxf > $base/$1_rb.md5 &&
    create_bmxtranswrap $tmpdir/testrb.mxf $tmpdir/testtw.mxf $base/$1_tw.xml &&
        $md5tool < $tmpdir/testtw.mxf > $base/$1_tw.md5
}

create_samples()
{
    create_raw2bmx $sampledir/as10_$1_rb.mxf $sampledir/as10_$1_rb.xml &&
    create_bmxtranswrap $sampledir/as10_$1_rb.mxf $sampledir/as10_$1_tw.mxf $sampledir/as10_$1_tw.xml
}


check_all()
{
    check high_hd_2014
}

create_data_all()
{
    create_data high_hd_2014
}

create_samples_all()
{
    create_samples high_hd_2014
}


mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data_all
elif test "$1" = "create_samples" ; then
    create_samples_all
else
    check_all
fi
res=$?

rm -Rf $tmpdir

exit $res
