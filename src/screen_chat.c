/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2012 The Music Player Daemon Project
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

#include "screen_chat.h"

#include "screen_interface.h"
#include "screen_utils.h"
#include "screen_status.h"
#include "screen_text.h"
#include "mpdclient.h"
#include "i18n.h"
#include "charset.h"
#include "options.h"

#include <glib.h>
#include <mpd/idle.h>

static struct screen_text text; /* rename? */
static const char chat_channel[] = "chat";

static bool
check_chat_support(struct mpdclient *c)
{
	static unsigned last_connection_id = 0;
	static bool was_supported = false;

	/* if we're using the same connection as the last time
	   check_chat_support was called, we can use the cached information
	   about whether chat is supported */
	if (c->connection_id == last_connection_id)
		return was_supported;

	last_connection_id = c->connection_id;

	if (c->connection == NULL)
		return (was_supported = false);

	if (mpd_connection_cmp_server_version(c->connection, 0, 17, 0) == -1) {
		const unsigned *version = mpd_connection_get_server_version(c->connection);
		char *str;

		str = g_strdup_printf(
			_("connected to MPD %u.%u.%u (you need at least \n"
			  "version 0.17.0 to use the chat feature)"),
			version[0], version[1], version[2]);
		screen_text_append(&text, str);
		g_free(str);

		return (was_supported = false);
	}

	/* mpdclient_get_connection? */
	if (!mpdclient_cmd_subscribe(c, chat_channel))
		return (was_supported = false);
	/* mpdclient_put_connection? */

	return (was_supported = true);
}

static void
screen_chat_init(WINDOW *w, int cols, int rows)
{
	screen_text_init(&text, w, cols, rows);
}

static void
screen_chat_exit(void)
{
	screen_text_deinit(&text);
}

static void
screen_chat_open(struct mpdclient *c)
{
	(void) check_chat_support(c);
}

static void
screen_chat_resize(int cols, int rows)
{
	screen_text_resize(&text, cols, rows);
}

static void
screen_chat_paint(void)
{
	screen_text_paint(&text);
}

static void
process_message(struct mpd_message *message)
{
	char *message_text;

	assert(message != NULL);
	/* You'll have to move this out of screen_chat, if you want to use
	   client-to-client messages anywhere else */
	assert(g_strcmp0(mpd_message_get_channel(message), chat_channel) == 0);

	message_text = utf8_to_locale(mpd_message_get_text(message));
	screen_text_append(&text, message_text);
	g_free(message_text);

	screen_chat_paint();
}

static void
screen_chat_update(struct mpdclient *c)
{
	if (check_chat_support(c) && (c->events & MPD_IDLE_MESSAGE)) {
		if (!mpdclient_send_read_messages(c))
			return;

		struct mpd_message *message;
		while ((message = mpdclient_recv_message(c)) != NULL) {
			process_message(message);
			mpd_message_free(message);
		}

		mpdclient_finish_command(c);

		c->events &= ~MPD_IDLE_MESSAGE;
	}
}

static char *
screen_chat_get_prefix(void)
{
	static char *prefix = NULL;

	if (prefix)
		return prefix;

	if (options.chat_prefix) {
		/* Options are encoded in the "locale" charset */
		prefix = locale_to_utf8(options.chat_prefix);
		return prefix;
	}

	prefix = g_strconcat("<", g_get_user_name(), "> ", NULL);
	return prefix;
}

static void
screen_chat_send_message(struct mpdclient *c, char *msg)
{
	char *utf8 = locale_to_utf8(msg);
	char *prefix = screen_chat_get_prefix();
	char *full_msg = g_strconcat(prefix, utf8, NULL);
	g_free(utf8);

	(void) mpdclient_cmd_send_message(c, chat_channel, full_msg);
	g_free(full_msg);
}

static bool
screen_chat_cmd(struct mpdclient *c, command_t cmd)
{
	if (screen_text_cmd(&text, c, cmd))
		return true;

	if (cmd == CMD_PLAY) {
		char *message = screen_readln(_("Your message"), NULL, NULL, NULL);

		/* the user entered an empty line */
		if (message == NULL)
			return true;

		if (check_chat_support(c))
			screen_chat_send_message(c, message);
		else
			screen_status_message(_("Message could not be sent"));

		g_free(message);

		return true;
	}

	return false;
}

static const char *
screen_chat_title(gcc_unused char *s, gcc_unused size_t size)
{
	return _("Chat");
}

const struct screen_functions screen_chat = {
	.init = screen_chat_init,
	.exit = screen_chat_exit,
	.open = screen_chat_open,
	/* close */
	.resize = screen_chat_resize,
	.paint = screen_chat_paint,
	.update = screen_chat_update,
	.cmd = screen_chat_cmd,
	.get_title = screen_chat_title,
};
