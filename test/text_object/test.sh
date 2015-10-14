#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/textobject_temp$$
sampledir=/tmp/


create_test_file()
{
    $testdir/create_test_essence -t 42 -d 3 $tmpdir/audio.pcm
    $testdir/create_test_essence -t 7 -d 3 $tmpdir/video.h264

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -y 10:00:00:00 \
        -o $1 \
        --embed-xml $base/utf8.xml \
        --embed-xml $base/utf16be.xml \
        --embed-xml $base/utf16le.xml \
        --xml-scheme-id "0772e8bd-f9a1-4b80-a517-85fd71c85675" \
        --xml-lang "de" \
        --embed-xml $base/utf8.xml \
        --embed-xml $base/other.xml \
        --embed-xml $base/other.xml \
        --avci100_1080i $tmpdir/video.h264 \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        >/dev/null
}

create_read_result()
{
    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $1 \
        --text-out $2 \
        $3
}


check()
{
    create_test_file $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test.md5 &&
        create_read_result $tmpdir/test.xml $tmpdir/textobject $tmpdir/test.mxf &&
        diff $tmpdir/test.xml $base/info.xml &&
        diff $tmpdir/textobject_0.xml $base/utf8.xml &&
        diff $tmpdir/textobject_1.xml $base/utf16be.xml &&
        diff $tmpdir/textobject_2.xml $base/utf16le.xml &&
        diff $tmpdir/textobject_3.xml $base/utf8.xml &&
        diff $tmpdir/textobject_4.xml $base/other.xml &&
        diff $tmpdir/textobject_5.xml $base/other.xml
}

create_data()
{
    create_test_file $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test.md5 &&
        create_read_result $base/info.xml $tmpdir/textobject $tmpdir/test.mxf
}

create_sample()
{
    create_test_file $sampledir/test.mxf &&
        create_read_result $sampledir/test.xml $sampledir/textobject $sampledir/test.mxf
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
