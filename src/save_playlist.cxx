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

#include "save_playlist.hxx"
#include "db_completion.hxx"
#include "screen_status.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "utils.hxx"
#include "wreadln.hxx"
#include "screen_utils.hxx"
#include "Compiler.h"

#include <mpd/client.h>

#include <glib.h>

#include <string.h>

#ifndef NCMPC_MINI

typedef struct
{
	GList **list;
	struct mpdclient *c;
} completion_callback_data_t;

/**
 * Wrapper for strncmp().  We are not allowed to pass &strncmp to
 * g_completion_set_compare(), because strncmp() takes size_t where
 * g_completion_set_compare passes a gsize value.
 */
static gint
completion_strncmp(const gchar *s1, const gchar *s2, gsize n)
{
	return strncmp(s1, s2, n);
}

static void
save_pre_completion_cb(GCompletion *gcmp, gcc_unused gchar *line,
		       void *data)
{
	completion_callback_data_t *tmp = (completion_callback_data_t *)data;
	GList **list = tmp->list;
	struct mpdclient *c = tmp->c;

	if( *list == nullptr ) {
		/* create completion list */
		*list = gcmp_list_from_path(c, "", nullptr, GCMP_TYPE_PLAYLIST);
		g_completion_add_items(gcmp, *list);
	}
}

static void
save_post_completion_cb(gcc_unused GCompletion *gcmp,
			gcc_unused gchar *line, GList *items,
			gcc_unused void *data)
{
	if (g_list_length(items) >= 1)
		screen_display_completion_list(items);
}

#endif

int
playlist_save(struct mpdclient *c, char *name, char *defaultname)
{
	struct mpd_connection *connection;
	gchar *filename;

#ifdef NCMPC_MINI
	(void)defaultname;
#endif

#ifndef NCMPC_MINI
	if (name == nullptr) {
		/* initialize completion support */
		GCompletion *gcmp = g_completion_new(nullptr);
		g_completion_set_compare(gcmp, completion_strncmp);
		GList *list = nullptr;
		completion_callback_data_t data = {
			.list = &list,
			.c = c,
		};
		wrln_completion_callback_data = &data;
		wrln_pre_completion_callback = save_pre_completion_cb;
		wrln_post_completion_callback = save_post_completion_cb;


		/* query the user for a filename */
		filename = screen_readln(_("Save queue as"),
					 defaultname,
					 nullptr,
					 gcmp);
		if (filename == nullptr)
			return -1;

		/* destroy completion support */
		wrln_completion_callback_data = nullptr;
		wrln_pre_completion_callback = nullptr;
		wrln_post_completion_callback = nullptr;
		g_completion_free(gcmp);
		list = string_list_free(list);
		filename = g_strstrip(filename);
	} else
#endif
		filename=g_strdup(name);

	/* send save command to mpd */

	connection = mpdclient_get_connection(c);
	if (connection == nullptr) {
		g_free(filename);
		return -1;
	}

	const LocaleToUtf8 filename_utf8(filename);
	if (!mpd_run_save(connection, filename_utf8.c_str())) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char *buf = g_strdup_printf(_("Replace %s?"), filename);
			bool replace = screen_get_yesno(buf, false);
			g_free(buf);

			if (!replace) {
				g_free(filename);
				screen_status_printf(_("Aborted"));
				return -1;
			}

			if (!mpd_run_rm(connection, filename_utf8.c_str()) ||
			    !mpd_run_save(connection, filename_utf8.c_str())) {
				mpdclient_handle_error(c);
				g_free(filename);
				return -1;
			}
		} else {
			mpdclient_handle_error(c);
			g_free(filename);
			return -1;
		}
	}

	c->events |= MPD_IDLE_STORED_PLAYLIST;

	/* success */
	screen_status_printf(_("Saved %s"), filename);
	g_free(filename);
	return 0;
}
