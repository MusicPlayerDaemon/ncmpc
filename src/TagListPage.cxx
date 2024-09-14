// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TagListPage.hxx"
#include "FileListPage.hxx"
#include "Command.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "Interface.hxx"
#include "page/FindSupport.hxx"
#include "client/mpdclient.hxx"
#include "util/StringUTF8.hxx"

#include <algorithm>

#include <assert.h>
#include <string.h>

using std::string_view_literals::operator""sv;

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

[[gnu::pure]]
static bool
CompareUTF8(const std::string &a, const std::string &b)
{
	return CollateUTF8(a.c_str(), b.c_str()) < 0;
}

std::string_view
TagListPage::GetListItemText(std::span<char> buffer,
			     unsigned idx) const noexcept
{
	if (parent != nullptr) {
		if (idx == 0)
			return ".."sv;

		--idx;
	}

	if (idx == values.size() + 1)
		return _("All tracks");

	assert(idx < values.size());

	return utf8_to_locale(values[idx].c_str(), buffer);
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

	SchedulePaint();
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
TagListPage::PaintListItem(const Window window, unsigned i,
			   [[maybe_unused]] unsigned y, unsigned width,
			   bool selected) const noexcept
{
	if (parent != nullptr) {
		if (i == 0) {
			screen_browser_paint_directory(window, width, selected,
						       "..");
			return;
		}

		--i;
	}

	if (i < values.size())
		screen_browser_paint_directory(window, width, selected,
					       Utf8ToLocale{values[i]});
	else
		screen_browser_paint_directory(window, width, selected,
					       all_text);
}

void
TagListPage::Paint() const noexcept
{
	lw.Paint(*this);
}

std::string_view
TagListPage::GetTitle(std::span<char>) const noexcept
{
	return title;
}

void
TagListPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (events & MPD_IDLE_DATABASE) {
		/* the db has changed -> update the list */
		Reload(c);
	}
}

bool
TagListPage::HandleEnter(struct mpdclient &c)
{
	if (lw.GetCursorIndex() == 0 && parent != nullptr)
		/* handle ".." */
		return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);

	return false;
}

/* add_query - Add all songs satisfying specified criteria.
   _artist is actually only used in the ALBUM case to distinguish albums with
   the same name from different artists. */
static void
add_query(Interface &interface, struct mpdclient *c, const TagFilter &filter,
	  enum mpd_tag_type tag, const char *value) noexcept
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	const char *text = value;
	if (text == nullptr)
		/* adding the special "[All]" entry: show the name of
		   the previous level in the filter */
		text = filter.empty() ? "?" : filter.front().second.c_str();

	interface.Alert(fmt::format(fmt::runtime(_("Adding '{}' to queue")),
				    (std::string_view)Utf8ToLocale{text}));

	mpd_search_add_db_songs(connection, true);
	AddConstraints(connection, filter);

	if (value != nullptr)
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      tag, value);

	mpd_search_commit(connection);
	c->FinishCommand();
}

bool
TagListPage::HandleSelect(struct mpdclient &c)
{
	bool result = false;

	for (unsigned i : lw.GetRange()) {
		if (parent != nullptr) {
			if (i == 0)
				continue;

			--i;
		}

		add_query(GetInterface(), &c, filter, tag,
			  i < values.size()
			  ? values[i].c_str() : nullptr);
		result = true;
	}

	return result;
}


bool
TagListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch(cmd) {
	case Command::PLAY:
		return HandleEnter(c);

	case Command::SELECT:
	case Command::ADD:
		if (HandleSelect(c))
			cmd = Command::LIST_NEXT; /* continue and select next item... */

		break;

		/* continue and update... */
	case Command::SCREEN_UPDATE:
		Reload(c);
		return false;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		CoStart(find_support.Find(lw, *this, cmd));
		return true;

	case Command::LIST_JUMP:
		CoStart(find_support.Jump(lw, *this, *this));
		return true;

	default:
		break;
	}

	if (lw.HandleCommand(cmd)) {
		SchedulePaint();
		return true;
	}

	return false;
}

#ifdef HAVE_GETMOUSE

bool
TagListPage::OnMouse(struct mpdclient &c, Point p,
		      mmask_t bstate)
{
	unsigned prev_selected = lw.GetCursorIndex();

	if (ListPage::OnMouse(c, p, bstate))
		return true;

	lw.SetCursorFromOrigin(p.y);

	if (bstate & (BUTTON1_CLICKED|BUTTON1_DOUBLE_CLICKED)) {
		if ((bstate & BUTTON1_DOUBLE_CLICKED) ||
		    prev_selected == lw.GetCursorIndex())
			HandleEnter(c);
	} else if (bstate & (BUTTON3_CLICKED|BUTTON3_DOUBLE_CLICKED)) {
		if ((bstate & BUTTON3_DOUBLE_CLICKED) ||
		    prev_selected == lw.GetCursorIndex())
			HandleSelect(c);
	}

	SchedulePaint();

	return true;
}

#endif
