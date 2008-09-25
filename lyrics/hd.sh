#!/bin/sh -e
#
# Load lyrics from the user's home directory
#
# Author: Max Kellermann <max@duempel.org>
#

cat ~/.lyrics/"$1 - $2".txt 2>/dev/null
