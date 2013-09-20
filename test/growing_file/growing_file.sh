#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

appsdir=../../apps
testdir=..
tmpdir=/tmp/gf_temp$$

testpcm="$tmpdir/pcm.raw"
testm2v="$tmpdir/test_in.raw"
testmxf="$tmpdir/gftest.mxf"
testrawtxt="$tmpdir/gftest_raw.txt"
testtxt="$tmpdir/gftest.txt"
testmd5="$tmpdir/gftest.md5"

sampletxt="/tmp/sample_gftest.txt"

md5file="$base/growing_file.md5"
gftxt="$base/growing_file.txt"


create_test_file()
{
    $testdir/create_test_essence -t 1 -d 24 $testpcm
    $testdir/create_test_essence -t 14 -d 24 $testm2v
    $appsdir/raw2bmx/raw2bmx --regtest -t op1a -o $testmxf --single-pass --part 10 --mpeg2lg_422p_hl_1080i $testm2v -q 16 --pcm $testpcm >/dev/null
}

read_file()
{
    $appsdir/mxf2raw/mxf2raw --regtest --track-chksum md5 $testmxf
}

calc_md5()
{
    $md5tool < $1 > $2
}

run_test()
{
    # full size,
    # RIP, footer, index body partition,
    # content package, audio element, content packages,
    # partial index segment, index segment,
    # no essence,
    # incomplete header
    truncate_lengths="5941560 \
                      5941464 5927832 5927350 \
                      5683470 5679610 2970790 \
                      2970328 2970308 \
                      13592 \
                      4866"

    rm -f $testrawtxt
    for tlen in $truncate_lengths ; do
        echo $tlen >>$testrawtxt
        $testdir/file_truncate $tlen $testmxf && read_file >>$testrawtxt 2>&1
    done

    sed 's/at\ .*//g' $testrawtxt | sed 's/in\ .*//g' | sed 's/near\ .*//g' | sed "s:$tmpdir:/tmp:g" >$testtxt

    calc_md5 $testtxt $1
}


check()
{
    run_test $testmd5 && diff $testmd5 $md5file >/dev/null
}

create_data()
{
    run_test $md5file && cp $testtxt $gftxt
}

create_sample()
{
    out=$1
    if test -z $out ; then
        out=$sampletxt
    fi
    run_test $testmd5 && cp $testtxt $out
}


mkdir -p $tmpdir

create_test_file

if test -z "$1" ; then
    check
elif test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_sample" ; then
    create_sample $2
fi
res=$?

rm -Rf $tmpdir

exit $res

