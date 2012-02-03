/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "command.h"
#include "config.h"
#include "ncmpc.h"
#include "i18n.h"
#include "mpdclient.h"
#include "screen.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>

#define KEY_CTL(x) ((x) & 0x1f) /* KEY_CTL(A) == ^A == \1 */

#define BS   KEY_BACKSPACE
#define DEL  KEY_DC
#define UP   KEY_UP
#define DWN  KEY_DOWN
#define LEFT KEY_LEFT
#define RGHT KEY_RIGHT
#define HOME KEY_HOME
#define END  KEY_END
#define PGDN KEY_NPAGE
#define PGUP KEY_PPAGE
#define TAB  0x09
#define STAB 0x161
#define ESC  0x1B
#define RET  '\r'
#define F1   KEY_F(1)
#define F2   KEY_F(2)
#define F3   KEY_F(3)
#define F4   KEY_F(4)
#define F5   KEY_F(5)
#define F6   KEY_F(6)
#define F7   KEY_F(7)
#define F8   KEY_F(8)
#define C(x) KEY_CTL(x)

static command_definition_t cmds[] = {
#ifdef ENABLE_KEYDEF_SCREEN
	{ {'K', 0, 0 }, 0, CMD_SCREEN_KEYDEF, "screen-keyedit",
	  N_("Key configuration screen") },
#endif
	{ { 'q', 'Q', C('C') }, 0, CMD_QUIT, "quit",
	  N_("Quit") },

	/* movement */
	{ { UP, 'k', 0 }, 0, CMD_LIST_PREVIOUS, "up",
	  N_("Move cursor up") },
	{ { DWN, 'j', 0 }, 0, CMD_LIST_NEXT, "down",
	  N_("Move cursor down") },
	{ { 'H', 0, 0 }, 0, CMD_LIST_TOP, "top",
	  N_("Move cursor to the top of screen") },
	{ { 'M', 0, 0 }, 0, CMD_LIST_MIDDLE, "middle",
	  N_("Move cursor to the middle of screen") },
	{ { 'L', 0, 0 }, 0, CMD_LIST_BOTTOM, "bottom",
	  N_("Move cursor to the bottom of screen") },
	{ { HOME, C('A'), 0 }, 0, CMD_LIST_FIRST, "home",
	  N_("Move cursor to the top of the list") },
	{ { END, C('E'), 0 }, 0, CMD_LIST_LAST, "end",
	  N_("Move cursor to the bottom of the list") },
	{ { PGUP, 0, 0 }, 0, CMD_LIST_PREVIOUS_PAGE, "pgup",
	  N_("Page up") },
	{ { PGDN, 0, 0 }, 0, CMD_LIST_NEXT_PAGE, "pgdn",
	  N_("Page down") },
	{ { 'v',  0, 0 }, 0, CMD_LIST_RANGE_SELECT, "range-select",
	  N_("Range selection") },
	{ { C('N'),  0, 0 }, 0, CMD_LIST_SCROLL_DOWN_LINE, "scroll-down-line",
	  N_("Scroll up one line") },
	{ { C('B'),  0, 0 }, 0, CMD_LIST_SCROLL_UP_LINE, "scroll-up-line",
	  N_("Scroll down one line") },
	{ { 'N',  0, 0 }, 0, CMD_LIST_SCROLL_DOWN_HALF, "scroll-down-half",
	  N_("Scroll up half a screen") },
	{ { 'B',  0, 0 }, 0, CMD_LIST_SCROLL_UP_HALF, "scroll-up-half",
	  N_("Scroll down half a screen") },
	{ { 'l', 0, 0 }, 0, CMD_SELECT_PLAYING, "select-playing",
	  N_("Select currently playing song") },


	/* basic screens */
	{ { '1', F1, 'h' }, 0, CMD_SCREEN_HELP, "screen-help",
	  N_("Help screen") },
	{ { '2', F2, 0 }, 0, CMD_SCREEN_PLAY, "screen-playlist",
	  N_("Playlist screen") },
	{ { '3', F3, 0 }, 0, CMD_SCREEN_FILE, "screen-browse",
	  N_("Browse screen") },


	/* player commands */
	{ { RET, 0, 0 }, 0, CMD_PLAY, "play",
	  N_("Play/Enter directory") },
	{ { 'P', 0, 0 }, 0, CMD_PAUSE,"pause",
	  N_("Pause") },
	{ { 's', BS, 0 }, 0, CMD_STOP, "stop",
	  N_("Stop") },
	{ { 'o', 0, 0 }, 0, CMD_CROP, "crop",
	  N_("Crop") },
	{ { '>', 0, 0 }, 0, CMD_TRACK_NEXT, "next",
	  N_("Next track") },
	{ { '<', 0, 0 }, 0, CMD_TRACK_PREVIOUS, "prev",
	  N_("Previous track") },
	{ { 'f', 0, 0 }, 0, CMD_SEEK_FORWARD, "seek-forward",
	  N_("Seek forward") },
	{ { 'b', 0, 0 }, 0, CMD_SEEK_BACKWARD, "seek-backward",
	  N_("Seek backward") },
	{ { '+', RGHT, 0 }, 0, CMD_VOLUME_UP, "volume-up",
	  N_("Increase volume") },
	{ { '-', LEFT, 0 }, 0, CMD_VOLUME_DOWN, "volume-down",
	  N_("Decrease volume") },
	{ { ' ', 0, 0 }, 0, CMD_SELECT, "select",
	  N_("Select/deselect song in playlist") },
	{ { 't', 0, 0 }, 0, CMD_SELECT_ALL, "select_all",
	  N_("Select all listed items") },
	{ { DEL, 'd', 0 }, 0, CMD_DELETE, "delete",
	  N_("Delete song from playlist") },
	{ { 'Z', 0, 0 }, 0, CMD_SHUFFLE, "shuffle",
	  N_("Shuffle playlist") },
	{ { 'c', 0, 0 }, 0, CMD_CLEAR, "clear",
	  N_("Clear playlist") },
	{ { 'r', 0, 0 }, 0, CMD_REPEAT, "repeat",
	  N_("Toggle repeat mode") },
	{ { 'z', 0, 0 }, 0, CMD_RANDOM, "random",
	  N_("Toggle random mode") },
	{ { 'y', 0, 0 }, 0, CMD_SINGLE, "single",
	  N_("Toggle single mode") },
	{ { 'C', 0, 0 }, 0, CMD_CONSUME, "consume",
	  N_("Toggle consume mode") },
	{ { 'x', 0, 0 }, 0, CMD_CROSSFADE, "crossfade",
	  N_("Toggle crossfade mode") },
	{ { C('U'), 0, 0 }, 0, CMD_DB_UPDATE, "db-update",
	  N_("Start a music database update") },
	{ { 'S', 0, 0 }, 0, CMD_SAVE_PLAYLIST, "save",
	  N_("Save playlist") },
	{ { 'a', 0, 0 }, 0, CMD_ADD, "add",
	  N_("Add url/file to playlist") },

	{ { '!', 0, 0 }, 0, CMD_GO_ROOT_DIRECTORY, "go-root-directory",
	  N_("Go to root directory") },
	{ { '"', 0, 0 }, 0, CMD_GO_PARENT_DIRECTORY, "go-parent-directory",
	  N_("Go to parent directory") },

	{ { 'G', 0, 0 }, 0, CMD_LOCATE, "locate",
	  N_("Locate song in browser") },

	/* lists */
	{ { C('K'), 0, 0 }, 0, CMD_LIST_MOVE_UP, "move-up",
	  N_("Move item up") },
	{ { C('J'), 0, 0 }, 0, CMD_LIST_MOVE_DOWN, "move-down",
	  N_("Move item down") },
	{ { C('L'), 0, 0 }, 0, CMD_SCREEN_UPDATE, "update",
	  N_("Refresh screen") },


	/* ncmpc options */
	{ { 'w', 0, 0 }, 0, CMD_TOGGLE_FIND_WRAP, "wrap-mode",
	  /* translators: toggle between wrapping and non-wrapping
	     search */
	  N_("Toggle find mode") },
	{ { 'U', 0, 0 }, 0, CMD_TOGGLE_AUTOCENTER, "autocenter-mode",
	  /* translators: the auto center mode always centers the song
	     currently being played */
	  N_("Toggle auto center mode") },


	/* change screen */
	{ { TAB, 0, 0 }, 0, CMD_SCREEN_NEXT, "screen-next",
	  N_("Next screen") },
	{ { STAB, 0, 0 }, 0, CMD_SCREEN_PREVIOUS, "screen-prev",
	  N_("Previous screen") },
	{ { '`', 0, 0 }, 0, CMD_SCREEN_SWAP, "screen-swap",
	  N_("Swap to most recent screen") },


	/* find */
	{ { '/', 0, 0 }, 0, CMD_LIST_FIND, "find",
	  N_("Forward find") },
	{ { 'n', 0, 0 }, 0, CMD_LIST_FIND_NEXT, "find-next",
	  N_("Forward find next") },
	{ { '?', 0, 0 }, 0, CMD_LIST_RFIND, "rfind",
	  N_("Backward find") },
	{ { 'p', 0, 0 }, 0, CMD_LIST_RFIND_NEXT, "rfind-next",
	  N_("Backward find previous") },
	{ { '.', 0, 0 }, 0, CMD_LIST_JUMP, "jump",
		/* translators: this queries the user for a string
		 * and jumps directly (while the user is typing)
		 * to the entry which begins with this string */
	  N_("Jump to") },


	/* extra screens */
#ifdef ENABLE_ARTIST_SCREEN
	{ {'4', F4, 0 }, 0, CMD_SCREEN_ARTIST, "screen-artist",
	  N_("Artist screen") },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ {'5', F5, 0 }, 0, CMD_SCREEN_SEARCH, "screen-search",
	  N_("Search screen") },
	{ {'m', 0, 0 }, 0, CMD_SEARCH_MODE, "search-mode",
	  N_("Change search mode") },
#endif
#ifdef ENABLE_SONG_SCREEN
	{ { 'i', 0, 0 }, 0, CMD_SCREEN_SONG, "view",
	  N_("View the selected and the currently playing song") },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ {'7', F7, 0 }, 0, CMD_SCREEN_LYRICS, "screen-lyrics",
	  N_("Lyrics screen") },
	{ {ESC, 0, 0 }, 0, CMD_INTERRUPT, "lyrics-interrupt",
	  /* translators: interrupt the current background action,
	     e.g. stop loading lyrics from the internet */
	  N_("Interrupt action") },
	{ {'u', 0, 0 }, 0, CMD_LYRICS_UPDATE, "lyrics-update",
	  N_("Update Lyrics") },
	/* this command may move out of #ifdef ENABLE_LYRICS_SCREEN
	   at some point */
	{ {'e', 0, 0 }, 0, CMD_EDIT, "edit",
	  N_("Edit the current item") },
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{ {'8', F8, 0 }, 0, CMD_SCREEN_OUTPUTS, "screen-outputs",
	  N_("Outputs screen") },
#endif


	{ { -1, -1, -1 }, 0, CMD_NONE, NULL, NULL }
};

#ifdef ENABLE_KEYDEF_SCREEN
command_definition_t *
get_command_definitions(void)
{
	return cmds;
}

size_t
get_cmds_max_name_width(command_definition_t *c)
{
	static size_t max = 0;

	if (max != 0)
		return max;

	size_t len;
	command_definition_t *p;

	for (p = c; p->name != NULL; p++) {
		/*
		 * width and length are considered the same here, as command
		 * names are not translated.
		 */
		len = (size_t) strlen(p->name);
		if (len > max)
			max = len;
	}

	return max;
}
#endif

const char *
key2str(int key)
{
	static char buf[32];
	int i;

	buf[0] = 0;
	switch(key) {
	case 0:
		return _("Undefined");
	case ' ':
		return _("Space");
	case RET:
		return _("Enter");
	case BS:
		return _("Backspace");
	case DEL:
		return _("Delete");
	case UP:
		return _("Up");
	case DWN:
		return _("Down");
	case LEFT:
		return _("Left");
	case RGHT:
		return _("Right");
	case HOME:
		return _("Home");
	case END:
		return _("End");
	case PGDN:
		return _("PageDown");
	case PGUP:
		return _("PageUp");
	case TAB:
		return _("Tab");
	case STAB:
		return _("Shift+Tab");
	case ESC:
		return _("Esc");
	case KEY_IC:
		return _("Insert");
	default:
		for (i = 0; i <= 63; i++)
			if (key == KEY_F(i)) {
				g_snprintf(buf, 32, _("F%d"), i );
				return buf;
			}
		if (!(key & ~037))
			g_snprintf(buf, 32, _("Ctrl-%c"), 'A'+(key & 037)-1 );
		else if ((key & ~037) == 224)
			g_snprintf(buf, 32, _("Alt-%c"), 'A'+(key & 037)-1 );
		else if (key > 32 && key < 256)
			g_snprintf(buf, 32, "%c", key);
		else
			g_snprintf(buf, 32, "0x%03X", key);
	}

	return buf;
}

void
command_dump_keys(void)
{
	for (int i = 0; cmds[i].description; i++)
		if (cmds[i].command != CMD_NONE)
			printf(" %20s : %s\n",
			       get_key_names(cmds[i].command, true),
			       cmds[i].name);
}

#ifndef NCMPC_MINI

static void
set_key_flags(command_definition_t *cp, command_t command, int flags)
{
	for (int i = 0; cp[i].name; i++) {
		if (cp[i].command == command) {
			cp[i].flags |= flags;
			break;
		}
	}
}

#endif

const char *
get_key_names(command_t command, bool all)
{
	for (int i = 0; cmds[i].description; i++) {
		if (cmds[i].command == command) {
			static char keystr[80];

			g_strlcpy(keystr, key2str(cmds[i].keys[0]), sizeof(keystr));
			if (!all)
				return keystr;

			for (int j = 1; j < MAX_COMMAND_KEYS &&
					cmds[i].keys[j] > 0; j++) {
				g_strlcat(keystr, " ", sizeof(keystr));
				g_strlcat(keystr, key2str(cmds[i].keys[j]), sizeof(keystr));
			}
			return keystr;
		}
	}
	return NULL;
}

const char *
get_key_description(command_t command)
{
	for (int i = 0; cmds[i].description; i++)
		if (cmds[i].command == command)
			return _(cmds[i].description);

	return NULL;
}

const char *
get_key_command_name(command_t command)
{
	for (int i = 0; cmds[i].name; i++)
		if (cmds[i].command == command)
			return cmds[i].name;

	return NULL;
}

command_t
get_key_command_from_name(char *name)
{
	for (int i = 0; cmds[i].name; i++)
		if (strcmp(name, cmds[i].name) == 0)
			return cmds[i].command;

	return CMD_NONE;
}

command_t
find_key_command(int key, command_definition_t *c)
{
	assert(key != 0);
	assert(c != NULL);

	for (int i = 0; c[i].name; i++) {
		for (int j = 0; j < MAX_COMMAND_KEYS; j++)
			if (c[i].keys[j] == key)
				return c[i].command;
	}

	return CMD_NONE;
}

command_t
get_key_command(int key)
{
	return find_key_command(key, cmds);
}

command_t
get_keyboard_command(void)
{
	int key;

	key = wgetch(stdscr);
	if (key == ERR || key == '\0')
		return CMD_NONE;

#ifdef HAVE_GETMOUSE
	if (key == KEY_MOUSE)
		return CMD_MOUSE_EVENT;
#endif

	return get_key_command(key);
}

int
assign_keys(command_t command, int keys[MAX_COMMAND_KEYS])
{
	for (int i = 0; cmds[i].name; i++) {
		if (cmds[i].command == command) {
			memcpy(cmds[i].keys, keys, sizeof(int)*MAX_COMMAND_KEYS);
#ifndef NCMPC_MINI
			cmds[i].flags |= COMMAND_KEY_MODIFIED;
#endif
			return 0;
		}
	}

	return -1;
}

#ifndef NCMPC_MINI

int
check_key_bindings(command_definition_t *cp, char *buf, size_t bufsize)
{
	int i;
	int retval = 0;

	if (cp == NULL)
		cp = cmds;

	for (i = 0; cp[i].name; i++)
		cp[i].flags &= ~COMMAND_KEY_CONFLICT;

	for (i = 0; cp[i].name; i++) {
		int j;
		command_t cmd;

		for(j=0; j<MAX_COMMAND_KEYS; j++) {
			if (cp[i].keys[j] &&
			    (cmd = find_key_command(cp[i].keys[j],cp)) != cp[i].command) {
				if (buf) {
					g_snprintf(buf, bufsize,
						   _("Key %s assigned to %s and %s"),
						   key2str(cp[i].keys[j]),
						   get_key_command_name(cp[i].command),
						   get_key_command_name(cmd));
				} else {
					fprintf(stderr,
						_("Key %s assigned to %s and %s"),
						key2str(cp[i].keys[j]),
						get_key_command_name(cp[i].command),
						get_key_command_name(cmd));
					fputc('\n', stderr);
				}
				cp[i].flags |= COMMAND_KEY_CONFLICT;
				set_key_flags(cp, cmd, COMMAND_KEY_CONFLICT);
				retval = -1;
			}
		}
	}

	return retval;
}

int
write_key_bindings(FILE *f, int flags)
{
	if (flags & KEYDEF_WRITE_HEADER)
		fprintf(f, "## Key bindings for ncmpc (generated by ncmpc)\n\n");

	for (int i = 0; cmds[i].name && !ferror(f); i++) {
		if (cmds[i].flags & COMMAND_KEY_MODIFIED ||
		    flags & KEYDEF_WRITE_ALL) {
			fprintf(f, "## %s\n", cmds[i].description);
			if (flags & KEYDEF_COMMENT_ALL)
				fprintf(f, "#");
			fprintf(f, "key %s = ", cmds[i].name);
			for (int j = 0; j < MAX_COMMAND_KEYS; j++) {
				if (j && cmds[i].keys[j])
					fprintf(f, ",  ");
				if (!j || cmds[i].keys[j]) {
					if (cmds[i].keys[j]<256 && (isalpha(cmds[i].keys[j]) ||
								    isdigit(cmds[i].keys[j])))
						fprintf(f, "\'%c\'", cmds[i].keys[j]);
					else
						fprintf(f, "%d", cmds[i].keys[j]);
				}
			}
			fprintf(f,"\n\n");
		}
	}

	return ferror(f);
}

#endif /* NCMPC_MINI */
