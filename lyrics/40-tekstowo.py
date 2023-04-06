#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The Music Player Daemon Project

#
# Load lyrics from tekstowo.pl if lyrics weren't found in the lyrics directory
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


artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [artist.removeprefix("the ") for artist in artists.split(",")]

title = re.sub("\(.*\)", "", title)

for artist in artists:
    r = requests.get(
        f"https://www.tekstowo.pl/wyszukaj.html?search-artist={artist}&search-title={title}"
    )
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    results = soup.find(attrs={"class": "card-body p-0"}).select("a")
    matches = [
        x
        for x in results
        if re.search(title, normalize_parameter(x.text).split(" - ")[1])
    ]
    if not matches:
        print("Lyrics not found :(", file=sys.stderr)
        exit(1)

    # first match is a good match
    song_url = f'https://www.tekstowo.pl{matches[0].get("href")}'
    r = requests.get(song_url)
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, "lxml")
    print(soup.select_one("#songText .inner-text").text)
