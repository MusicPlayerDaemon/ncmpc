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
#include "screen_status.hxx"
#include "paint.hxx"
#include "i18n.h"
#include "list_window.hxx"
#include "mpdclient.hxx"

#include <mpd/client.h>

#include <glib.h>
#include <assert.h>

static ListWindow *lw;

static GPtrArray *mpd_outputs = nullptr;

static bool
toggle_output(struct mpdclient *c, unsigned int output_index)
{
	assert(mpd_outputs != nullptr);

	if (output_index >= mpd_outputs->len)
		return false;

	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return false;

	auto *output = (struct mpd_output *)g_ptr_array_index(mpd_outputs,
							      output_index);
	if (!mpd_output_get_enabled(output)) {
		if (!mpd_run_enable_output(connection,
					   mpd_output_get_id(output))) {
			mpdclient_handle_error(c);
			return false;
		}

		c->events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' enabled"),
				     mpd_output_get_name(output));
	} else {
		if (!mpd_run_disable_output(connection,
					    mpd_output_get_id(output))) {
			mpdclient_handle_error(c);
			return false;
		}

		c->events |= MPD_IDLE_OUTPUT;

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

static void
clear_outputs_list()
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
fill_outputs_list(struct mpdclient *c)
{
	assert(mpd_outputs != nullptr);

	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr) {
		list_window_set_length(lw, 0);
		return;
	}

	mpd_send_outputs(connection);

	struct mpd_output *output;
	while ((output = mpd_recv_output(connection)) != nullptr) {
		g_ptr_array_add(mpd_outputs, output);
	}

	mpdclient_finish_command(c);

	list_window_set_length(lw, mpd_outputs->len);
}

static void
outputs_init(WINDOW *w, unsigned cols, unsigned rows)
{
	lw = new ListWindow(w, cols, rows);

	mpd_outputs = g_ptr_array_new();
}

static void
outputs_resize(unsigned cols, unsigned rows)
{
	list_window_resize(lw, cols, rows);
}

static void
outputs_exit()
{
	delete lw;

	g_ptr_array_free(mpd_outputs, true);
}

static void
outputs_open(struct mpdclient *c)
{
	fill_outputs_list(c);
}

static void
outputs_close()
{
	clear_outputs_list();
}

static const char *
outputs_title(gcc_unused char *str, gcc_unused size_t size)
{
	return _("Outputs");
}

static void
screen_outputs_paint_callback(WINDOW *w, unsigned i,
			      gcc_unused unsigned y, unsigned width,
			      bool selected, gcc_unused const void *data)
{
	assert(mpd_outputs != nullptr);
	assert(i < mpd_outputs->len);

	const auto *output = (const struct mpd_output *)
		g_ptr_array_index(mpd_outputs, i);

	row_color(w, COLOR_LIST, selected);
	waddstr(w, mpd_output_get_enabled(output) ? "[X] " : "[ ] ");
	waddstr(w, mpd_output_get_name(output));
	row_clear_to_eol(w, width, selected);
}

static void
outputs_paint()
{
	list_window_paint2(lw, screen_outputs_paint_callback, nullptr);
}

static void
screen_outputs_update(struct mpdclient *c)
{
	if (c->events & MPD_IDLE_OUTPUT) {
		clear_outputs_list();
		fill_outputs_list(c);
		outputs_paint();
	}
}

static bool
outputs_cmd(struct mpdclient *c, command_t cmd)
{
	assert(mpd_outputs != nullptr);

	if (list_window_cmd(lw, cmd)) {
		outputs_paint();
		return true;
	}

	switch (cmd) {
	case CMD_PLAY:
		toggle_output(c, lw->selected);
		return true;

	case CMD_SCREEN_UPDATE:
		clear_outputs_list();
		fill_outputs_list(c);
		outputs_paint();
		return true;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_outputs = {
	.init      = outputs_init,
	.exit      = outputs_exit,
	.open      = outputs_open,
	.close     = outputs_close,
	.resize    = outputs_resize,
	.paint     = outputs_paint,
	.update    = screen_outputs_update,
	.cmd       = outputs_cmd,
#ifdef HAVE_GETMOUSE
	.mouse = nullptr,
#endif
	.get_title = outputs_title,
};
