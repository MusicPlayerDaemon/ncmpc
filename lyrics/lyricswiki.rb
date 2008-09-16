#!/usr/bin/env ruby
#
# Load lyrics from lyricswiki.org
#
# Author: Max Kellermann <max@duempel.org>
#

require 'uri'
require 'net/http'

url = "http://lyricwiki.org/api.php" + \
    "?artist=#{URI.escape(ARGV[0])}&song=#{URI.escape(ARGV[1])}"
response = Net::HTTP.get(URI.parse(url))

exit(2) unless response =~ /<pre>\s*(.*?)\s*<\/pre>/im
puts $1
