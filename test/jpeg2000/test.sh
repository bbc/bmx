#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/jpeg2000_temp$$
sampledir=/tmp/


create_mxf_cdci()
{
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --clip test \
        --part 60s \
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
        --display-primaries 35400,14600,8500,39850,6550,2300 \
        --display-chroma 15635,16450 \
        --display-max-luma 10000000 \
        --display-min-luma 50 \
        --fill-pattern-gaps \
        --j2c_cdci $1
        >/dev/null
}

create_mxf_rgba()
{
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --clip test \
        --part 60s \
        -f 25 \
        -a 16:9 \
        --frame-layout fullframe \
        --transfer-ch hlg \
        --coding-eq bt2020 \
        --color-prim bt2020 \
        --comp-max-ref 940 \
        --comp-min-ref 64 \
        --scan-dir 0 \
        --display-primaries 35400,14600,8500,39850,6550,2300 \
        --display-chroma 15635,16450 \
        --display-max-luma 10000000 \
        --display-min-luma 50 \
        --fill-pattern-gaps \
        --j2c_rgba $1
        >/dev/null
}

transwrap_mxf()
{
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t op1a \
        -o $2 \
        --clip test \
        --part 60s \
        $1 \
        >/dev/null
}

create_read_result()
{
    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $2 \
        $1
}


check()
{
    create_mxf_cdci $base/image_yuv_%d.j2c $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_1.md5 &&
        create_read_result $tmpdir/test.mxf $tmpdir/test.xml &&
        diff $tmpdir/test.xml $base/info_1.xml &&
    transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
        $md5tool < $tmpdir/test_transwrap.mxf > $tmpdir/test_transwrap.md5 &&
        diff $tmpdir/test_transwrap.md5 $base/test_2.md5 &&
        create_read_result $tmpdir/test.mxf $tmpdir/test.xml &&
        diff $tmpdir/test.xml $base/info_2.xml &&
    create_mxf_rgba $base/image_rgb_%d.j2c $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_3.md5 &&
        create_read_result $tmpdir/test.mxf $tmpdir/test.xml &&
        diff $tmpdir/test.xml $base/info_3.xml &&
    transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
        $md5tool < $tmpdir/test_transwrap.mxf > $tmpdir/test_transwrap.md5 &&
        diff $tmpdir/test_transwrap.md5 $base/test_4.md5 &&
        create_read_result $tmpdir/test.mxf $tmpdir/test.xml &&
        diff $tmpdir/test.xml $base/info_4.xml
}

create_data()
{
    create_mxf_cdci $base/image_yuv_%d.j2c $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_1.md5 &&
        create_read_result $tmpdir/test.mxf $base/info_1.xml &&
    transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
        $md5tool < $tmpdir/test_transwrap.mxf > $base/test_2.md5 &&
        create_read_result $tmpdir/test.mxf $base/info_2.xml &&
    create_mxf_rgba $base/image_rgb_%d.j2c $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_3.md5 &&
        create_read_result $tmpdir/test.mxf $base/info_3.xml &&
    transwrap_mxf $tmpdir/test.mxf $tmpdir/test_transwrap.mxf &&
        $md5tool < $tmpdir/test_transwrap.mxf > $base/test_4.md5 &&
        create_read_result $tmpdir/test.mxf $base/info_4.xml
}

create_samples()
{
    create_mxf_cdci $base/image_yuv_%d.j2c $sampledir/test_1.mxf &&
        create_read_result $sampledir/test_1.mxf $sampledir/info_1.xml &&
    transwrap_mxf $sampledir/test_1.mxf $sampledir/test_2.mxf &&
        create_read_result $sampledir/test_2.mxf $sampledir/info_2.xml &&
    create_mxf_rgba $base/image_rgb_%d.j2c $sampledir/test_3.mxf &&
        create_read_result $sampledir/test_3.mxf $sampledir/info_3.xml &&
    transwrap_mxf $sampledir/test_3.mxf $sampledir/test_4.mxf &&
        create_read_result $sampledir/test_4.mxf $sampledir/info_4.xml
}


mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_samples" ; then
    create_samples
else
    check
fi
res=$?

rm -Rf $tmpdir

exit $res
