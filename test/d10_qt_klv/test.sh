#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/d10_qt_klv_temp$$
sampledir=/tmp/
samplemxf=$base/sample.mxf


rawwrap()
{
    $appsdir/mxf2raw/mxf2raw -p $tmpdir/input $1

    $appsdir/raw2bmx/raw2bmx \
        --regtest-real \
        -t d10 \
        -o $2 \
        --clip test \
        -y 23:58:30:00 \
        --d10 $tmpdir/input_v0.raw \
        --audio-chan 4 \
        -q 24 \
        --pcm $tmpdir/input_a0.raw
}

transwrap()
{
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t d10 \
        -o $2 \
        --clip test \
        --assume-d10-50 \
        $1
}


check()
{
    # Perform the following checks:
    # 1) wrapping from the raw essence extracted from sample.mxf containing the D-10 KL prefix
    # 2) wrapping from the raw essence extracted from 1) produces the same result as 1)
    # 3) transwrapping from the sample.mxf containing the D-10 KL prefix
    # 4) transwrapping from the MXF from 3) produces the same result as 3)
    rawwrap $samplemxf $tmpdir/test_rawwrap.mxf &&
        $md5tool < $tmpdir/test_rawwrap.mxf > $tmpdir/test_rawwrap.md5 &&
        diff $tmpdir/test_rawwrap.md5 $base/test_1.md5 &&
    rawwrap $tmpdir/test_rawwrap.mxf $tmpdir/test_rawwrap_again.mxf &&
        $md5tool < $tmpdir/test_rawwrap_again.mxf > $tmpdir/test_rawwrap.md5 &&
        diff $tmpdir/test_rawwrap_again.md5 $tmpdir/test_rawwrap.md5
    transwrap $samplemxf $tmpdir/test_transwrap.mxf &&
        $md5tool < $tmpdir/test_transwrap.mxf > $tmpdir/test_transwrap.md5 &&
        diff $tmpdir/test_transwrap.md5 $base/test_2.md5 &&
    transwrap $tmpdir/test_transwrap.mxf $tmpdir/test_transwrap_again.mxf &&
        $md5tool < $tmpdir/test_transwrap_again.mxf > $tmpdir/test_transwrap_again.md5 &&
        diff $tmpdir/test_transwrap_again.md5 $tmpdir/test_transwrap.md5
}

create_data()
{
    rawwrap $samplemxf $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_1.md5 &&
    transwrap $samplemxf $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_2.md5
}

create_samples()
{
    rawwrap $samplemxf $sampledir/test_d10_qt_klv_rawwrap.mxf &&
    transwrap $samplemxf $sampledir/test_d10_qt_klv_transwrap.mxf
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
