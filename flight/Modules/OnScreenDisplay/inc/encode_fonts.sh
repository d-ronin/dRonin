#!/bin/bash

# The first 3 fonts are the small / medium / large fonts selectable by the user
./encode_fonts.py -s 32 -l 127 -o ../fonts.c -d fonts.h font8x10.png font_outlined8x14.png font12x18.png font_outlined8x8.png  