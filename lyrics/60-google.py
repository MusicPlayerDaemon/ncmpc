#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The Music Player Daemon Project

#
# Load lyrics from google.com if lyrics weren't found in the lyrics directory
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


base_url = "http://www.google.com/"
artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [artist.removeprefix("the ") for artist in artists.split(",")]

title = re.sub("\(.*\)", "", title)

for artist in artists:
    artist = artist.replace(" ", "+")
    title = title.replace(" ", "+")
    r = requests.get(
        f"{base_url}/search?q={artist}+{title}+lyrics",
        headers={
            "authority": "www.google.com",
            # doesn't seem to work without this
            "user-agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Safari/537.36",
            "accept": "text/html,application/xhtml+xml"
        }
    )
    try:
        r.raise_for_status()
    except:
        exit(1)
    soup = bs4.BeautifulSoup(r.text, "html5lib")
    try:
        lyrics_container = soup.select_one("div[data-lyricid]")
        if not lyrics_container: raise IndexError
        for tag in lyrics_container:
            lyrics = tag.find_all("span")
            for lyric in lyrics:
                print(lyric.text)
            break
    except IndexError:
        print("Lyrics not found :(")
        exit(1)
    except Exception as e:
        print("Unknown error: ", e)
        exit(2)
