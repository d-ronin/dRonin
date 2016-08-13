#!/bin/bash

# $1 = build dir
# $2 = debug symbols dir (to copy into)

reldir() {
	python -c "import os.path; print os.path.dirname(os.path.relpath('$1','${2:-$PWD}'))" ;
}

movedsym() {
	echo "Moving $3" ;
	mkdir -p $2/`reldir $3 $1` ;
	mv -f $3 $_ ;
}

rm -rf $2/*

find $1 -type d -name "*.dSYM" | while read fname; do
    movedsym $1 $2 $fname
done

# work around Qt funkiness to get main app debug symbols in the right place
if [ -d "$2/dRonin-GCS.app.dSYM" ]; then
	mv -f "$2/dRonin-GCS.app.dSYM" "$2/dRonin-GCS.app/Contents/MacOS/dRonin-GCS.dSYM"
fi
