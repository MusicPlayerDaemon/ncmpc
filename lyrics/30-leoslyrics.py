#!/usr/bin/python
#
#  (c) 2004-2008 The Music Player Daemon Project
#  http://www.musicpd.org/
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

#
# Load lyrics from leoslyrics.com
#

from sys import argv, exit, stderr
from urllib import urlencode, urlopen
from xml.sax import make_parser, SAXException
from xml.sax.handler import ContentHandler

class SearchContentHandler(ContentHandler):
    def __init__(self):
        self.code = None
        self.hid = None

    def startElement(self, name, attrs):
        if name == 'response':
            self.code = int(attrs['code'])
        elif name == 'result':
            if self.hid is None or attrs['exactMatch'] == 'true':
                self.hid = attrs['hid']

def search(artist, title):
    query = urlencode({'auth': 'ncmpc',
                       'artist': artist,
                       'songtitle': title})
    url = "http://api.leoslyrics.com/api_search.php?" + query
    try:
        f = urlopen(url)
    except IOError:
        stderr.write("Failed to connect to http://api.leoslyrics.com, it seems down!\n")
        exit(1)
    handler = SearchContentHandler()
    parser = make_parser()
    parser.setContentHandler(handler)
    try:
        parser.parse(f)
    except SAXException:
        stderr.write("Failed to parse the search result!\n")
        exit(1)
    return handler.hid

class LyricsContentHandler(ContentHandler):
    def __init__(self):
        self.code = None
        self.is_text = False
        self.text = None

    def startElement(self, name, attrs):
        if name == 'text':
            self.text = ''
            self.is_text = True
        else:
            self.is_text = False

    def characters(self, chars):
        if self.is_text:
            self.text += chars

def lyrics(hid):
    query = urlencode({'auth': 'ncmpc',
                       'hid': hid})
    url = "http://api.leoslyrics.com/api_lyrics.php?" + query
    f = urlopen(url)
    handler = LyricsContentHandler()
    parser = make_parser()
    parser.setContentHandler(handler)
    try:
        parser.parse(f)
    except SAXException:
        stderr.write("Failed to parse the lyrics!\n")
        exit(1)
    return handler.text

hid = search(argv[1], argv[2])
if hid is None:
    exit(69)
print lyrics(hid).encode('utf-8').rstrip()
