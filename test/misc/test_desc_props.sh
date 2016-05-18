#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/desc_props_temp$$
sampledir=/tmp/


create_essence()
{
    $testdir/create_test_essence -t 42 -d 3 -s 0 $tmpdir/audio0
    $testdir/create_test_essence -t 42 -d 3 -s 1 $tmpdir/audio1
    $testdir/create_test_essence -t 8 -d 3 $tmpdir/video
}

create_raw2bmx()
{
    create_essence

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --signal-std st428 \
        --frame-layout segmentedframe \
        --transfer-ch bt2020 \
        --coding-eq gbr \
        --color-prim dcdm \
        --color-siting quincunx \
        --black-level 65 \
        --white-level 938 \
        --color-range 899 \
        --avc_high_422_intra $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio0 \
        -q 24 --locked true --pcm $tmpdir/audio1 \
        >/dev/null
}

create_bmxtranswrap()
{
    create_essence

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $tmpdir/input.mxf \
        --frame-layout segmentedframe \
        --black-level 65 \
        --white-level 938 \
        --color-range 899 \
        --avc_high_422_intra $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio0 \
        -q 24 --locked true --pcm $tmpdir/audio1 \
        >/dev/null

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $1 \
        --signal-std st428 \
        --transfer-ch bt2020 \
        --coding-eq gbr \
        --color-prim dcdm \
        --color-siting quincunx \
        $tmpdir/input.mxf \
        >/dev/null
}


check()
{
    create_raw2bmx $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/desc_props_raw2bmx.md5 &&
        create_bmxtranswrap $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/desc_props_bmxtranswrap.md5
}

create_data()
{
    create_raw2bmx $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/desc_props_raw2bmx.md5 &&
        create_bmxtranswrap $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/desc_props_bmxtranswrap.md5
}

create_sample()
{
    create_raw2bmx $sampledir/desc_props_raw2bmx.mxf &&
        create_bmxtranswrap $sampledir/desc_props_bmxtranswrap.mxf
}


mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_sample" ; then
    create_sample
else
    check
fi
res=$?

rm -Rf $tmpdir

exit $res
