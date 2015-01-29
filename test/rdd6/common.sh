#!/bin/sh

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/rdd6_temp$$
sampledir=/tmp/


create_test_file()
{
    $testdir/create_test_essence -t 42 -d 3 $tmpdir/$1.pcm
    $testdir/create_test_essence -t 7 -d 3 $tmpdir/$1.h264

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $tmpdir/$1_input.mxf \
        --avci100_1080i $tmpdir/$1.h264 \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        -q 24 --pcm $tmpdir/$1.pcm \
        >/dev/null

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $tmpdir/$1.mxf \
        --rdd6 $base/$1.xml \
        --rdd6-lines $2 \
        --rdd6-sdid $3 \
        $tmpdir/$1_input.mxf \
        >/dev/null
}

extract_rdd6_xml()
{
    $appsdir/mxf2raw/mxf2raw --rdd6 0-2 $tmpdir/$1.xml $tmpdir/$1.mxf
}

check()
{
    create_test_file $1 $2 $3 &&
        $md5tool < $tmpdir/$1.mxf > $tmpdir/$1.md5 &&
        extract_rdd6_xml $1 &&
        $md5tool < $tmpdir/$1.xml >> $tmpdir/$1.md5 &&
        diff $tmpdir/$1.md5 $base/$1.md5
}

create_data()
{
    create_test_file $1 $2 $3 &&
        $md5tool < $tmpdir/$1.mxf > $base/$1.md5 &&
        $md5tool < $base/$1.xml >> $base/$1.md5
}

create_sample()
{
    create_test_file $1 $2 $3 &&
        extract_rdd6_xml $1 &&
        mv $tmpdir/$1.mxf $tmpdir/$1.xml $sampledir
}


run_test()
{
    mkdir -p $tmpdir

    if test "$1" = "create_data" ; then
        create_data $2 $3 $4
    elif test "$1" = "create_sample" ; then
        create_sample $2 $3 $4
    else
        check $1 $2 $3
    fi
    res=$?

    rm -Rf $tmpdir

    exit $res
}
