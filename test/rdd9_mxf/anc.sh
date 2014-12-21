#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

appsdir=../../apps
testdir=..
tmpdir=/tmp/anc_temp$$

testpcm="$tmpdir/test_pcm.raw"
testm2v="$tmpdir/test_m2v.raw"
testanc="$tmpdir/test_anc.raw"
testmxf="$tmpdir/test.mxf"
testmd5="$tmpdir/test.md5"

sample="/tmp/rdd9_anc.mxf"

md5file="$base/anc.md5"


create_test_file()
{
    $testdir/create_test_essence -t 1 -d 24 $testpcm
    $testdir/create_test_essence -t 14 -d 24 $testm2v
    $testdir/create_test_essence -t 43 -d 24 $testanc
    $appsdir/raw2bmx/raw2bmx --regtest -t rdd9 -o $testmxf --anc-const 24 --anc $testanc --mpeg2lg_422p_hl_1080i $testm2v -q 16 --pcm $testpcm -q 16 --pcm $testpcm >/dev/null
}

calc_md5()
{
    $md5tool < $1 > $2
}

run_test()
{
    calc_md5 $testmxf $1
}


check()
{
    run_test $testmd5 && diff $testmd5 $md5file >/dev/null
}

create_data()
{
    run_test $testmd5 && cp $testmd5 $md5file
}

create_sample()
{
    mv $testmxf $sample
}


mkdir -p $tmpdir

create_test_file

if test -z "$1" ; then
    check
elif test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_sample" ; then
    create_sample
fi
res=$?

rm -Rf $tmpdir

exit $res
