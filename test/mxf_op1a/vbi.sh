#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

appsdir=../../apps
testdir=..
tmpdir=/tmp/vbi_temp$$

testpcm="$tmpdir/test_pcm.raw"
testavci="$tmpdir/test_avci.raw"
testvbi="$tmpdir/test_vbi.raw"
testmxf="$tmpdir/test.mxf"
testmd5="$tmpdir/test.md5"

sample="/tmp/op1a_vbi.mxf"

md5file="$base/vbi.md5"


create_test_file()
{
    $testdir/create_test_essence -t 1 -d 3 $testpcm
    $testdir/create_test_essence -t 7 -d 3 $testavci
    $testdir/create_test_essence -t 44 -d 3 $testvbi
    $appsdir/raw2bmx/raw2bmx --regtest -t op1a -o $testmxf --vbi-const 24 --vbi $testvbi --avci100_1080i $testavci -q 16 --pcm $testpcm >/dev/null
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

