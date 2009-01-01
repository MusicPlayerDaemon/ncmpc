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

#include "i18n.h"
#include "screen.h"
#include "list_window.h"

#include <glib.h>

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
toggle_output(mpdclient_t *c, unsigned int output_index)
{
	int return_value;
	mpd_OutputEntity *output;

	assert(mpd_outputs != NULL);

	if (output_index >= mpd_outputs->len)
		return -1;

	output = g_ptr_array_index(mpd_outputs, output_index);

	if (output->enabled == 0) {
		mpd_sendEnableOutputCommand(c->connection, output->id);

		output->enabled = 1;

		screen_status_printf(_("Output '%s' enabled"), output->name);
	} else {
		mpd_sendDisableOutputCommand(c->connection, output->id);

		output->enabled = 0;

		screen_status_printf(_("Output '%s' disabled"), output->name);
	}

	return_value = mpdclient_finish_command(c);

	outputs_repaint();

	return return_value;
}

static void
clear_output_element(gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	mpd_freeOutputElement(data);
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
fill_outputs_list(mpdclient_t *c)
{
	mpd_OutputEntity *output;

	assert(mpd_outputs != NULL);

	mpd_sendOutputsCommand(c->connection);
	while ((output = mpd_getNextOutput(c->connection)) != NULL) {
		g_ptr_array_add(mpd_outputs, output);
	}
}

static const char *
outputs_list_callback(unsigned int output_index, bool *highlight,
		      G_GNUC_UNUSED void *data)
{
	mpd_OutputEntity *output;

	assert(mpd_outputs != NULL);

	if (output_index >= mpd_outputs->len)
		return NULL;

	output = g_ptr_array_index(mpd_outputs, output_index);

	if (output->enabled)
		*highlight = true;

	return output->name;
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
outputs_open(mpdclient_t *c)
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

static bool
outputs_cmd(mpdclient_t *c, command_t cmd)
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
	.cmd       = outputs_cmd,
	.get_title = outputs_title,
};
