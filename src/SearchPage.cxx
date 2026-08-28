// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SearchPage.hxx"
#include "PageMeta.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "Options.hxx"
#include "charset.hxx"
#include "Command.hxx"
#include "FileListPage.hxx"
#include "Styles.hxx"
#include "page/ProxyPage.hxx"
#include "ui/form/StringEditRow.hxx"
#include "ui/form/TableForm.hxx"
#include "ui/dialogs/TextInputDialog.hxx"
#include "ui/paint.hxx"
#include "client/mpdclient.hxx"
#include "time/Parser.hxx"
#include "co/Task.hxx"
#include "util/LocaleString.hxx"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"

#include <fmt/format.h>

#include <array>
#include <iterator>

using std::string_view_literals::operator""sv;

enum {
	SEARCH_URI = MPD_TAG_COUNT,
	SEARCH_COUNT,

	SEARCH_MODIFIED,
	SEARCH_ADVANCED,
};

static constexpr struct {
	enum mpd_tag_type tag_type;
	const char *name;
	const char *localname;
} search_tag[MPD_TAG_COUNT] = {
	{ MPD_TAG_ARTIST, "artist", N_("artist") },
	{ MPD_TAG_ALBUM, "album", N_("album") },
	{ MPD_TAG_TITLE, "title", N_("title") },
	{ MPD_TAG_TRACK, "track", N_("track") },
	{ MPD_TAG_NAME, "name", N_("name") },
	{ MPD_TAG_GENRE, "genre", N_("genre") },
	{ MPD_TAG_DATE, "date", N_("date") },
	{ MPD_TAG_COMPOSER, "composer", N_("composer") },
	{ MPD_TAG_PERFORMER, "performer", N_("performer") },
	{ MPD_TAG_LABEL, "label", N_("label") },
	{ MPD_TAG_COMMENT, "comment", N_("comment") },
	{ MPD_TAG_COUNT, nullptr, nullptr }
};

static constexpr int
search_get_tag_id(std::string_view name) noexcept
{
	if (StringIsEqualIgnoreCase(name, "file"sv) ||
	    StringIsEqualIgnoreCase(name, _("file")))
		return SEARCH_URI;

	if (StringIsEqualIgnoreCase(name, "modified"sv))
		return SEARCH_MODIFIED;

	for (unsigned i = 0; search_tag[i].name != nullptr; ++i)
		if (StringIsEqualIgnoreCase(search_tag[i].name, name) ||
		    StringIsEqualIgnoreCase(my_gettext(search_tag[i].localname), name))
			return search_tag[i].tag_type;

	return -1;
}

struct SearchFilter {
	std::array<std::string, SEARCH_COUNT> tag_constraints;

	time_t modified_since = 0;

	bool IsEmpty() const noexcept {
		for (const auto &i : tag_constraints)
			if (!i.empty())
				return false;

		return modified_since == 0;
	}

	void SetAdvanced(std::string_view s);

	void SendDbSearch(struct mpd_connection &c) const noexcept;
	bool DoSearch(struct mpdclient &c, FileList &result) const noexcept;
};

/**
 * Throws on error.
 */
static time_t
ParseModifiedSince(std::string_view s)
{
	return time(nullptr) - ParseDuration(s);
}

inline void
SearchFilter::SetAdvanced(std::string_view s)
{
	while (!s.empty()) {
		auto [a, value] = SplitLast(s, ':');
		if (value.data() == nullptr)
			throw std::invalid_argument{"Missing colon"};

		auto [rest, name] = SplitLast(a, ' ');
		if (name.empty()) {
			name = rest;
			rest = {};

			if (name.empty())
				throw std::invalid_argument{"Missing name"};
		}

		const int tag = search_get_tag_id(name);
		if (tag < 0)
			throw std::invalid_argument{
				fmt::format(fmt::runtime(_("Bad search tag {}")), name),
			};

		value = Strip(value);
		if (value.empty())
			throw std::invalid_argument{
				fmt::format(fmt::runtime(_("No argument for search tag {}")), name),
			};

		if (tag == SEARCH_MODIFIED) {
			modified_since = ParseModifiedSince(value);
		} else {
			const std::size_t idx = static_cast<std::size_t>(tag);
			assert(idx < tag_constraints.size());
			auto &contraint = tag_constraints[idx];
			if (!contraint.empty())
				throw std::invalid_argument{"Duplicate name"};

			contraint = value;
		}

		s = rest;
	}
}

inline void
SearchFilter::SendDbSearch(struct mpd_connection &c) const noexcept
{
	constexpr bool exact_match = false; // TODO

	mpd_search_db_songs(&c, exact_match);

	for (unsigned i = 0; i < tag_constraints.size(); ++i) {
		if (tag_constraints[i].empty())
			continue;

		const LocaleToUtf8Z utf8{tag_constraints[i]};

		if (i == SEARCH_URI)
			mpd_search_add_uri_constraint(&c, MPD_OPERATOR_DEFAULT,
						      utf8.c_str());
		else
			mpd_search_add_tag_constraint(&c, MPD_OPERATOR_DEFAULT,
						      static_cast<enum mpd_tag_type>(i),
						      utf8.c_str());
	}

	if (modified_since > 0)
		mpd_search_add_modified_since_constraint(&c,
							 MPD_OPERATOR_DEFAULT,
							 modified_since);

	mpd_search_commit(&c);
}

inline bool
SearchFilter::DoSearch(struct mpdclient &c, FileList &result) const noexcept
{
	if (IsEmpty())
		return false;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return false;

	SendDbSearch(*connection);

	result.Receive(*connection);
	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		c.HandleError();
		return false;
	}

	return true;
}

template<std::size_t N>
static constexpr std::array<StringEditRow, N>
MakeStringEditRowArray(const std::span<const char *const, N> labels) noexcept
{
	return [&]<std::size_t... i>(std::index_sequence<i...>) {
		return std::array<StringEditRow, N>{
			StringEditRow{my_gettext(labels[i]), ""sv}...
		};
	}(std::make_index_sequence<N>{});
}

struct SearchFilterForm {
	static constexpr unsigned tags[] = {
		MPD_TAG_TITLE,
		MPD_TAG_ARTIST,
		MPD_TAG_ALBUM,
		SEARCH_URI,
		SEARCH_ADVANCED,
	};

	static constexpr const char *labels[] = {
		N_("Title"),
		N_("Artist"),
		N_("Album"),
		N_("Filename"),
		N_("Advanced"),
	};

	static constexpr unsigned N_ROWS = std::size(labels);

	std::array<StringEditRow, N_ROWS> rows = MakeStringEditRowArray(std::span{labels});

	void Clear() noexcept {
		for (auto &i : rows)
			i.Clear();
	}

	operator SearchFilter() const {
		SearchFilter filter;

		for (unsigned i = 0; i < N_ROWS; ++i) {
			const auto &value = rows[i].GetValue();
			const unsigned tag = tags[i];
			if (tag == SEARCH_ADVANCED)
				filter.SetAdvanced(value);
			else
				filter.tag_constraints[tag] = value;
		};

		return filter;
	}
};

class SearchFilterPage final : public ListPage, ListRenderer {
	ModalDock &modal_dock;

	SearchFilterForm filter;

	static constexpr unsigned SEARCH_INDEX = SearchFilterForm::N_ROWS;

public:
	SearchFilterPage(PageContainer &_container, const Window _window,
			 ModalDock &_modal_dock) noexcept
		:ListPage(_container, _window),
		 modal_dock(_modal_dock)
	{
		std::apply([](auto&... rows) {
			AdjustLabelWidths(rows...);
		}, filter.rows);

		lw.SetLength(SEARCH_INDEX + 1);
	}

	bool IsSearchButtonSelected() const noexcept {
		return lw.GetCursorIndex() == SEARCH_INDEX;
	}

	SearchFilter GetFilter() const {
		return filter;
	}

private:
	Co::InvokeTask EditPattern(unsigned i) noexcept {
		if (co_await filter.rows[i].Edit(modal_dock))
			SchedulePaint();
	}

	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(const Window window, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;
};

void
SearchFilterPage::Paint() const noexcept
{
	lw.Paint(*this);
}

bool
SearchFilterPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (cmd == Command::LIST_RANGE_SELECT)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::PLAY:
		if (unsigned i = lw.GetCursorIndex(); i < filter.rows.size()) {
			CoStart(EditPattern(i));
			return true;
		}

		return false;

	case Command::DELETE:
		if (unsigned i = lw.GetCursorIndex(); i < filter.rows.size()) {
			filter.rows[i].Clear();
			SchedulePaint();
			return true;
		}

		return false;

	case Command::CLEAR:
		filter.Clear();
		SchedulePaint();
		return true;

	default:
		return false;
	}
}

std::string_view
SearchFilterPage::GetTitle([[maybe_unused]] std::span<char> buffer) const noexcept
{
	return _("Search");
}

void
SearchFilterPage::PaintListItem(const Window window, unsigned i,
				unsigned y, unsigned width,
				bool selected) const noexcept
{
	if (i == SEARCH_INDEX) {
		row_paint_text(window, width, Style::DIRECTORY, selected, _("Search"));
		return;
	}

	filter.rows[i].Paint(window, y, width, selected);
}

class SearchResultPage final : public FileListPage {
	Page *const parent;

	SearchFilter filter;

public:
	SearchResultPage(PageContainer &_container, ScreenManager &_screen,
			 Page *_parent,
			 const Window _window) noexcept
		:FileListPage(_container, _screen, _window,
			      !options.search_format.empty()
			      ? options.search_format.c_str()
			      : options.list_format.c_str()),
		 parent(_parent) {}

	void SetFilter(struct mpdclient &c,SearchFilter &&_filter) noexcept {
		filter = std::move(_filter);
		Reload(c);
	}

private:
	void Reload(struct mpdclient &c);

	[[nodiscard]]
	Co::InvokeTask Start(struct mpdclient &c);

public:
	/* virtual methods from class FileListPage */
	bool HandleEnter(struct mpdclient &c) override;

	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};

void
SearchResultPage::Reload(struct mpdclient &c)
{
	filelist.clear();
	filelist.emplace_back(nullptr);

	filter.DoSearch(c, filelist);
	lw.SetLength(filelist.size());

	screen_browser_sync_highlights(filelist, c.playlist);

	SchedulePaint();
}

bool
SearchResultPage::HandleEnter(struct mpdclient &c)
{
	if (lw.GetCursorIndex() == 0 && parent != nullptr)
		/* handle ".." */
		return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);

	return FileListPage::HandleEnter(c);
}

void
SearchResultPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (events & MPD_IDLE_QUEUE) {
		screen_browser_sync_highlights(filelist, c.playlist);
		SchedulePaint();
	}
}

bool
SearchResultPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch (cmd) {
	case Command::SCREEN_UPDATE:
		Reload(c);
		return true;

	default:
		break;
	}

	if (FileListPage::OnCommand(c, cmd))
		return true;

	return false;
}

std::string_view
SearchResultPage::GetTitle([[maybe_unused]] std::span<char> buffer) const noexcept
{
	return _("Search");
}

class SearchPage final : public ProxyPage {
	SearchFilterPage filter_page;
	SearchResultPage result_page;

public:
	SearchPage(ScreenManager &screen, const Window _window)
		:ProxyPage(screen, _window),
		 filter_page(*this, _window, screen),
		 result_page(*this, screen, this, _window) {}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
};

void
SearchPage::OnOpen(struct mpdclient &c) noexcept
{
	ProxyPage::OnOpen(c);

	if (GetCurrentPage() == nullptr)
		SetCurrentPage(c, &filter_page);
}

bool
SearchPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ProxyPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::PLAY:
		if (GetCurrentPage() == &filter_page &&
		    filter_page.IsSearchButtonSelected()) {
			if (auto filter = filter_page.GetFilter(); !filter.IsEmpty()) {
				result_page.SetFilter(c, std::move(filter));
				SetCurrentPage(c, &result_page);
				return true;
			}
		}

		return false;

	case Command::SCREEN_SEARCH:
	case Command::GO_PARENT_DIRECTORY:
	case Command::GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &filter_page) {
			SetCurrentPage(c, &filter_page);
			return true;
		}

		return false;

	default:
		return false;
	}

	std::unreachable();
}

static std::unique_ptr<Page>
screen_search_init(ScreenManager &_screen, const Window window)
{
	return std::make_unique<SearchPage>(_screen, window);
}

const PageMeta screen_search = {
	"search",
	N_("Search"),
	Command::SCREEN_SEARCH,
	screen_search_init,
};
