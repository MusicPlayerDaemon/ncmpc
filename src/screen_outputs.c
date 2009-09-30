/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "screen_interface.h"
#include "i18n.h"
#include "screen.h"
#include "list_window.h"
#include "mpdclient.h"

#include <mpd/client.h>

#include <glib.h>
#include <assert.h>

static list_window_t *lw = NULL;

static GPtrArray *mpd_outputs = NULL;

static void
outputs_paint(void);

static void
outputs_repaint(void)
{
	outputs_paint();
	wrefresh(lw->w);
}

static int
toggle_output(struct mpdclient *c, unsigned int output_index)
{
	struct mpd_output *output;

	assert(mpd_outputs != NULL);

	if (output_index >= mpd_outputs->len)
		return -1;

	output = g_ptr_array_index(mpd_outputs, output_index);

	if (!mpd_output_get_enabled(output)) {
		if (!mpd_run_enable_output(c->connection,
					   mpd_output_get_id(output))) {
			mpdclient_handle_error(c);
			return -1;
		}

		c->events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' enabled"),
				     mpd_output_get_name(output));
	} else {
		if (!mpd_run_disable_output(c->connection,
					    mpd_output_get_id(output))) {
			mpdclient_handle_error(c);
			return -1;
		}

		c->events |= MPD_IDLE_OUTPUT;

		screen_status_printf(_("Output '%s' disabled"),
				     mpd_output_get_name(output));
	}

	return 0;
}

static void
clear_output_element(gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	mpd_output_free(data);
}

static void
clear_outputs_list(void)
{
	assert(mpd_outputs != NULL);

	if (mpd_outputs->len <= 0)
		return;

	g_ptr_array_foreach(mpd_outputs, clear_output_element, NULL);
	g_ptr_array_remove_range(mpd_outputs, 0, mpd_outputs->len);
}

static void
fill_outputs_list(struct mpdclient *c)
{
	struct mpd_output *output;

	assert(mpd_outputs != NULL);

	if (c->connection == NULL)
		return;

	mpd_send_outputs(c->connection);
	while ((output = mpd_recv_output(c->connection)) != NULL) {
		g_ptr_array_add(mpd_outputs, output);
	}

	if (!mpd_response_finish(c->connection))
		mpdclient_handle_error(c);
}

static const char *
outputs_list_callback(unsigned int output_index, bool *highlight,
		      G_GNUC_UNUSED char **sc, G_GNUC_UNUSED void *data)
{
	struct mpd_output *output;

	assert(mpd_outputs != NULL);

	if (output_index >= mpd_outputs->len)
		return NULL;

	output = g_ptr_array_index(mpd_outputs, output_index);

	if (mpd_output_get_enabled(output))
		*highlight = true;

	return mpd_output_get_name(output);
}

static void
outputs_init(WINDOW *w, int cols, int rows)
{
	lw = list_window_init(w, cols, rows);

	mpd_outputs = g_ptr_array_new();
}

static void
outputs_resize(int cols, int rows)
{
	lw->cols = cols;
	lw->rows = rows;
}

static void
outputs_exit(void)
{
	list_window_free(lw);

	g_ptr_array_free(mpd_outputs, TRUE);
}

static void
outputs_open(struct mpdclient *c)
{
	fill_outputs_list(c);
}

static void
outputs_close(void)
{
	clear_outputs_list();
}

static const char *
outputs_title(G_GNUC_UNUSED char *str, G_GNUC_UNUSED size_t size)
{
	return _("Outputs");
}

static void
outputs_paint(void)
{
	list_window_paint(lw, outputs_list_callback, NULL);
}

static void
screen_outputs_update(struct mpdclient *c)
{
	if (c->events & MPD_IDLE_OUTPUT) {
		clear_outputs_list();
		fill_outputs_list(c);
		outputs_repaint();
	}
}

static bool
outputs_cmd(struct mpdclient *c, command_t cmd)
{
	assert(mpd_outputs != NULL);

	if (list_window_cmd(lw, mpd_outputs->len, cmd)) {
		outputs_repaint();
		return true;
	}

	if (cmd == CMD_PLAY) {
		toggle_output(c, lw->selected);
		return true;
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
	.update = screen_outputs_update,
	.cmd       = outputs_cmd,
	.get_title = outputs_title,
};
