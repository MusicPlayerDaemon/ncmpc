#!/bin/bash

set -e

search="http://api.leoslyrics.com/api_search.php?auth=ncmpc"
lyrics="http://api.leoslyrics.com/api_lyrics.php?auth=ncmpc"
cache="$HOME/.lyrics/$1 - $2.txt"

hid=$(wget -q -O- "$search&artist=$1&songtitle=$2" |
	sed -n 's/.*hid="\([^"]*\)".*exactMatch="true".*/\1/p' |
	head -n 1)

test "$hid"

mkdir -p "$(dirname "$cache")"

wget -q -O- "$lyrics&hid=$hid" | awk '
	/<text>/   { go=1; sub(".*<text>", "")  };
	/<\/text>/ { go=0; sub("</text>.*", "") };
	go         { sub("&#xD;", ""); print    };
' | tee "$cache"
