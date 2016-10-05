#!/bin/sh

for i
do
	mv "$i" "$i.bak" || exit 1
	XMLLINT_INDENT="	" xmllint --format "$i.bak" > "$i"
	rm "$i.bak"
done
