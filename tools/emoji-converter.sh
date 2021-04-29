#! /bin/bash

for i in `ls joypixels-6.5-free/png/unicode/128`; do
	file=$(echo $i | cut -d '.' -f 1);
	ffmpeg -i joypixels-6.5-free/png/unicode/128/$i -sws_dither bayer -pix_fmt rgb565 -s 30x30 -y emojis/$file.bmp
done
