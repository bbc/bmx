#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/as11test_temp$$
sampledir=/tmp/


create_test_file()
{
    $testdir/create_test_essence -t 42 -d 24 $tmpdir/audio
    $testdir/create_test_essence -t $1 -d 24 $tmpdir/video

    type=
    coreopts=
    dppopts=
    if test $2 != mpeg2lg_422p_hl_1080i; then
        type=as11op1a
        coreopts="--dm-file as11 $base/as11_core_framework.txt"
        dppopts="--dm-file dpp $base/ukdpp_framework.txt"
    else
        type=as11rdd9
    fi

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t $type \
        -f 25 \
        -y 09:58:00:00 \
        -o $3 \
        $coreopts \
        $dppopts \
        --seg $base/as11_segmentation_framework.txt \
        --afd 10 -a 16:9 --$2 $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        >/dev/null
}

create_read_result()
{
    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $1 \
        --as11 \
        $2
}


check()
{
    create_test_file $1 $2 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/$2.md5 &&
        create_read_result $tmpdir/test.xml $tmpdir/test.mxf &&
        diff $tmpdir/test.xml $base/$2.xml
}

create_data()
{
    create_test_file $1 $2 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/$2.md5 &&
        create_read_result $base/$2.xml $tmpdir/test.mxf
}

create_samples()
{
    create_test_file $1 $2 $sampledir/as11_$2.mxf &&
        create_read_result $sampledir/as11_$2.xml $sampledir/as11_$2.mxf
}


check_all()
{
    check 7 avci100_1080i &&
        check 11 d10_50 &&
        check 14 mpeg2lg_422p_hl_1080i
}

create_data_all()
{
    create_data 7 avci100_1080i &&
        create_data 11 d10_50 &&
        create_data 14 mpeg2lg_422p_hl_1080i
}

create_samples_all()
{
    create_samples 7 avci100_1080i &&
        create_samples 11 d10_50 &&
        create_samples 14 mpeg2lg_422p_hl_1080i
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
