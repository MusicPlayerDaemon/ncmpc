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

#include "ChatPage.hxx"
#include "screen_interface.hxx"
#include "screen_utils.hxx"
#include "screen_status.hxx"
#include "TextPage.hxx"
#include "mpdclient.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "options.hxx"

#include <mpd/idle.h>

#include <glib.h>

#include <string.h>

static constexpr char chat_channel[] = "chat";

class ChatPage final : public TextPage {
	unsigned last_connection_id = 0;
	bool was_supported = false;

public:
	ChatPage(ScreenManager &_screen, WINDOW *w, Size size)
		:TextPage(_screen, w, size) {}

private:
	bool CheckChatSupport(struct mpdclient &c);

	void ProcessMessage(const struct mpd_message &message);

public:
	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

	const char *GetTitle(char *, size_t) const override {
		return _("Chat");
	}
};

bool
ChatPage::CheckChatSupport(struct mpdclient &c)
{
	/* if we're using the same connection as the last time
	   check_chat_support was called, we can use the cached information
	   about whether chat is supported */
	if (c.connection_id == last_connection_id)
		return was_supported;

	last_connection_id = c.connection_id;

	if (c.connection == nullptr)
		return (was_supported = false);

	if (mpd_connection_cmp_server_version(c.connection, 0, 17, 0) == -1) {
		const unsigned *version = mpd_connection_get_server_version(c.connection);
		char *str;

		str = g_strdup_printf(
			_("connected to MPD %u.%u.%u (you need at least \n"
			  "version 0.17.0 to use the chat feature)"),
			version[0], version[1], version[2]);
		Append(str);
		g_free(str);

		return (was_supported = false);
	}

	/* mpdclient_get_connection? */
	if (!mpdclient_cmd_subscribe(&c, chat_channel))
		return (was_supported = false);
	/* mpdclient_put_connection? */

	return (was_supported = true);
}

static Page *
screen_chat_init(ScreenManager &screen, WINDOW *w, Size size)
{
	return new ChatPage(screen, w, size);
}

void
ChatPage::ProcessMessage(const struct mpd_message &message)
{
	/* You'll have to move this out of screen_chat, if you want to use
	   client-to-client messages anywhere else */
	assert(strcmp(mpd_message_get_channel(&message), chat_channel) == 0);

	Append(Utf8ToLocale(mpd_message_get_text(&message)).c_str());

	SetDirty();
}

void
ChatPage::Update(struct mpdclient &c, unsigned events)
{
	if (CheckChatSupport(c) && (events & MPD_IDLE_MESSAGE)) {
		auto *connection = c.GetConnection();
		if (connection == nullptr)
			return;

		if (!mpd_send_read_messages(connection)) {
			c.HandleError();
			return;
		}

		struct mpd_message *message;
		while ((message = mpd_recv_message(connection)) != nullptr) {
			ProcessMessage(*message);
			mpd_message_free(message);
		}

		c.FinishCommand();
	}
}

static char *
screen_chat_get_prefix()
{
	static char *prefix = nullptr;

	if (prefix)
		return prefix;

	if (!options.chat_prefix.empty()) {
		/* Options are encoded in the "locale" charset */
		prefix = locale_to_utf8(options.chat_prefix.c_str());
		return prefix;
	}

	prefix = g_strconcat("<", g_get_user_name(), "> ", nullptr);
	return prefix;
}

static void
screen_chat_send_message(struct mpdclient *c, const char *msg)
{
	char *prefix = screen_chat_get_prefix();
	char *full_msg = g_strconcat(prefix, LocaleToUtf8(msg).c_str(),
				     nullptr);

	(void) mpdclient_cmd_send_message(c, chat_channel, full_msg);
	g_free(full_msg);
}

bool
ChatPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (TextPage::OnCommand(c, cmd))
		return true;

	if (cmd == CMD_PLAY) {
		auto message = screen_readln(_("Your message"), nullptr, nullptr, nullptr);

		/* the user entered an empty line */
		if (message.empty())
			return true;

		if (CheckChatSupport(c))
			screen_chat_send_message(&c, message.c_str());
		else
			screen_status_message(_("Message could not be sent"));

		return true;
	}

	return false;
}

const struct screen_functions screen_chat = {
	"chat",
	screen_chat_init,
};
