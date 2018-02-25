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

#include "ArtistListPage.hxx"
#include "screen_interface.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen_browser.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"

#include <algorithm>

#include <glib.h>

#include <assert.h>
#include <string.h>

#define BUFSIZE 1024

gcc_pure
static bool
CompareUTF8(const std::string &a, const std::string &b)
{
	char *key1 = g_utf8_collate_key(a.c_str(), -1);
	char *key2 = g_utf8_collate_key(b.c_str(), -1);
	int n = strcmp(key1,key2);
	g_free(key1);
	g_free(key2);
	return n < 0;
}

/* list_window callback */
static const char *
screen_artist_lw_callback(unsigned idx, void *data)
{
	const auto &list = *(const std::vector<std::string> *)data;

	assert(idx < list.size());

	const char *str_utf8 = list[idx].c_str();

	static char buf[BUFSIZE];
	g_strlcpy(buf, Utf8ToLocale(str_utf8).c_str(), sizeof(buf));
	return buf;
}

static void
recv_tag_values(struct mpd_connection *connection, enum mpd_tag_type tag,
		std::vector<std::string> &list)
{
	struct mpd_pair *pair;

	while ((pair = mpd_recv_pair_tag(connection, tag)) != nullptr) {
		list.emplace_back(pair->value);
		mpd_return_pair(connection, pair);
	}
}

void
ArtistListPage::LoadArtistList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	artist_list.clear();

	if (connection != nullptr) {
		mpd_search_db_tags(connection, MPD_TAG_ARTIST);
		mpd_search_commit(connection);
		recv_tag_values(connection, MPD_TAG_ARTIST, artist_list);

		mpdclient_finish_command(&c);
	}

	/* sort list */
	std::sort(artist_list.begin(), artist_list.end(), CompareUTF8);
	lw.SetLength(artist_list.size());
}

void
ArtistListPage::Reload(struct mpdclient &c)
{
	LoadArtistList(c);
}

void
ArtistListPage::PaintListItem(WINDOW *w, unsigned i,
			      gcc_unused unsigned y, unsigned width,
			      bool selected) const
{
	screen_browser_paint_directory(w, width, selected,
				       Utf8ToLocale(artist_list[i].c_str()).c_str());
}

void
ArtistListPage::Paint() const
{
	lw.Paint(*this);
}

const char *
ArtistListPage::GetTitle(char *, size_t) const
{
	return _("All artists");
}

void
ArtistListPage::Update(struct mpdclient &c, unsigned events)
{
	if (events & MPD_IDLE_DATABASE) {
		/* the db has changed -> update the list */
		Reload(c);
		SetDirty();
	}
}

/* add_query - Add all songs satisfying specified criteria */
static void
add_query(struct mpdclient *c, enum mpd_tag_type table, const char *_filter)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);

	assert(_filter != nullptr);

	if (connection == nullptr)
		return;

	screen_status_printf(_("Adding \'%s\' to queue"),
			     Utf8ToLocale(_filter).c_str());

	mpd_search_add_db_songs(connection, true);
	mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
				      table, _filter);
	mpd_search_commit(connection);
	mpdclient_finish_command(c);
}

inline bool
ArtistListPage::OnListCommand(command_t cmd)
{
	if (lw.HandleCommand(cmd)) {
		SetDirty();
		return true;
	}

	return false;
}

bool
ArtistListPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	switch(cmd) {
		const char *selected;

	case CMD_SELECT:
	case CMD_ADD:
		if (lw.selected >= artist_list.size())
			return true;

		for (const unsigned i : lw.GetRange()) {
			selected = artist_list[i].c_str();
			add_query(&c, MPD_TAG_ARTIST, selected);
			cmd = CMD_LIST_NEXT; /* continue and select next item... */
		}

		break;

		/* continue and update... */
	case CMD_SCREEN_UPDATE:
		Reload(c);
		return false;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd,
			    screen_artist_lw_callback, &artist_list);
		SetDirty();
		return true;

	case CMD_LIST_JUMP:
		screen_jump(screen, &lw,
			    screen_artist_lw_callback, &artist_list,
			    this);
		SetDirty();
		return true;

	default:
		break;
	}

	if (OnListCommand(cmd))
		return true;

	return false;
}
