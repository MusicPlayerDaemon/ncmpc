#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The Music Player Daemon Project

#
# Load lyrics from karaoketexty.sk if lyrics weren't found in the lyrics directory
#
import bs4
import re
import requests
import sys

try:
    # using the "unidecode" library if installed
    # (https://pypi.org/project/Unidecode/ or package
    # "python3-unidecode" on Debian)
    from unidecode import unidecode
except ImportError:
    # Dummy fallback (don't crash if "unidecode" is not installed)
    def unidecode(s):
        return s


def normalize_parameter(s):
    return unidecode(s).lower()


base_url = "https://www.karaoketexty.sk"
artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [artist.removeprefix("the ") for artist in artists.split(",")]

title = re.sub("\(.*\)", "", title)

for artist in artists:
    r = requests.get(f"{base_url}/search?sid=outxq&q={title}")
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    results = soup.findAll(attrs={"class": "h2_search"})[1]
    matches = [
        x.a for x in results.findAllNext("span", attrs={"class": "searchresrow_songs"})
    ]
    matches = [
        x for x in matches if f"{artist} - {title}" == normalize_parameter(x.text)
    ]
    if not matches:
        print("Lyrics not found :(", file=sys.stderr)
        exit(1)

    # first match is a good match
    song_url = f'{base_url}{matches[0].get("href")}'
    r = requests.get(song_url)
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    lyrics_block = soup.find(attrs={"class": "lyrics_cont"}).findAll(
        attrs={"class": "para_row"}
    )
    lyrics_paragraphs = [x.findAll("span") for x in lyrics_block]
    lyrics = "".join(
        [
            x.text
            for x in soup.find(attrs={"class": "lyrics_cont"}).findAll(
                attrs={"class": "para_row"}
            )
        ]
    )
    print("\n".join(["\n".join([x.text for x in x]) for x in lyrics_paragraphs]))
