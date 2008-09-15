/*
 * $Id$
 *
 * This file is based on the 'Grand digital clock' (gdc.c) shipped with 
 * ncurses. 
 */

#include "config.h"

#ifndef  DISABLE_CLOCK_SCREEN
#include "ncmpc.h"
#include "mpdclient.h"
#include "options.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "gcc.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>


#define YDEPTH	5
#define BUFSIZE 64

#define ENABLE_SECONDS 0

static window_t win;
static gboolean enable_seconds = ENABLE_SECONDS;

/* orginal variables from gdc.c */
static short disp[11] =
	{
		075557, 011111, 071747, 071717, 055711,
		074717, 074757, 071111, 075757, 075717, 002020
	};

static long older[6], next[6], newer[6], mask;

static int YBASE = 10;
static int XBASE = 10;
static int XLENGTH = 54;

/* orginal functions */
static void
set(int t, int n)
{
	int i, m;

	m = 7 << n;
	for (i = 0; i < 5; i++) {
		next[i] |= ((disp[t] >> ((4 - i) * 3)) & 07) << n;
		mask |= (next[i] ^ older[i]) & m;
	}
	if (mask & m)
		mask |= m;
}

static void
drawbox(void)
{
	chtype bottom[XLENGTH + 1];
	int n;

	mvwaddch(win.w, YBASE - 1, XBASE - 1, ACS_ULCORNER);
	whline(win.w, ACS_HLINE, XLENGTH);
	mvwaddch(win.w, YBASE - 1, XBASE + XLENGTH, ACS_URCORNER);

	mvwaddch(win.w, YBASE + YDEPTH, XBASE - 1, ACS_LLCORNER);
	mvwinchnstr(win.w, YBASE + YDEPTH, XBASE, bottom, XLENGTH);
	for (n = 0; n < XLENGTH; n++)
		bottom[n] = ACS_HLINE | (bottom[n] & (A_ATTRIBUTES | A_COLOR));
	mvwaddchnstr(win.w, YBASE + YDEPTH, XBASE, bottom, XLENGTH);
	mvwaddch(win.w, YBASE + YDEPTH, XBASE + XLENGTH, ACS_LRCORNER);

	wmove(win.w, YBASE, XBASE - 1);
	wvline(win.w, ACS_VLINE, YDEPTH);

	wmove(win.w, YBASE, XBASE + XLENGTH);
	wvline(win.w, ACS_VLINE, YDEPTH);
}


static void
standt(int on)
{
	if (on)
		wattron(win.w, A_REVERSE);
	else
		wattroff(win.w, A_REVERSE);
}

/* ncmpc screen functions */

static void
clock_resize(int cols, int rows)
{
	int j;

	for (j = 0; j < 5; j++)
		older[j] = newer[j] = next[j] = 0;

	win.cols = cols;
	win.rows = rows;

	if (cols < 60)
		enable_seconds = FALSE;
	else
		enable_seconds = ENABLE_SECONDS;

	if (enable_seconds)
		XLENGTH = 54;
	else
		XLENGTH = 54-18;


	XBASE = (cols-XLENGTH)/2;
	YBASE = (rows-YDEPTH)/2-(YDEPTH/2)+2;
}

static void
clock_init(WINDOW *w, int cols, int rows)
{
	win.w = w;
	clock_resize(cols, rows);
}

static void
clock_exit(void)
{
}

static void
clock_open(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	int j;

	for (j = 0; j < 5; j++)
		older[j] = newer[j] = next[j] = 0;

}

static void
clock_close(void)
{
}

static const char *
clock_title(mpd_unused char *str, mpd_unused size_t size)
{
	return _("Clock");
}

static void
clock_update(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	time_t now;
	struct tm *tm;
	long t, a;
	int i, j, s, k;
	char buf[BUFSIZE];

	time(&now);
	tm = localtime(&now);

	if (win.rows<=YDEPTH+1 || win.cols<=XLENGTH+1) {
		strftime(buf, BUFSIZE, "%X ",tm);
		mvwaddstr(win.w, win.rows ? win.rows/2:0, (win.cols-strlen(buf))/2, buf);
		wrefresh(win.w);
		return;
	}

	mask = 0;
	set(tm->tm_sec % 10, 0);
	set(tm->tm_sec / 10, 4);
	set(tm->tm_min % 10, 10);
	set(tm->tm_min / 10, 14);
	set(tm->tm_hour % 10, 20);
	set(tm->tm_hour / 10, 24);
	set(10, 7);
	set(10, 17);

	for (k = 0; k < 6; k++) {
		newer[k] = (newer[k] & ~mask) | (next[k] & mask);
		next[k] = 0;
		for (s = 1; s >= 0; s--) {
			standt(s);
			for (i = 0; i < 6; i++) {
				if ((a = (newer[i] ^ older[i]) & (s ? newer : older)[i])
				    != 0) {
					for (j = 0, t = 1 << 26; t; t >>= 1, j++) {
						if (a & t) {
							if (!(a & (t << 1))) {
								wmove(win.w, YBASE + i, XBASE + 2 * j);
							}
							if( enable_seconds || j<18 )
								waddstr(win.w, "  ");
						}
					}
				}
				if (!s) {
					older[i] = newer[i];
				}
			}
			if (!s) {
				wrefresh(win.w);
			}
		}
	}

#ifdef HAVE_LOCALE_H
	strftime(buf, BUFSIZE, "%x", tm);
# else
	/* this depends on the detailed format of ctime(3) */
	strcpy(buf, ctime(&now));
	strcpy(buf + 10, buf + 19);
#endif
	mvwaddstr(win.w, YBASE+YDEPTH+1, (win.cols-strlen(buf))/2, buf);

	wmove(win.w, 6, 0);
	drawbox();
	wrefresh(win.w);
}

static void
clock_paint(screen_t *screen, mpdclient_t *c)
{
	/* this seems to be a better way to clear the window than wclear() ?! */
	wmove(win.w, 0, 0);
	wclrtobot(win.w);
	clock_update(screen, c);
}

static int
clock_cmd(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c,
	  mpd_unused command_t cmd)
{
	return 0;
}

screen_functions_t *
get_screen_clock(void)
{
	static screen_functions_t functions;

	memset(&functions, 0, sizeof(screen_functions_t));
	functions.init = clock_init;
	functions.exit = clock_exit;
	functions.open = clock_open;
	functions.close = clock_close;
	functions.resize = clock_resize;
	functions.paint = clock_paint;
	functions.update = clock_update;
	functions.cmd = clock_cmd;
	functions.get_lw = NULL;
	functions.get_title = clock_title;

	return &functions;
}

#endif
