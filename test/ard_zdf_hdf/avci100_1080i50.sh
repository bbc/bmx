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
    $testdir/create_test_essence -t 42 -d 3 $tmpdir/audio.pcm
    $testdir/create_test_essence -t 7 -d 3 $tmpdir/video.h264

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        --ard-zdf-hdf \
        -f 25 \
        -y 10:00:00:00 \
        -o $tmpdir/$1.mxf \
        --afd 10 \
        --avci100_1080i $tmpdir/video.h264 \
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
