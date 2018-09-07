/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "command.hxx"
#include "i18n.h"
#include "ncmpc_curses.h"
#include "util/Macros.hxx"

#include <assert.h>
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
#define F9   KEY_F(9)
#define C(x) KEY_CTL(x)

static command_definition_t cmds[] = {
#ifdef ENABLE_KEYDEF_SCREEN
	{ {'K', 0, 0 }, 0, "screen-keyedit",
	  N_("Key configuration screen") },
#endif
	{ { 'q', 'Q', C('C') }, 0, "quit",
	  N_("Quit") },

	/* movement */
	{ { UP, 'k', 0 }, 0, "up",
	  N_("Move cursor up") },
	{ { DWN, 'j', 0 }, 0, "down",
	  N_("Move cursor down") },
	{ { 'H', 0, 0 }, 0, "top",
	  N_("Move cursor to the top of screen") },
	{ { 'M', 0, 0 }, 0, "middle",
	  N_("Move cursor to the middle of screen") },
	{ { 'L', 0, 0 }, 0, "bottom",
	  N_("Move cursor to the bottom of screen") },
	{ { HOME, C('A'), 0 }, 0, "home",
	  N_("Move cursor to the top of the list") },
	{ { END, C('E'), 0 }, 0, "end",
	  N_("Move cursor to the bottom of the list") },
	{ { PGUP, 0, 0 }, 0, "pgup",
	  N_("Page up") },
	{ { PGDN, 0, 0 }, 0, "pgdn",
	  N_("Page down") },
	{ { 'v',  0, 0 }, 0, "range-select",
	  N_("Range selection") },
	{ { C('N'),  0, 0 }, 0, "scroll-down-line",
	  N_("Scroll down one line") },
	{ { C('B'),  0, 0 }, 0, "scroll-up-line",
	  N_("Scroll up one line") },
	{ { 'N',  0, 0 }, 0, "scroll-down-half",
	  N_("Scroll up half a screen") },
	{ { 'B',  0, 0 }, 0, "scroll-up-half",
	  N_("Scroll down half a screen") },
	{ { 'l', 0, 0 }, 0, "select-playing",
	  N_("Select currently playing song") },


	/* basic screens */
	{ { '1', F1, 'h' }, 0, "screen-help",
	  N_("Help screen") },
	{ { '2', F2, 0 }, 0, "screen-playlist",
	  N_("Queue screen") },
	{ { '3', F3, 0 }, 0, "screen-browse",
	  N_("Browse screen") },


	/* player commands */
	{ { RET, 0, 0 }, 0, "play",
	  N_("Play/Enter directory") },
	{ { 'P', 0, 0 }, 0, "pause",
	  N_("Pause") },
	{ { 's', BS, 0 }, 0, "stop",
	  N_("Stop") },
	{ { 'o', 0, 0 }, 0, "crop",
	  N_("Crop") },
	{ { '>', 0, 0 }, 0, "next",
	  N_("Next track") },
	{ { '<', 0, 0 }, 0, "prev",
	  N_("Previous track") },
	{ { 'f', 0, 0 }, 0, "seek-forward",
	  N_("Seek forward") },
	{ { 'b', 0, 0 }, 0, "seek-backward",
	  N_("Seek backward") },
	{ { '+', RGHT, 0 }, 0, "volume-up",
	  N_("Increase volume") },
	{ { '-', LEFT, 0 }, 0, "volume-down",
	  N_("Decrease volume") },
	{ { ' ', 0, 0 }, 0, "select",
	  N_("Select/deselect song in queue") },
	{ { 't', 0, 0 }, 0, "select_all",
	  N_("Select all listed items") },
	{ { DEL, 'd', 0 }, 0, "delete",
	  N_("Delete song from queue") },
	{ { 'Z', 0, 0 }, 0, "shuffle",
	  N_("Shuffle queue") },
	{ { 'c', 0, 0 }, 0, "clear",
	  N_("Clear queue") },
	{ { 'r', 0, 0 }, 0, "repeat",
	  N_("Toggle repeat mode") },
	{ { 'z', 0, 0 }, 0, "random",
	  N_("Toggle random mode") },
	{ { 'y', 0, 0 }, 0, "single",
	  N_("Toggle single mode") },
	{ { 'C', 0, 0 }, 0, "consume",
	  N_("Toggle consume mode") },
	{ { 'x', 0, 0 }, 0, "crossfade",
	  N_("Toggle crossfade mode") },
	{ { C('U'), 0, 0 }, 0, "db-update",
	  N_("Start a music database update") },
	{ { 'S', 0, 0 }, 0, "save",
	  N_("Save queue") },
	{ { 'a', 0, 0 }, 0, "add",
	  N_("Add url/file to queue") },

	{ { '!', 0, 0 }, 0, "go-root-directory",
	  N_("Go to root directory") },
	{ { '"', 0, 0 }, 0, "go-parent-directory",
	  N_("Go to parent directory") },

	{ { 'G', 0, 0 }, 0, "locate",
	  N_("Locate song in browser") },

	/* lists */
	{ { C('K'), 0, 0 }, 0, "move-up",
	  N_("Move item up") },
	{ { C('J'), 0, 0 }, 0, "move-down",
	  N_("Move item down") },
	{ { C('L'), 0, 0 }, 0, "update",
	  N_("Refresh screen") },


	/* ncmpc options */
	{ { 'w', 0, 0 }, 0, "wrap-mode",
	  /* translators: toggle between wrapping and non-wrapping
	     search */
	  N_("Toggle find mode") },
	{ { 'U', 0, 0 }, 0, "autocenter-mode",
	  /* translators: the auto center mode always centers the song
	     currently being played */
	  N_("Toggle auto center mode") },


	/* change screen */
	{ { TAB, 0, 0 }, 0, "screen-next",
	  N_("Next screen") },
	{ { STAB, 0, 0 }, 0, "screen-prev",
	  N_("Previous screen") },
	{ { '`', 0, 0 }, 0, "screen-swap",
	  N_("Swap to most recent screen") },


	/* find */
	{ { '/', 0, 0 }, 0, "find",
	  N_("Forward find") },
	{ { 'n', 0, 0 }, 0, "find-next",
	  N_("Forward find next") },
	{ { '?', 0, 0 }, 0, "rfind",
	  N_("Backward find") },
	{ { 'p', 0, 0 }, 0, "rfind-next",
	  N_("Backward find previous") },
	{ { '.', 0, 0 }, 0, "jump",
		/* translators: this queries the user for a string
		 * and jumps directly (while the user is typing)
		 * to the entry which begins with this string */
	  N_("Jump to") },


	/* extra screens */
#ifdef ENABLE_ARTIST_SCREEN
	{ {'4', F4, 0 }, 0, "screen-artist",
	  N_("Artist screen") },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ {'5', F5, 0 }, 0, "screen-search",
	  N_("Search screen") },
	{ {'m', 0, 0 }, 0, "search-mode",
	  N_("Change search mode") },
#endif
#ifdef ENABLE_SONG_SCREEN
	{ { 'i', 0, 0 }, 0, "view",
	  N_("View the selected and the currently playing song") },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ {'7', F7, 0 }, 0, "screen-lyrics",
	  N_("Lyrics screen") },
	{ {ESC, 0, 0 }, 0, "lyrics-interrupt",
	  /* translators: interrupt the current background action,
	     e.g. stop loading lyrics from the internet */
	  N_("Interrupt action") },
	{ {'u', 0, 0 }, 0, "lyrics-update",
	  N_("Update Lyrics") },
	/* this command may move out of #ifdef ENABLE_LYRICS_SCREEN
	   at some point */
	{ {'e', 0, 0 }, 0, "edit",
	  N_("Edit the current item") },
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{ {'8', F8, 0 }, 0, "screen-outputs",
	  N_("Outputs screen") },
#endif

#ifdef ENABLE_CHAT_SCREEN
	{ {'9', F9, 0}, 0, "screen-chat",
	  N_("Chat screen") },
#endif

	{ { -1, -1, -1 }, 0, nullptr, nullptr }
};

static_assert(ARRAY_SIZE(cmds) == size_t(CMD_NONE) + 1,
	      "Wrong command table size");

#ifdef ENABLE_KEYDEF_SCREEN
command_definition_t *
get_command_definitions()
{
	return cmds;
}

size_t
get_cmds_max_name_width(command_definition_t *c)
{
	static size_t max = 0;

	if (max != 0)
		return max;

	for (command_definition_t *p = c; p->name != nullptr; p++) {
		/*
		 * width and length are considered the same here, as command
		 * names are not translated.
		 */
		size_t len = strlen(p->name);
		if (len > max)
			max = len;
	}

	return max;
}
#endif

const char *
key2str(int key)
{
	switch(key) {
		static char buf[32];

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
		for (int i = 0; i <= 63; i++)
			if (key == KEY_F(i)) {
				snprintf(buf, 32, _("F%d"), i );
				return buf;
			}
		if (!(key & ~037))
			snprintf(buf, 32, _("Ctrl-%c"), 'A'+(key & 037)-1 );
		else if ((key & ~037) == 224)
			snprintf(buf, 32, _("Alt-%c"), 'A'+(key & 037)-1 );
		else if (key > 32 && key < 256)
			snprintf(buf, 32, "%c", key);
		else
			snprintf(buf, 32, "0x%03X", key);
		return buf;
	}
}

void
command_dump_keys()
{
	for (size_t i = 0; i < size_t(CMD_NONE); ++i)
		printf(" %20s : %s\n",
		       get_key_names(command_t(i), true),
		       cmds[i].name);
}

#ifndef NCMPC_MINI

static void
set_key_flags(command_definition_t *cp, command_t command, int flags)
{
	cp[size_t(command)].flags |= flags;
}

#endif

const char *
get_key_names(command_t command, bool all)
{
	const auto &c = cmds[size_t(command)];

	static char keystr[80];

	g_strlcpy(keystr, key2str(c.keys[0]), sizeof(keystr));
	if (!all)
		return keystr;

	for (unsigned j = 1; j < MAX_COMMAND_KEYS &&
		     c.keys[j] > 0; j++) {
		g_strlcat(keystr, " ", sizeof(keystr));
		g_strlcat(keystr, key2str(c.keys[j]), sizeof(keystr));
	}
	return keystr;
}

const char *
get_key_description(command_t command)
{
	return _(cmds[size_t(command)].description);
}

const char *
get_key_command_name(command_t command)
{
	return cmds[size_t(command)].name;
}

command_t
get_key_command_from_name(const char *name)
{
	for (size_t i = 0; i < size_t(CMD_NONE); ++i)
		if (strcmp(name, cmds[i].name) == 0)
			return command_t(i);

	return CMD_NONE;
}

command_t
find_key_command(int key, const command_definition_t *c)
{
	assert(key != 0);
	assert(c != nullptr);

	for (size_t i = 0; c[i].name; i++) {
		for (int j = 0; j < MAX_COMMAND_KEYS; j++)
			if (c[i].keys[j] == key)
				return command_t(i);
	}

	return CMD_NONE;
}

command_t
get_key_command(int key)
{
	return find_key_command(key, cmds);
}

bool
assign_keys(command_t command, int keys[MAX_COMMAND_KEYS])
{
	auto &c = cmds[size_t(command)];
	memcpy(c.keys, keys, sizeof(int)*MAX_COMMAND_KEYS);
#ifndef NCMPC_MINI
	c.flags |= COMMAND_KEY_MODIFIED;
#endif
	return true;
}

#ifndef NCMPC_MINI

bool
check_key_bindings(command_definition_t *cp, char *buf, size_t bufsize)
{
	bool success = true;

	if (cp == nullptr)
		cp = cmds;

	for (size_t i = 0; cp[i].name; i++)
		cp[i].flags &= ~COMMAND_KEY_CONFLICT;

	for (size_t i = 0; cp[i].name; i++) {
		int j;
		command_t cmd;

		for(j=0; j<MAX_COMMAND_KEYS; j++) {
			if (cp[i].keys[j] &&
			    (cmd = find_key_command(cp[i].keys[j],cp)) != command_t(i)) {
				if (buf) {
					snprintf(buf, bufsize,
						 _("Key %s assigned to %s and %s"),
						 key2str(cp[i].keys[j]),
						 get_key_command_name(command_t(i)),
						 get_key_command_name(cmd));
				} else {
					fprintf(stderr,
						_("Key %s assigned to %s and %s"),
						key2str(cp[i].keys[j]),
						get_key_command_name(command_t(i)),
						get_key_command_name(cmd));
					fputc('\n', stderr);
				}
				cp[i].flags |= COMMAND_KEY_CONFLICT;
				set_key_flags(cp, cmd, COMMAND_KEY_CONFLICT);
				success = false;
			}
		}
	}

	return success;
}

bool
write_key_bindings(FILE *f, int flags)
{
	if (flags & KEYDEF_WRITE_HEADER)
		fprintf(f, "## Key bindings for ncmpc (generated by ncmpc)\n\n");

	for (size_t i = 0; cmds[i].name && !ferror(f); i++) {
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

	return ferror(f) == 0;
}

#endif /* NCMPC_MINI */
