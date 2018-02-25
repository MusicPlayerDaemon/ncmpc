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

#include "screen_outputs.hxx"
#include "screen_interface.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "screen_status.hxx"
#include "paint.hxx"
#include "i18n.h"
#include "mpdclient.hxx"

#include <mpd/client.h>

#include <vector>
#include <memory>

#include <assert.h>

struct OutputDeleter {
	void operator()(struct mpd_output *o) const {
		mpd_output_free(o);
	}
};

class OutputsPage final : public ListPage, ListRenderer {
	std::vector<std::unique_ptr<struct mpd_output, OutputDeleter>> items;

public:
	OutputsPage(WINDOW *w, Size size)
		:ListPage(w, size) {}

private:
	void Clear();

	bool Toggle(struct mpdclient &c, unsigned output_index);

public:
	/* virtual methods from class Page */
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			   bool selected) const override;
};

bool
OutputsPage::Toggle(struct mpdclient &c, unsigned output_index)
{
	if (output_index >= items.size())
		return false;

	struct mpd_connection *connection = mpdclient_get_connection(&c);
	if (connection == nullptr)
		return false;

	const auto &output = *items[output_index];
	if (!mpd_output_get_enabled(&output)) {
		if (!mpd_run_enable_output(connection,
					   mpd_output_get_id(&output))) {
			mpdclient_handle_error(&c);
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' enabled"),
				     mpd_output_get_name(&output));
	} else {
		if (!mpd_run_disable_output(connection,
					    mpd_output_get_id(&output))) {
			mpdclient_handle_error(&c);
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
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return;

	mpd_send_outputs(connection);

	struct mpd_output *output;
	while ((output = mpd_recv_output(connection)) != nullptr)
		items.emplace_back(output);

	mpdclient_finish_command(c);
}

static Page *
outputs_init(ScreenManager &, WINDOW *w, Size size)
{
	return new OutputsPage(w, size);
}

const char *
OutputsPage::GetTitle(gcc_unused char *str, gcc_unused size_t size) const
{
	return _("Outputs");
}

void
OutputsPage::PaintListItem(WINDOW *w, unsigned i,
			   gcc_unused unsigned y, unsigned width,
			   bool selected) const
{
	assert(i < items.size());
	const auto *output = items[i].get();

	row_color(w, COLOR_LIST, selected);
	waddstr(w, mpd_output_get_enabled(output) ? "[X] " : "[ ] ");
	waddstr(w, mpd_output_get_name(output));
	row_clear_to_eol(w, width, selected);
}

void
OutputsPage::Paint() const
{
	lw.Paint(*this);
}

void
OutputsPage::Update(struct mpdclient &c, unsigned events)
{
	if (events & MPD_IDLE_OUTPUT) {
		Clear();
		fill_outputs_list(&c, items);
		lw.SetLength(items.size());
		SetDirty();
	}
}

bool
OutputsPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case CMD_PLAY:
		Toggle(c, lw.selected);
		return true;

	case CMD_SCREEN_UPDATE:
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

const struct screen_functions screen_outputs = {
	"outputs",
	outputs_init,
};
