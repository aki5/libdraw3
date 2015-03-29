#!/bin/bash

for kmap in /usr/share/keymaps/i386/*ert*/[a-z][a-z].kmap.gz; do
	code=`basename $kmap .kmap.gz`
	echo $code
	gunzip -c $kmap \
		| loadkeys --mktable -u \
		| sed \
			-e 's,u_short,unsigned short,g' \
			-e 's,ushort,unsigned short,g' \
			-e 's,\[NR_KEYS\],[256],g' \
			-e 's,\[MAX_NR_KEYMAPS\],[],g' \
			-e 's,^#.*$,,g' \
			-e 's,/\*.*$,,g' \
		| awk '{if($0!="")print $0} /keymap_count = /{exit}' \
		| sed -e 's,key_maps,'$code'_key_maps,g' -e 's,keymap_count,'$code'_keymap_count,g' \
		> $code-keyb.h
done

