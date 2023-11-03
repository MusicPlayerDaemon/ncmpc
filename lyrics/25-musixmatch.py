#!/usr/bin/env python3
import requests
import bs4
import sys
import html

def normalize(s):
    for i in [' ', '\'']:
        s = s.replace(i, '-')
    for i in [',', '(', ')']:
        s = s.replace(i, '')
    return html.escape(s)

def main():
    artist = normalize(sys.argv[1])
    title = normalize(sys.argv[2])

    try:
        musixmatch_url = "https://www.musixmatch.com/lyrics/"
        r = requests.get(
                musixmatch_url + artist + "/" + title,
                headers = {
                    "Host": "www.musixmatch.com",
                    # emulate a linux user
                    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Safari/537.36",
                    "Accept": "*/*"
                    }
                )

        if r.status_code == 404:
            print("Lyrics not found :(", file=sys.stderr)
            exit(1)

        soup = bs4.BeautifulSoup(r.text, features="lxml")
        p_tags = soup.find_all("p", {"class": "mxm-lyrics__content"})
        if len(p_tags) == 0:
            # Sometimes musixmatch shows a "Restricted Lyrics" page with a 200 status code
            print("Unable to get lyrics :(")
            exit(1)

        for p in p_tags:
            print(p.text)
            exit(1)

    except Exception as e:
        print("Unknown error: ", e, file=sys.stderr)
        exit(2)

if __name__ == '__main__':
    main()
