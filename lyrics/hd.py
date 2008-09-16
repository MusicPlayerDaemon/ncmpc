#!/usr/bin/python
#
# Load lyrics from the user's home directory
#
# Author: Max Kellermann <max@duempel.org>
#

from sys import argv, exit, stdout
from os import environ
from os.path import expanduser

path = expanduser("~/.lyrics/%s - %s.txt" % (argv[1], argv[2]))
try:
    f = file(path)
except IOError:
    exit(2)

while True:
    x = f.read(4096)
    if not x:
        break
    stdout.write(x)
