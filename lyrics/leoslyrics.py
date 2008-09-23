#!/usr/bin/python
#
# Load lyrics from leoslyrics.com
#
# Author: Max Kellermann <max@duempel.org>
#

from sys import argv, exit
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
    f = urlopen(url)
    handler = SearchContentHandler()
    parser = make_parser()
    parser.setContentHandler(handler)
    parser.parse(f)
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
    parser.parse(f)
    return handler.text

hid = search(argv[1], argv[2])
if hid is None:
    exit(2)
print lyrics(hid).encode('utf-8')
