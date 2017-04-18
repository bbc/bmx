#!/bin/sh

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/rdd6_temp$$
sampledir=/tmp/


create_test_file()
{
    $testdir/create_test_essence -t 42 -d 3 $tmpdir/$1.pcm
    if test $2 -gt 30; then
        $testdir/create_test_essence -t 45 -d 3 $tmpdir/$1.h264
        type=unc_3840
    else
        $testdir/create_test_essence -t 7 -d 3 $tmpdir/$1.h264
        type=avci100_1080i
    fi

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -f $2 \
        -t op1a \
        -o $tmpdir/$1_input.mxf \
        --$type $tmpdir/$1.h264 \
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
        --rdd6-lines $3 \
        --rdd6-sdid $4 \
        $tmpdir/$1_input.mxf \
        >/dev/null
}

extract_rdd6_xml()
{
    $appsdir/mxf2raw/mxf2raw --rdd6 0-2 $tmpdir/$1.xml $tmpdir/$1.mxf
}

check()
{
    create_test_file $1 $2 $3 $4 &&
        $md5tool < $tmpdir/$1.mxf > $tmpdir/$1.md5 &&
        extract_rdd6_xml $1 &&
        $md5tool < $tmpdir/$1.xml >> $tmpdir/$1.md5 &&
        diff $tmpdir/$1.md5 $base/$1.md5
}

create_data()
{
    create_test_file $1 $2 $3 $4 &&
        $md5tool < $tmpdir/$1.mxf > $base/$1.md5 &&
        $md5tool < $base/$1.xml >> $base/$1.md5
}

create_samples()
{
    create_test_file $1 $2 $3 $4 &&
        extract_rdd6_xml $1 &&
        mv $tmpdir/$1.mxf $tmpdir/$1.xml $sampledir
}


run_test()
{
    mkdir -p $tmpdir

    if test "$1" = "create_data" ; then
        create_data $2 $3 $4 $5
    elif test "$1" = "create_samples" ; then
        create_samples $2 $3 $4 $5
    else
        check $1 $2 $3 $4
    fi
    res=$?

    rm -Rf $tmpdir

    exit $res
}
