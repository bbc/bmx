#!/bin/sh


rm -Rf p2testdrive

mkdir p2testdrive
mkdir p2testdrive/CONTENTS
mkdir p2testdrive/CONTENTS/CLIP
mkdir p2testdrive/CONTENTS/AUDIO
mkdir p2testdrive/CONTENTS/VIDEO
mkdir p2testdrive/CONTENTS/ICON
mkdir p2testdrive/CONTENTS/PROXY
mkdir p2testdrive/CONTENTS/VOICE

echo "000000" > p2testdrive/LASTCLIP.TXT
echo "1.0" >> p2testdrive/LASTCLIP.TXT
echo "2" >> p2testdrive/LASTCLIP.TXT
