#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The Music Player Daemon Project

#
# Load lyrics from zeneszoveg.hu if lyrics weren't found in the lyrics directory
#
import re
import sys
import urllib.request

import requests
import html
import bs4

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


def get_artist_url(artist):
    r = requests.get(f"{base_url}eloadok/{artist[0]}")
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    pages = [
        x["href"] for x in soup.select("div.paginator > a") if len(x.text.strip()) < 3
    ]
    for page in pages:
        r = requests.get(f"{base_url}{page}")
        soup = bs4.BeautifulSoup(r.text, "html5lib")
        match = [
            x
            for x in soup.select(".col2.olist a")
            if normalize_parameter(x.text) == artist
        ]
        if match:
            break
    else:
        print("Artist not found :(", file=sys.stderr)
        exit(1)
    return match[0].get("href")


def get_title_url(artist_url, title):
    r = requests.get(f"{base_url}{artist_url}")
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    match = [
        x.select("a")[-1]["href"]
        for x in soup.select(".artistRelatedList .search-result h3")
        if normalize_parameter(x.text.strip()) == title
    ]
    if not match:
        print("Song not found :(", file=sys.stderr)
        exit(1)
    return match[0]


def get_lyrics(title_url):
    r = requests.get(f"{base_url}{title_url}")
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    lyrics = soup.select_one("div.lyrics-plain-text")
    if not lyrics:
        print("Lyrics not found :(", file=sys.stderr)
        exit(1)
    lyrics = lyrics.text.lstrip()
    return lyrics


artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [artist.removeprefix("the ") for artist in artists.split(",")]

title = re.sub("\(.*\)", "", title)
base_url = "https://kulfoldi.zeneszoveg.hu/"

for artist in artists:
    artist_url = get_artist_url(artist)
    title_url = get_title_url(artist_url, title)
    lyrics = get_lyrics(title_url)
    print(lyrics)
