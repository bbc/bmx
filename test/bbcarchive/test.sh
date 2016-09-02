#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

appsdir=../../apps
tmpdir=/tmp/bbcarchive_temp$$
sampledir=/tmp/


create_file()
{
    ./test_write_archive_mxf --regtest --num-audio 6 --format 1080i25 --16by9 --crc32 3 $1

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --app \
        --app-events dptv \
        $1 \
        > $2
}


check()
{
    create_file $tmpdir/test.mxf $tmpdir/test.xml &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/bbcarchive_1080_25i.md5 &&
        diff $tmpdir/test.xml $base/bbcarchive_1080_25i.xml
}

create_data()
{
    create_file $tmpdir/test.mxf $base/bbcarchive_1080_25i.xml &&
        $md5tool < $tmpdir/test.mxf > $base/bbcarchive_1080_25i.md5
}

create_samples()
{
    create_file $sampledir/bbcarchive_1080_25i.mxf $sampledir/bbcarchive_1080_25i.xml
}



mkdir -p $tmpdir

if test "$1" = "create_data" ; then
    create_data
elif test "$1" = "create_samples" ; then
    create_samples
else
    check
fi
res=$?

rm -Rf $tmpdir

exit $res
