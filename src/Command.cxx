// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Command.hxx"
#include "i18n.h"
#include "util/StringAPI.hxx"

#include <iterator>

#include <string.h>

static constexpr command_definition_t cmds[] = {
#ifdef ENABLE_KEYDEF_SCREEN
	{ "screen-keyedit",
	  N_("Key configuration screen") },
#endif
	{ "quit",
	  N_("Quit") },

	/* movement */
	{ "up",
	  N_("Move cursor up") },
	{ "down",
	  N_("Move cursor down") },
	{ "top",
	  N_("Move cursor to the top of screen") },
	{ "middle",
	  N_("Move cursor to the middle of screen") },
	{ "bottom",
	  N_("Move cursor to the bottom of screen") },
	{ "home",
	  N_("Move cursor to the top of the list") },
	{ "end",
	  N_("Move cursor to the bottom of the list") },
	{ "pgup",
	  N_("Page up") },
	{ "pgdn",
	  N_("Page down") },
	{ "range-select",
	  N_("Range selection") },
	{ "scroll-down-line",
	  N_("Scroll down one line") },
	{ "scroll-up-line",
	  N_("Scroll up one line") },
	{ "scroll-down-half",
	  N_("Scroll up half a screen") },
	{ "scroll-up-half",
	  N_("Scroll down half a screen") },
	{ "select-playing",
	  N_("Select currently playing song") },


	/* basic screens */
	{ "screen-help",
	  N_("Help screen") },
	{ "screen-playlist",
	  N_("Queue screen") },
	{ "screen-browse",
	  N_("Browse screen") },


	/* player commands */
	{ "play",
	  N_("Play/Enter directory") },
	{ "pause",
	  N_("Pause") },
	{ "stop",
	  N_("Stop") },
	{ "crop",
	  N_("Crop") },
	{ "next",
	  N_("Next track") },
	{ "prev",
	  N_("Previous track") },
	{ "seek-forward",
	  N_("Seek forward") },
	{ "seek-backward",
	  N_("Seek backward") },
	{ "volume-up",
	  N_("Increase volume") },
	{ "volume-down",
	  N_("Decrease volume") },
	{ "select",
	  N_("Select/deselect song in queue") },
	{ "select_all",
	  N_("Select all listed items") },
	{ "delete",
	  N_("Delete song from queue") },
	{ "shuffle",
	  N_("Shuffle queue") },
	{ "clear",
	  N_("Clear queue") },
	{ "repeat",
	  N_("Toggle repeat mode") },
	{ "random",
	  N_("Toggle random mode") },
	{ "single",
	  N_("Toggle single mode") },
	{ "consume",
	  N_("Toggle consume mode") },
	{ "crossfade",
	  N_("Toggle crossfade mode") },
	{ "db-update",
	  N_("Start a music database update") },
	{ "save",
	  N_("Save queue") },
	{ "add",
	  N_("Append song to queue") },

	{ "go-root-directory",
	  N_("Go to root directory") },
	{ "go-parent-directory",
	  N_("Go to parent directory") },

	{ "locate",
	  N_("Locate song in browser") },

	/* lists */
	{ "move-up",
	  N_("Move item up") },
	{ "move-down",
	  N_("Move item down") },
	{ "update",
	  N_("Refresh screen") },


	/* ncmpc options */
	{ "wrap-mode",
	  /* translators: toggle between wrapping and non-wrapping
	     search */
	  N_("Toggle find mode") },
	{ "autocenter-mode",
	  /* translators: the auto center mode always centers the song
	     currently being played */
	  N_("Toggle auto center mode") },


	/* change screen */
	{ "screen-next",
	  N_("Next screen") },
	{ "screen-prev",
	  N_("Previous screen") },
	{ "screen-swap",
	  N_("Swap to most recent screen") },


	/* find */
	{ "find",
	  N_("Forward find") },
	{ "find-next",
	  N_("Forward find next") },
	{ "rfind",
	  N_("Backward find") },
	{ "rfind-next",
	  N_("Backward find previous") },
	{ "jump",
		/* translators: this queries the user for a string
		 * and jumps directly (while the user is typing)
		 * to the entry which begins with this string */
	  N_("Jump to") },


	/* extra screens */
#ifdef ENABLE_LIBRARY_PAGE
	{ "library-page",
	  N_("Library page") },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ "screen-search",
	  N_("Search screen") },
	{ "search-mode",
	  N_("Change search mode") },
#endif
#ifdef ENABLE_PLAYLIST_EDITOR
	{ "playlist-editor-page",
	  N_("Playlist editor") },
#endif
#ifdef ENABLE_SONG_SCREEN
	{ "view",
	  N_("View the selected and the currently playing song") },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ "screen-lyrics",
	  N_("Lyrics screen") },
	{ "lyrics-interrupt",
	  /* translators: interrupt the current background action,
	     e.g. stop loading lyrics from the internet */
	  N_("Interrupt action") },
	{ "lyrics-update",
	  N_("Update Lyrics") },
#endif
#if defined(ENABLE_LYRICS_SCREEN) || defined(ENABLE_PLAYLIST_EDITOR)
	{ "edit",
	  N_("Edit the current item") },
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{ "screen-outputs",
	  N_("Outputs screen") },
#endif

#ifdef ENABLE_CHAT_SCREEN
	{ "screen-chat",
	  N_("Chat screen") },
#endif
};

static_assert(std::size(cmds) == size_t(Command::NONE),
	      "Wrong command table size");

const command_definition_t *
get_command_definitions() noexcept
{
	return cmds;
}

std::size_t
get_cmds_max_name_width() noexcept
{
	static size_t max = 0;

	if (max != 0)
		return max;

	for (const auto &i : cmds) {
		/*
		 * width and length are considered the same here, as command
		 * names are not translated.
		 */
		size_t len = strlen(i.name);
		if (len > max)
			max = len;
	}

	return max;
}

const char *
get_key_description(Command command) noexcept
{
	return my_gettext(cmds[size_t(command)].description);
}

const char *
get_key_command_name(Command command) noexcept
{
	return cmds[size_t(command)].name;
}

Command
get_key_command_from_name(const char *name) noexcept
{
	for (size_t i = 0; i < size_t(Command::NONE); ++i)
		if (StringIsEqual(name, cmds[i].name))
			return Command(i);

#ifdef ENABLE_LIBRARY_PAGE
	/* compatibility with 0.32 and older */
	if (StringIsEqual(name, "screen-artist"))
		return Command::LIBRARY_PAGE;
#endif

	return Command::NONE;
}
