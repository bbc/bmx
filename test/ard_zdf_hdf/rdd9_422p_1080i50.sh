#!/bin/sh

base=$(dirname $0)
prefix=$(basename $0 .sh)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/ardzdfhdf_temp$$
sampledir=/tmp/


create_test_file()
{
    $testdir/create_test_essence -t 42 -d 14 $tmpdir/audio.pcm
    $testdir/create_test_essence -t 14 -d 14 $tmpdir/video.m2v

    # NOTE: --part 12 only used to force multiple body partitions in this short sample
    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t rdd9 \
        --ard-zdf-hdf \
        -f 25 \
        -y 10:00:00:00 \
        --part 12 \
        -o $tmpdir/$1.mxf \
        --afd 10 \
        --mpeg2lg_422p_hl_1080i $tmpdir/video.m2v \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        -q 24 --pcm $tmpdir/audio.pcm \
        >/dev/null
}

check()
{
    create_test_file $prefix &&
        $md5tool < $tmpdir/$prefix.mxf > $tmpdir/$prefix.md5 &&
        diff $tmpdir/$prefix.md5 $base/$prefix.md5
}

create_data()
{
    create_test_file $prefix &&
        $md5tool < $tmpdir/$prefix.mxf > $base/$prefix.md5
}

create_sample()
{
    create_test_file ardzdfhdf_$prefix &&
        mv $tmpdir/ardzdfhdf_$prefix.mxf $sampledir
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
