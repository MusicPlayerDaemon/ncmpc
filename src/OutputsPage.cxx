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

#include "OutputsPage.hxx"
#include "Deleter.hxx"
#include "PageMeta.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "screen_status.hxx"
#include "paint.hxx"
#include "Command.hxx"
#include "i18n.h"
#include "mpdclient.hxx"
#include "util/FNVHash.hxx"
#include "util/StringAPI.hxx"

#include <mpd/client.h>

#include <vector>
#include <memory>

#include <assert.h>

class OutputsPage final : public ListPage, ListRenderer {
	static constexpr unsigned RELOAD_IDLE_FLAGS =
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
		MPD_IDLE_PARTITION |
#endif
		MPD_IDLE_OUTPUT;

	struct Item {
		std::unique_ptr<struct mpd_output, LibmpdclientDeleter> output;
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
		std::unique_ptr<struct mpd_partition, LibmpdclientDeleter> partition;
#endif

		explicit Item(struct mpd_output *_output) noexcept
			:output(_output) {}

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
		explicit Item(struct mpd_partition *_partition) noexcept
			:partition(_partition) {}
#endif

		gcc_pure
		uint64_t GetHash() const noexcept {
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
			if (partition) {
				return FNV1aHash64(mpd_partition_get_name(partition.get()))
					^ 0x1;
			}
#endif

			return FNV1aHash64(mpd_output_get_name(output.get()));
		}
	};

	std::vector<Item> items;

public:
	OutputsPage(WINDOW *w, Size size)
		:ListPage(w, size) {}

private:
	void Clear();
	void Reload(struct mpdclient &c) noexcept;

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	bool ActivatePartition(struct mpdclient &c,
			       const struct mpd_partition &partition) noexcept;
#endif

	bool Toggle(struct mpdclient &c, unsigned output_index);

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const noexcept override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;
};

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)

bool
OutputsPage::ActivatePartition(struct mpdclient &c,
			       const struct mpd_partition &partition) noexcept
{
	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return false;

	const char *partition_name = mpd_partition_get_name(&partition);

	if (!mpd_run_switch_partition(connection, partition_name)) {
		c.HandleError();
		return false;
	}

	screen_status_printf(_("Switched to partition '%s'"),
			     partition_name);
	return true;
}

#endif

bool
OutputsPage::Toggle(struct mpdclient &c, unsigned output_index)
{
	if (output_index >= items.size())
		return false;

	const auto &item = items[output_index];
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	if (item.partition)
		return ActivatePartition(c, *item.partition);
#endif

	assert(item.output);

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return false;

	const auto &output = *item.output;
	if (!mpd_output_get_enabled(&output)) {
		if (!mpd_run_enable_output(connection,
					   mpd_output_get_id(&output))) {
			c.HandleError();
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' enabled"),
				     mpd_output_get_name(&output));
	} else {
		if (!mpd_run_disable_output(connection,
					    mpd_output_get_id(&output))) {
			c.HandleError();
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' disabled"),
				     mpd_output_get_name(&output));
	}

	return true;
}

void
OutputsPage::Clear()
{
	if (items.empty())
		return;

	items.clear();

	/* not updating the list_window length here, because that
	   would clear the cursor position, and fill_outputs_list()
	   will be called after this function anyway */
	/* lw.SetLength(0); */
}

template<typename O>
static void
fill_outputs_list(struct mpdclient *c, O &items)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	mpd_send_outputs(connection);

	struct mpd_output *output;
	while ((output = mpd_recv_output(connection)) != nullptr) {
#if LIBMPDCLIENT_CHECK_VERSION(2,14,0)
		const char *plugin = mpd_output_get_plugin(output);
		if (plugin != nullptr && StringIsEqual(plugin, "dummy")) {
			/* hide "dummy" outputs; they are placeholders
			   for an output which was moved to a
			   different partition */
			mpd_output_free(output);
			continue;
		}
#endif

		items.emplace_back(output);
	}

	c->FinishCommand();
}

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)

template<typename O>
static void
FillPartitionList(struct mpdclient &c, O &items)
{
	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return;

	mpd_send_listpartitions(connection);

	while (auto *pair = mpd_recv_partition_pair(connection)) {
		auto *partition = mpd_partition_new(pair);
		mpd_return_pair(connection, pair);
		items.emplace_back(partition);
	}

	c.FinishCommand();
}

#endif

inline void
OutputsPage::Reload(struct mpdclient &c) noexcept
{
	const auto hash = lw.GetCursorHash(items);

	Clear();

	fill_outputs_list(&c, items);

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	FillPartitionList(c, items);
#endif

	lw.SetLength(items.size());
	SetDirty();

	/* restore the cursor position */
	lw.SetCursorHash(items, hash);
}

static std::unique_ptr<Page>
outputs_init(ScreenManager &, WINDOW *w, Size size)
{
	return std::make_unique<OutputsPage>(w, size);
}

const char *
OutputsPage::GetTitle(char *, size_t) const noexcept
{
	return _("Outputs");
}

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)

static void
PaintPartition(WINDOW *w, unsigned width, bool selected,
	       const struct mpd_partition &partition) noexcept
{
	const char *name = mpd_partition_get_name(&partition);

	row_color(w, Style::LIST, selected);
	waddstr(w, _("Partition"));
	waddstr(w, ": ");
	waddstr(w, name);
	row_clear_to_eol(w, width, selected);
}

#endif

void
OutputsPage::PaintListItem(WINDOW *w, unsigned i,
			   gcc_unused unsigned y, unsigned width,
			   bool selected) const noexcept
{
	assert(i < items.size());
	const auto &item = items[i];

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	if (item.partition) {
		PaintPartition(w, width, selected, *item.partition);
		return;
	}
#endif

	assert(item.output);

	const auto *output = item.output.get();

	row_color(w, Style::LIST, selected);
	waddstr(w, mpd_output_get_enabled(output) ? "[X] " : "[ ] ");
	waddstr(w, mpd_output_get_name(output));
	row_clear_to_eol(w, width, selected);
}

void
OutputsPage::Paint() const noexcept
{
	lw.Paint(*this);
}

void
OutputsPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (events & RELOAD_IDLE_FLAGS)
		Reload(c);
}

bool
OutputsPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case Command::PLAY:
		Toggle(c, lw.GetCursorIndex());
		return true;

	case Command::SCREEN_UPDATE:
		Clear();
		fill_outputs_list(&c, items);
		lw.SetLength(items.size());
		SetDirty();
		return true;

	default:
		break;
	}

	return false;
}

const PageMeta screen_outputs = {
	"outputs",
	N_("Outputs"),
	Command::SCREEN_OUTPUTS,
	outputs_init,
};
