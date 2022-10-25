#!/bin/sh

base=$(dirname $0)

md5tool=../file_md5

testdir=..
appsdir=../../apps
tmpdir=/tmp/wave_adm_temp$$
sampledir=/tmp/

create_mxf_1()
{
    # Create an MXF from a Wave+ADM and AVC-Intra.
    # Map the audio channels to 2 stereo tracks,
    # with the stereo tracks in reverse order.

    $testdir/create_test_essence -t 7 -d 1 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --track-map "2,3;0,1" \
        --avci100_1080i $tmpdir/video \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null
}

create_mxf_2()
{
    # Create an MXF from a Wave+ADM and AVC-Intra.
    # Map only audio 2 channels to a stereo track,
    # with the channels in reverse order.

    $testdir/create_test_essence -t 7 -d 1 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --track-map "3,2" \
        --avci100_1080i $tmpdir/video \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null
}

create_mxf_3()
{
    # Create an MXF from a Wave+ADM and AVC-Intra.
    # Map only audio 2 channels to 2 stereo tracks,
    # with a silence channel in each track.

    $testdir/create_test_essence -t 7 -d 1 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --track-map "0,s1;s1,1" \
        --avci100_1080i $tmpdir/video \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null
}

create_wave_1()
{
    # Create a Wave file from mxf_1 and map the channels
    # back into the original order in adm_1.wav.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        --track-map "2,3,0,1" \
        $1 \
        >/dev/null
}

create_wave_2()
{
    # Create a Wave file from mxf_1 and only map 2 channels back
    # into the original order in adm_1.wav.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        --track-map "2,3" \
        $1 \
        >/dev/null
}

create_wave_3()
{
    # Create a Wave file from mxf_3, including the 2 silence channels,
    # and map the 2 channels back into the original order in adm_1.wav.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        --track-map "0,3,1,2" \
        $1 \
        >/dev/null
}

create_wave_4()
{
    # Create a Wave file from mxf_3 with no ADM channels mapped.

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        --track-map "1,2" \
        $1 \
        >/dev/null
}

create_wave_5()
{
    # Create a Wave file from the sample Wave+ADM file.

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t wave \
        -o $1 \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null
}

create_wave_6()
{
    # Create a Wave file from test generated content, an axml chunk file and a chna text file.

    $testdir/create_test_essence -t 42 -d 1 -s 1 $tmpdir/audio1.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 2 $tmpdir/audio2.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 3 $tmpdir/audio3.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 4 $tmpdir/audio4.pcm

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t wave \
        -o $3 \
        --wave-chunk-data $1 axml \
        --chna-audio-ids $2 \
        -s 48000 -q 24 --pcm $tmpdir/audio1.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio2.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio3.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio4.pcm \
        >/dev/null
}

create_wave_7()
{
    # Create a Wave file from test generated content and a chna text file.

    $testdir/create_test_essence -t 42 -d 1 -s 1 $tmpdir/audio1.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 2 $tmpdir/audio2.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 3 $tmpdir/audio3.pcm
    $testdir/create_test_essence -t 42 -d 1 -s 4 $tmpdir/audio4.pcm

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t wave \
        -o $2 \
        --chna-audio-ids $1 \
        -s 48000 -q 24 --pcm $tmpdir/audio1.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio2.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio3.pcm \
        -s 48000 -q 24 --pcm $tmpdir/audio4.pcm \
        >/dev/null
}

create_mxf_4()
{
    # Create an MXF from wave_3 including only silence (non-ADM) channels.

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --track-map "2,3" \
        --wave $1 \
        >/dev/null
}

create_mxf_5()
{
    # Create an MXF from a Wave+ADM and AVC-Intra.
    # Map the audio channels to 2 stereo tracks
    # Add ADM MCA labels from mca_1.txt

    $testdir/create_test_essence -t 7 -d 1 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --track-map "0,1;2,3" \
        --track-mca-labels x $base/mca_1.txt \
        --audio-layout adm \
        --avci100_1080i $tmpdir/video \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $2 \
        --mca-detail \
        $1
}

create_mxf_6()
{
    # Create an MXF from a Wave+ADM and AVC-Intra.
    # Map the audio channels to mono, mono and stereo tracks
    # Add ADM MCA labels from mca_2.txt

    $testdir/create_test_essence -t 7 -d 1 $tmpdir/video

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -f 25 \
        -o $1 \
        --track-map "0;1;2,3" \
        --track-mca-labels x $base/mca_2.txt \
        --audio-layout adm \
        --avci100_1080i $tmpdir/video \
        --wave-chunks axml \
        --wave $base/adm_1.wav \
        >/dev/null

    $appsdir/mxf2raw/mxf2raw \
        --regtest \
        --info \
        --info-format xml \
        --info-file $2 \
        --mca-detail \
        $1
}

create_mxf_7()
{
    # Create an MXF from wave_7

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --wave-chunks axml \
        --wave $1 \
        >/dev/null
}

create_wave_8()
{
    # Create a Wave file from mxf_7

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        $1 \
        >/dev/null
}

create_mxf_8()
{
    # Create a multi-channel track MXF from wave_3

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --track-map singlemca \
        --wave-chunks axml \
        --wave $1 \
        >/dev/null
}

create_wave_9()
{
    # Create a Wave file from mxf_8

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        $1 \
        >/dev/null
}

create_mxf_9()
{
    # Create mono track MXF from wave_3

    $appsdir/raw2bmx/raw2bmx \
        --regtest \
        -t op1a \
        -o $2 \
        --track-map mono \
        --wave-chunks axml \
        --wave $1 \
        >/dev/null
}

create_wave_10()
{
    # Create a Wave file from mxf_9

    $appsdir/bmxtranswrap/bmxtranswrap \
        --regtest \
        -t wave \
        -o $2 \
        $1 \
        >/dev/null
}


check()
{
    create_mxf_1 $tmpdir/test_mxf_1.mxf &&
        $md5tool < $tmpdir/test_mxf_1.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_1.md5 &&
    create_mxf_2 $tmpdir/test_mxf_2.mxf &&
        $md5tool < $tmpdir/test_mxf_2.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_2.md5 &&
    create_mxf_3 $tmpdir/test_mxf_3.mxf &&
        $md5tool < $tmpdir/test_mxf_3.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_3.md5 &&
    create_wave_1 $tmpdir/test_mxf_1.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_1.md5 &&
        diff $tmpdir/test.wav $base/adm_1.wav &&
    create_wave_2 $tmpdir/test_mxf_1.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_2.md5 &&
    create_wave_3 $tmpdir/test_mxf_3.mxf $tmpdir/test_wave_3.wav &&
        $md5tool < $tmpdir/test_wave_3.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_3.md5 &&
    create_wave_4 $tmpdir/test_mxf_3.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_4.md5 &&
    create_wave_5 $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_5.md5 &&
        diff $tmpdir/test.wav $base/adm_1.wav &&
    create_wave_6 $base/axml_1.xml $base/chna_1.txt $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_6.md5 &&
    create_mxf_4 $tmpdir/test_wave_3.wav $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_4.md5 &&
    create_mxf_5 $tmpdir/test.mxf $tmpdir/test.xml &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_5.md5 &&
        diff $tmpdir/test.xml $base/test_mxf_5.xml &&
    create_mxf_6 $tmpdir/test.mxf $tmpdir/test.xml &&
        $md5tool < $tmpdir/test.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_6.md5 &&
        diff $tmpdir/test.xml $base/test_mxf_6.xml &&
    create_wave_7 $base/chna_1.txt $tmpdir/test_wave_7.wav &&
        $md5tool < $tmpdir/test_wave_7.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_7.md5 &&
    create_mxf_7 $tmpdir/test_wave_7.wav $tmpdir/test_mxf_7.mxf &&
        $md5tool < $tmpdir/test_mxf_7.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_7.md5 &&
    create_wave_8 $tmpdir/test_mxf_7.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_8.md5 &&
    create_mxf_8 $tmpdir/test_wave_3.wav $tmpdir/test_mxf_8.mxf &&
        $md5tool < $tmpdir/test_mxf_8.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_8.md5 &&
    create_wave_9 $tmpdir/test_mxf_8.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_9.md5 &&
        diff $tmpdir/test.wav $tmpdir/test_wave_3.wav &&
    create_mxf_9 $tmpdir/test_wave_3.wav $tmpdir/test_mxf_9.mxf &&
        $md5tool < $tmpdir/test_mxf_9.mxf > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_mxf_9.md5 &&
    create_wave_10 $tmpdir/test_mxf_9.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $tmpdir/test.md5 &&
        diff $tmpdir/test.md5 $base/test_wave_10.md5 &&
        diff $tmpdir/test.wav $tmpdir/test_wave_3.wav
}

create_data()
{
    create_mxf_1 $tmpdir/test_mxf_1.mxf &&
        $md5tool < $tmpdir/test_mxf_1.mxf > $base/test_mxf_1.md5 &&
    create_mxf_2 $tmpdir/test_mxf_2.mxf &&
        $md5tool < $tmpdir/test_mxf_2.mxf > $base/test_mxf_2.md5 &&
    create_mxf_3 $tmpdir/test_mxf_3.mxf &&
        $md5tool < $tmpdir/test_mxf_3.mxf > $base/test_mxf_3.md5 &&
    create_wave_1 $tmpdir/test_mxf_1.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_1.md5 &&
    create_wave_2 $tmpdir/test_mxf_1.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_2.md5 &&
    create_wave_3 $tmpdir/test_mxf_3.mxf $tmpdir/test_wave_3.wav &&
        $md5tool < $tmpdir/test_wave_3.wav > $base/test_wave_3.md5 &&
    create_wave_4 $tmpdir/test_mxf_3.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_4.md5 &&
    create_wave_5 $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_5.md5 &&
    create_wave_6 $base/axml_1.xml $base/chna_1.txt $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_6.md5 &&
    create_mxf_4 $tmpdir/test_wave_3.wav $tmpdir/test.mxf &&
        $md5tool < $tmpdir/test.mxf > $base/test_mxf_4.md5 &&
    create_mxf_5 $tmpdir/test.mxf $base/test_mxf_5.xml &&
        $md5tool < $tmpdir/test.mxf > $base/test_mxf_5.md5 &&
    create_mxf_6 $tmpdir/test.mxf $base/test_mxf_6.xml &&
        $md5tool < $tmpdir/test.mxf > $base/test_mxf_6.md5 &&
    create_wave_7 $base/chna_1.txt $tmpdir/test_wave_7.wav &&
        $md5tool < $tmpdir/test_wave_7.wav > $base/test_wave_7.md5 &&
    create_mxf_7 $tmpdir/test_wave_7.wav $tmpdir/test_mxf_7.mxf &&
        $md5tool < $tmpdir/test_mxf_7.mxf > $base/test_mxf_7.md5 &&
    create_wave_8 $tmpdir/test_mxf_7.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_8.md5 &&
    create_mxf_8 $tmpdir/test_wave_3.wav $tmpdir/test_mxf_8.mxf &&
        $md5tool < $tmpdir/test_mxf_8.mxf > $base/test_mxf_8.md5 &&
    create_wave_9 $tmpdir/test_mxf_8.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_9.md5 &&
    create_mxf_9 $tmpdir/test_wave_3.wav $tmpdir/test_mxf_9.mxf &&
        $md5tool < $tmpdir/test_mxf_9.mxf > $base/test_mxf_9.md5 &&
    create_wave_10 $tmpdir/test_mxf_9.mxf $tmpdir/test.wav &&
        $md5tool < $tmpdir/test.wav > $base/test_wave_10.md5
}

create_samples()
{
    create_mxf_1 $sampledir/test_mxf_1.mxf &&
    create_mxf_2 $sampledir/test_mxf_2.mxf &&
    create_mxf_3 $sampledir/test_mxf_3.mxf &&
    create_wave_1 $sampledir/test_mxf_1.mxf $sampledir/test_wave_1.wav &&
    create_wave_2 $sampledir/test_mxf_1.mxf $sampledir/test_wave_2.wav &&
    create_wave_3 $sampledir/test_mxf_3.mxf $sampledir/test_wave_3.wav &&
    create_wave_4 $sampledir/test_mxf_3.mxf $sampledir/test_wave_4.wav &&
    create_wave_5 $sampledir/test_wave_5.wav &&
    create_wave_6 $base/axml_1.xml $base/chna_1.txt $sampledir/test_wave_6.wav &&
    create_mxf_4 $sampledir/test_wave_3.wav $sampledir/test_mxf_4.mxf &&
    create_mxf_5 $sampledir/test_mxf_5.mxf $sampledir/test_mxf_5.xml &&
    create_mxf_6 $sampledir/test_mxf_6.mxf $sampledir/test_mxf_6.xml &&
    create_wave_7 $base/chna_1.txt $sampledir/test_wave_7.wav &&
    create_mxf_7 $sampledir/test_wave_7.wav $sampledir/test_mxf_7.mxf &&
    create_wave_8 $sampledir/test_mxf_7.mxf $sampledir/test_wave_8.wav &&
    create_mxf_8 $sampledir/test_wave_3.wav $sampledir/test_mxf_8.mxf &&
    create_wave_9 $sampledir/test_mxf_8.mxf $sampledir/test_wave_9.wav &&
    create_mxf_9 $sampledir/test_wave_3.wav $sampledir/test_mxf_9.mxf &&
    create_wave_10 $sampledir/test_mxf_9.mxf $sampledir/test_wave_10.wav
}


check_all()
{
    check
}

create_data_all()
{
    create_data
}

create_samples_all()
{
    create_samples
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
