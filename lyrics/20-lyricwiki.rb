#!/usr/bin/env ruby
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
# Load lyrics from lyrics.wikia.com, formerly lyricwiki.org
#

require 'uri'
require 'net/http'
require 'cgi'
require 'iconv'

# We need this because URI.escape doesn't escape ampersands.
def escape(string)
	new = URI.escape(string)
	new.gsub(/&/, "%26")
end

url = "http://lyrics.wikia.com/api.php?action=lyrics&fmt=xml&func=getSong" + \
    "&artist=#{escape(ARGV[0])}&song=#{escape(ARGV[1])}"
response = Net::HTTP.get(URI.parse(url))

if not response =~ /<url>\s*(.*?)\s*<\/url>/im
	$stderr.puts "No URL in response!"
	exit(1)
end

url = $1
exit(69) if url =~ /action=edit$/

response = Net::HTTP.get(URI.parse(url))
if not response =~ /<div class='lyricbox'>\s*(.*?)\s*<!--/im
	$stderr.puts "No <div class='lyricbox'> in lyrics page!\n"
	exit(1)
end

if not $1 =~ /^.*<\/div>(.*?)$/im
	$stderr.puts "Couldn't remove leading XML tags in lyricbox!\n"
	exit(1)
end

# lyrics come in Latin1, but we need UTF-8
lyrics_latin1 = CGI::unescapeHTML($1.gsub(/<br \/>/, "\n"))
puts Iconv.conv('UTF-8//TRANSLIT//IGNORE', 'Latin1', lyrics_latin1)
