#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/bmxtranswraptest_temp$$
sampledir=/tmp/


create_input_file()
{
    $testdir/create_test_essence -t 42 -d 24 $tmpdir/audio
    $testdir/create_test_essence -t $1 -d 24 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t $3 \
        -f 25 \
        -y 09:58:00:00 \
        -o $tmpdir/input.mxf \
        --afd 10 -a 16:9 --$2 $tmpdir/video \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        -q 24 --locked true --pcm $tmpdir/audio \
        >/dev/null
}

create_output_file()
{
    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t $1 \
        -o $2 \
        $tmpdir/input.mxf \
        >/dev/null
}


check()
{
    create_input_file $1 $2 $3 &&
        create_output_file $4 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/$2_$3_$4.md5
}

create_data()
{
    create_input_file $1 $2 $3 &&
        create_output_file $4 $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/$2_$3_$4.md5
}

create_samples()
{
    create_input_file $1 $2 $3 &&
        create_output_file $4 $sampledir/bmxtranswrap_$2_$3_$4.mxf
}


check_all()
{
    check 7 avci100_1080i op1a op1a &&
        check 11 d10_50 d10 op1a &&
        check 11 d10_50 op1a d10
}

create_data_all()
{
    create_data 7 avci100_1080i op1a op1a &&
        create_data 11 d10_50 d10 op1a &&
        create_data 11 d10_50 op1a d10
}

create_samples_all()
{
    create_samples 7 avci100_1080i op1a op1a &&
        create_samples 11 d10_50 d10 op1a &&
        create_samples 11 d10_50 op1a d10
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
