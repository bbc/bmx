#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/textobject_temp$$
sampledir=/tmp/


create_test_file()
{
    if test $1 = "op1a"; then
            $testdir/create_test_essence -t 42 -d 3 $tmpdir/audio.pcm
            $testdir/create_test_essence -t 7 -d 3 $tmpdir/video
            videoformat=avci100_1080i
    elif test $1 = "d10"; then
            $testdir/create_test_essence -t 42 -d 3 $tmpdir/audio.pcm
            $testdir/create_test_essence -t 11 -d 3 $tmpdir/video
            videoformat=d10_50
    else
            $testdir/create_test_essence -t 42 -d 24 $tmpdir/audio.pcm
            $testdir/create_test_essence -t 14 -d 24 $tmpdir/video
            videoformat=mpeg2lg_422p_hl_1080i
    fi

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t $1 \
        -f 25 \
        -y 10:00:00:00 \
        -o $2 \
        --embed-xml $base/utf8.xml \
        --embed-xml $base/utf16be.xml \
        --embed-xml $base/utf16le.xml \
        --xml-scheme-id "0772e8bd-f9a1-4b80-a517-85fd71c85675" \
        --xml-lang "de" \
        --embed-xml $base/utf8.xml \
        --embed-xml $base/other.xml \
        --embed-xml $base/other.xml \
        --embed-xml $base/utf8_noprolog.xml \
        --$videoformat $tmpdir/video \
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
    create_test_file $1 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_$1.md5 &&
        create_read_result $tmpdir/test.xml $tmpdir/textobject $tmpdir/test.mxf &&
        diff $tmpdir/test.xml $base/info_$1.xml &&
        diff $tmpdir/textobject_0.xml $base/utf8.xml &&
        diff $tmpdir/textobject_1.xml $base/utf16be.xml &&
        diff $tmpdir/textobject_2.xml $base/utf16le.xml &&
        diff $tmpdir/textobject_3.xml $base/utf8.xml &&
        diff $tmpdir/textobject_4.xml $base/other.xml &&
        diff $tmpdir/textobject_5.xml $base/other.xml &&
        diff $tmpdir/textobject_6.xml $base/utf8_noprolog.xml
}

create_data()
{
    create_test_file $1 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_$1.md5 &&
        create_read_result $base/info_$1.xml $tmpdir/textobject $tmpdir/test.mxf
}

create_samples()
{
    create_test_file $1 $sampledir/test_$1.mxf &&
        create_read_result $sampledir/test_$1.xml $sampledir/textobject_$1 $sampledir/test_$1.mxf
}


check_all()
{
    check op1a && check rdd9 && check d10
}

create_data_all()
{
    create_data op1a && create_data rdd9 && create_data d10
}

create_samples_all()
{
    create_samples op1a && create_samples rdd9 && create_samples d10
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
