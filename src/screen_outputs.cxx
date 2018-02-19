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
#include "screen_status.hxx"
#include "paint.hxx"
#include "i18n.h"
#include "mpdclient.hxx"

#include <mpd/client.h>

#include <glib.h>

#include <assert.h>

class OutputsPage final : public ListPage {
	GPtrArray *mpd_outputs = g_ptr_array_new();

public:
	OutputsPage(WINDOW *w, unsigned cols, unsigned rows)
		:ListPage(w, cols, rows) {}

	~OutputsPage() override {
		g_ptr_array_free(mpd_outputs, true);
	}

private:
	void Clear();

	bool Toggle(struct mpdclient &c, unsigned output_index);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void OnClose() override;
	void Paint() const override;
	void Update(struct mpdclient &c) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

bool
OutputsPage::Toggle(struct mpdclient &c, unsigned output_index)
{
	assert(mpd_outputs != nullptr);

	if (output_index >= mpd_outputs->len)
		return false;

	struct mpd_connection *connection = mpdclient_get_connection(&c);
	if (connection == nullptr)
		return false;

	auto *output = (struct mpd_output *)g_ptr_array_index(mpd_outputs,
							      output_index);
	if (!mpd_output_get_enabled(output)) {
		if (!mpd_run_enable_output(connection,
					   mpd_output_get_id(output))) {
			mpdclient_handle_error(&c);
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' enabled"),
				     mpd_output_get_name(output));
	} else {
		if (!mpd_run_disable_output(connection,
					    mpd_output_get_id(output))) {
			mpdclient_handle_error(&c);
			return false;
		}

		c.events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' disabled"),
				     mpd_output_get_name(output));
	}

	return true;
}

static void
clear_output_element(gpointer data, gcc_unused gpointer user_data)
{
	auto *output = (struct mpd_output *)data;
	mpd_output_free(output);
}

void
OutputsPage::Clear()
{
	assert(mpd_outputs != nullptr);

	if (mpd_outputs->len <= 0)
		return;

	g_ptr_array_foreach(mpd_outputs, clear_output_element, nullptr);
	g_ptr_array_remove_range(mpd_outputs, 0, mpd_outputs->len);

	/* not updating the list_window length here, because that
	   would clear the cursor position, and fill_outputs_list()
	   will be called after this function anyway */
	/* list_window_set_length(lw, 0); */
}

static void
fill_outputs_list(struct mpdclient *c, GPtrArray *mpd_outputs)
{
	assert(mpd_outputs != nullptr);

	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return;

	mpd_send_outputs(connection);

	struct mpd_output *output;
	while ((output = mpd_recv_output(connection)) != nullptr) {
		g_ptr_array_add(mpd_outputs, output);
	}

	mpdclient_finish_command(c);
}

static Page *
outputs_init(WINDOW *w, unsigned cols, unsigned rows)
{
	return new OutputsPage(w, cols, rows);
}

void
OutputsPage::OnOpen(struct mpdclient &c)
{
	fill_outputs_list(&c, mpd_outputs);
	list_window_set_length(&lw, mpd_outputs->len);
}

void
OutputsPage::OnClose()
{
	Clear();
}

const char *
OutputsPage::GetTitle(gcc_unused char *str, gcc_unused size_t size) const
{
	return _("Outputs");
}

static void
screen_outputs_paint_callback(WINDOW *w, unsigned i,
			      gcc_unused unsigned y, unsigned width,
			      bool selected, const void *data)
{
	const auto *mpd_outputs = (const GPtrArray *)data;
	assert(i < mpd_outputs->len);
	const auto *output = (const struct mpd_output *)
		g_ptr_array_index(mpd_outputs, i);

	row_color(w, COLOR_LIST, selected);
	waddstr(w, mpd_output_get_enabled(output) ? "[X] " : "[ ] ");
	waddstr(w, mpd_output_get_name(output));
	row_clear_to_eol(w, width, selected);
}

void
OutputsPage::Paint() const
{
	list_window_paint2(&lw, screen_outputs_paint_callback, mpd_outputs);
}

void
OutputsPage::Update(struct mpdclient &c)
{
	if (c.events & MPD_IDLE_OUTPUT) {
		Clear();
		fill_outputs_list(&c, mpd_outputs);
		list_window_set_length(&lw, mpd_outputs->len);
		SetDirty();
	}
}

bool
OutputsPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	assert(mpd_outputs != nullptr);

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case CMD_PLAY:
		Toggle(c, lw.selected);
		return true;

	case CMD_SCREEN_UPDATE:
		Clear();
		fill_outputs_list(&c, mpd_outputs);
		list_window_set_length(&lw, mpd_outputs->len);
		SetDirty();
		return true;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_outputs = {
	.init      = outputs_init,
};
