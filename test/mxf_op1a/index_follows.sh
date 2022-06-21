#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/op1aindexfollows_temp$$
sampledir=/tmp/

testpcm="$tmpdir/test_pcm.raw"
testavci="$tmpdir/test_avci.raw"


create_test_files()
{
    $testdir/create_test_essence -t 1 -d 3 $testpcm
    $testdir/create_test_essence -t 7 -d 3 $testavci
}

create_index_follows_op1a()
{
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $2 \
        --index-follows \
        $1 \
        --avci100_1080i $testavci \
        -q 16 \
        --pcm $testpcm \
        >/dev/null
}


create_regular_op1a()
{
    create_index_follows_op1a "" $1
}

create_min_partitions_op1a()
{
    create_index_follows_op1a "--min-part" $1
}

create_body_partitions_op1a()
{
    create_index_follows_op1a "--body-part" $1
}

create_repeat_index_op1a()
{
    create_index_follows_op1a "--repeat-index" $1
}

check_can_read()
{
    $appsdir/mxf2raw/mxf2raw --read-ess $1 >/dev/null
}


check()
{
    create_regular_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/index_follows_1.md5 &&
    create_min_partitions_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/index_follows_2.md5 &&
    create_body_partitions_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/index_follows_3.md5 &&
    create_repeat_index_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/index_follows_4.md5
}

create_data()
{
    create_regular_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/index_follows_1.md5 &&
    create_min_partitions_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/index_follows_2.md5 &&
    create_body_partitions_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/index_follows_3.md5 &&
    create_repeat_index_op1a $tmpdir/test.mxf &&
        check_can_read $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/index_follows_4.md5
}

create_samples()
{
    create_regular_op1a $sampledir/op1a_index_follows.mxf &&
    create_min_partitions_op1a $sampledir/op1a_index_follows_min_partitions.mxf &&
    create_body_partitions_op1a $sampledir/op1a_index_follows_body_partitions.mxf &&
    create_repeat_index_op1a $sampledir/op1a_index_follows_repeat_index.mxf
}


mkdir -p $tmpdir

create_test_files

if test -z "$1" || test "$1" = "check" ; then
    check
elif test "$1" = "create-data" ; then
    create_data
elif test "$1" = "create-samples" ; then
    create_samples
fi
res=$?

rm -Rf $tmpdir

exit $res

