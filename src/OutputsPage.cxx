// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "OutputsPage.hxx"
#include "Deleter.hxx"
#include "PageMeta.hxx"
#include "screen.hxx"
#include "Command.hxx"
#include "i18n.h"
#include "page/ListPage.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/paint.hxx"
#include "client/mpdclient.hxx"
#include "util/FNVHash.hxx"
#include "util/StringAPI.hxx"

#include <mpd/client.h>

#include <vector>
#include <memory>

#include <assert.h>

using std::string_view_literals::operator""sv;

[[gnu::pure]]
static uint64_t
PartitionNameHash(const char *name) noexcept
{
	return FNV1aHash64(name) ^ 0x1;
}

[[gnu::pure]]
static uint64_t
GetActivePartitionNameHash(const struct mpd_status *status) noexcept
{
	if (status == nullptr)
		return 0;

	const char *partition = mpd_status_get_partition(status);
	if (partition == nullptr)
		return 0;

	return PartitionNameHash(partition);
}

class OutputsPage final : public ListPage, ListRenderer {
	static constexpr unsigned RELOAD_IDLE_FLAGS =
		MPD_IDLE_PARTITION |
		MPD_IDLE_OUTPUT;

	struct Item {
		std::unique_ptr<struct mpd_output, LibmpdclientDeleter> output;
		std::unique_ptr<struct mpd_partition, LibmpdclientDeleter> partition;

		enum class Special {
			NONE,
			NEW_PARTITION,
		} special = Special::NONE;

		explicit Item(Special _special) noexcept
			:special(_special) {}

		explicit Item(struct mpd_output *_output) noexcept
			:output(_output) {}

		explicit Item(struct mpd_partition *_partition) noexcept
			:partition(_partition) {}

		[[gnu::pure]]
		uint64_t GetHash() const noexcept {
			switch (special) {
			case Special::NONE:
				break;

			case Special::NEW_PARTITION:
				return 0x2;
			}

			if (partition) {
				return PartitionNameHash(mpd_partition_get_name(partition.get()));
			}

			return FNV1aHash64(mpd_output_get_name(output.get()));
		}
	};

	ScreenManager &screen;

	std::vector<Item> items;

	uint64_t active_partition = 0;

public:
	OutputsPage(ScreenManager &_screen, const Window window)
		:ListPage(_screen, window), screen(_screen) {}

private:
	void Clear();
	void Reload(struct mpdclient &c) noexcept;

	bool ActivatePartition(struct mpdclient &c,
			       const struct mpd_partition &partition) noexcept;

	[[nodiscard]]
	Co::InvokeTask CreateNewPartition(struct mpdclient &c) noexcept;

	bool Toggle(struct mpdclient &c, unsigned output_index);

	bool Delete(struct mpdclient &c, unsigned idx);

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(Window window, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;
};

inline bool
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

	FmtAlert(_("Switched to partition '{}'"),
		 partition_name);
	return true;
}

inline Co::InvokeTask
OutputsPage::CreateNewPartition(struct mpdclient &c) noexcept
{
	if (!c.IsConnected())
		co_return;

	const auto name = co_await TextInputDialog{
		screen, _("Name"),
	};
	if (name.empty())
		co_return;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		co_return;

	if (!mpd_run_newpartition(connection, name.c_str())) {
		c.HandleError();
	}
}

inline bool
OutputsPage::Toggle(struct mpdclient &c, unsigned output_index)
{
	if (output_index >= items.size())
		return false;

	const auto &item = items[output_index];
	switch (item.special) {
	case Item::Special::NONE:
		break;

	case Item::Special::NEW_PARTITION:
		CoStart(CreateNewPartition(c));
		return true;
	}

	if (item.partition)
		return ActivatePartition(c, *item.partition);

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

		FmtAlert(_("Output '{}' enabled"),
			 mpd_output_get_name(&output));
	} else {
		if (!mpd_run_disable_output(connection,
					    mpd_output_get_id(&output))) {
			c.HandleError();
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		FmtAlert(_("Output '{}' disabled"),
			 mpd_output_get_name(&output));
	}

	return true;
}

bool
OutputsPage::Delete(struct mpdclient &c, unsigned idx)
{
	const auto &item = items[idx];

	if (item.partition) {
		auto *connection = c.GetConnection();
		if (connection == nullptr)
			return false;

		const char *name =
			mpd_partition_get_name(item.partition.get());
		if (!mpd_run_delete_partition(connection, name))
			c.HandleError();

		return true;
	} else
		return false;
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
		const char *plugin = mpd_output_get_plugin(output);
		if (plugin != nullptr && StringIsEqual(plugin, "dummy")) {
			/* hide "dummy" outputs; they are placeholders
			   for an output which was moved to a
			   different partition */
			mpd_output_free(output);
			continue;
		}

		items.emplace_back(output);
	}

	c->FinishCommand();
}

template<typename O>
static void
FillPartitionList(struct mpdclient &c, O &items)
{
	using Item = typename O::value_type;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return;

	if (mpd_connection_cmp_server_version(connection, 0, 22, 0) < 0)
		return;

	mpd_send_listpartitions(connection);

	while (auto *partition = mpd_recv_partition(connection))
		items.emplace_back(partition);

	c.FinishCommand();

	items.emplace_back(Item::Special::NEW_PARTITION);
}

inline void
OutputsPage::Reload(struct mpdclient &c) noexcept
{
	const auto hash = lw.GetCursorHash(items);

	Clear();

	fill_outputs_list(&c, items);

	FillPartitionList(c, items);

	lw.SetLength(items.size());
	SchedulePaint();

	/* restore the cursor position */
	lw.SetCursorHash(items, hash);
}

static std::unique_ptr<Page>
outputs_init(ScreenManager &screen, const Window window)
{
	return std::make_unique<OutputsPage>(screen, window);
}

std::string_view 
OutputsPage::GetTitle(std::span<char>) const noexcept
{
	return _("Outputs");
}

static void
PaintPartition(const Window window, unsigned width, bool selected, bool active,
	       const struct mpd_partition &partition) noexcept
{
	const char *name = mpd_partition_get_name(&partition);

	row_color(window, active ? Style::LIST_BOLD : Style::LIST, selected);
	window.String(_("Partition"));
	window.String(": "sv);
	window.String(name);
	row_clear_to_eol(window, width, selected);
}

void
OutputsPage::PaintListItem(Window window, unsigned i,
			   [[maybe_unused]] unsigned y, unsigned width,
			   bool selected) const noexcept
{
	assert(i < items.size());
	const auto &item = items[i];

	switch (item.special) {
	case Item::Special::NONE:
		break;

	case Item::Special::NEW_PARTITION:
		row_color(window, Style::LIST, selected);
		window.Char('[');
		window.String(_("Create new partition"));
		window.Char(']');
		row_clear_to_eol(window, width, selected);
		return;
	}

	if (item.partition) {
		PaintPartition(window, width, selected,
			       active_partition == item.GetHash(),
			       *item.partition);
		return;
	}


	assert(item.output);

	const auto *output = item.output.get();

	row_color(window, Style::LIST, selected);
	window.String(mpd_output_get_enabled(output) ? "[X] "sv : "[ ] "sv);
	window.String(mpd_output_get_name(output));
	row_clear_to_eol(window, width, selected);
}

void
OutputsPage::Paint() const noexcept
{
	lw.Paint(*this);
}

void
OutputsPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	active_partition = GetActivePartitionNameHash(c.status);

	if (events & RELOAD_IDLE_FLAGS)
		Reload(c);
}

bool
OutputsPage::OnCommand(struct mpdclient &c, Command cmd)
{
	CoCancel();

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
		SchedulePaint();
		return true;

	case Command::DELETE:
		return Delete(c, lw.GetCursorIndex());

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
