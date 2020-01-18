/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "TagListPage.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "FileListPage.hxx"
#include "Command.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "util/StringUTF8.hxx"

#include <algorithm>

#include <assert.h>
#include <string.h>

TagFilter
TagListPage::MakeCursorFilter() const noexcept
{
	unsigned i = lw.GetCursorIndex();
	if (parent != nullptr) {
		if (i == 0)
			return {};

		--i;
	}

	auto new_filter = filter;
	if (i < values.size())
		new_filter.emplace_front(tag, values[i]);
	return new_filter;
}

gcc_pure
static bool
CompareUTF8(const std::string &a, const std::string &b)
{
	return CollateUTF8(a.c_str(), b.c_str()) < 0;
}

const char *
TagListPage::GetListItemText(char *buffer, size_t size,
			     unsigned idx) const noexcept
{
	if (parent != nullptr) {
		if (idx == 0)
			return "..";

		--idx;
	}

	if (idx == values.size() + 1)
		return _("All tracks");

	assert(idx < values.size());

	return utf8_to_locale(values[idx].c_str(), buffer, size);
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
TagListPage::LoadValues(struct mpdclient &c) noexcept
{
	auto *connection = c.GetConnection();

	values.clear();

	if (connection != nullptr) {
		mpd_search_db_tags(connection, tag);
		AddConstraints(connection, filter);
		mpd_search_commit(connection);

		recv_tag_values(connection, tag, values);

		c.FinishCommand();
	}

	/* sort list */
	std::sort(values.begin(), values.end(), CompareUTF8);
	lw.SetLength((parent != nullptr) + values.size() + (all_text != nullptr));
}

void
TagListPage::Reload(struct mpdclient &c)
{
	LoadValues(c);
}

/**
 * Paint one item in the album list.  There are two virtual items
 * inserted: at the beginning, there's the special item ".." to go to
 * the parent directory, and at the end, there's the item "All tracks"
 * to view the tracks of all albums.
 */
void
TagListPage::PaintListItem(WINDOW *w, unsigned i,
			     gcc_unused unsigned y, unsigned width,
			     bool selected) const noexcept
{
	if (parent != nullptr) {
		if (i == 0) {
			screen_browser_paint_directory(w, width, selected,
						       "..");
			return;
		}

		--i;
	}

	if (i < values.size())
		screen_browser_paint_directory(w, width, selected,
					       Utf8ToLocale(values[i].c_str()).c_str());
	else
		screen_browser_paint_directory(w, width, selected,
					       all_text);
}

void
TagListPage::Paint() const noexcept
{
	lw.Paint(*this);
}

const char *
TagListPage::GetTitle(char *, size_t) const noexcept
{
	return title.c_str();
}

void
TagListPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (events & MPD_IDLE_DATABASE) {
		/* the db has changed -> update the list */
		Reload(c);
		SetDirty();
	}
}

/* add_query - Add all songs satisfying specified criteria.
   _artist is actually only used in the ALBUM case to distinguish albums with
   the same name from different artists. */
static void
add_query(struct mpdclient *c, const TagFilter &filter,
	  enum mpd_tag_type tag, const char *value) noexcept
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	const char *text = value;
	if (value == nullptr)
		value = filter.empty() ? "?" : filter.front().second.c_str();

	screen_status_printf(_("Adding \'%s\' to queue"),
			     Utf8ToLocale(text).c_str());

	mpd_search_add_db_songs(connection, true);
	AddConstraints(connection, filter);

	if (value != nullptr)
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      tag, value);

	mpd_search_commit(connection);
	c->FinishCommand();
}

bool
TagListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch(cmd) {
	case Command::PLAY:
		if (lw.GetCursorIndex() == 0 && parent != nullptr)
			/* handle ".." */
			return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);

		break;

	case Command::SELECT:
	case Command::ADD:
		for (unsigned i : lw.GetRange()) {
			if (parent != nullptr) {
				if (i == 0)
					continue;

				--i;
			}

			add_query(&c, filter, tag,
				  i < values.size()
				  ? values[i].c_str() : nullptr);
			cmd = Command::LIST_NEXT; /* continue and select next item... */
		}
		break;

		/* continue and update... */
	case Command::SCREEN_UPDATE:
		Reload(c);
		return false;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, lw, cmd, *this);
		SetDirty();
		return true;

	case Command::LIST_JUMP:
		screen_jump(screen, lw, *this, *this);
		SetDirty();
		return true;

	default:
		break;
	}

	if (lw.HandleCommand(cmd)) {
		SetDirty();
		return true;
	}

	return false;
}
