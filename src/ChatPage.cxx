// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ChatPage.hxx"
#include "PageMeta.hxx"
#include "screen.hxx"
#include "screen_status.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "Command.hxx"
#include "Options.hxx"
#include "page/TextPage.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "client/mpdclient.hxx"
#include "util/StringAPI.hxx"

#include <mpd/idle.h>

#include <fmt/core.h>

#include <string.h>
#include <stdlib.h>

using std::string_view_literals::operator""sv;

static constexpr char chat_channel[] = "chat";

class ChatPage final : public TextPage {
	ScreenManager &screen;

	unsigned last_connection_id = 0;
	bool was_supported = false;

	std::string prefix;

public:
	ChatPage(ScreenManager &_screen, const Window _window)
		:TextPage(_screen, _window, _screen.find_support),
		 screen(_screen) {}

private:
	bool CheckChatSupport(struct mpdclient &c);

	void ProcessMessage(const struct mpd_message &message);

	[[gnu::pure]]
	const std::string &GetPrefix() noexcept;

	void SendMessage(struct mpdclient &c, const char *msg) noexcept;

	[[nodiscard]]
	Co::InvokeTask EnterMessage(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

	std::string_view GetTitle(std::span<char>) const noexcept override {
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

	/* mpdclient_get_connection? */
	if (!mpdclient_cmd_subscribe(c, chat_channel))
		return (was_supported = false);
	/* mpdclient_put_connection? */

	return (was_supported = true);
}

static std::unique_ptr<Page>
screen_chat_init(ScreenManager &screen, const Window window)
{
	return std::make_unique<ChatPage>(screen, window);
}

void
ChatPage::ProcessMessage(const struct mpd_message &message)
{
	/* You'll have to move this out of screen_chat, if you want to use
	   client-to-client messages anywhere else */
	assert(StringIsEqual(mpd_message_get_channel(&message), chat_channel));

	Append(mpd_message_get_text(&message));
}

void
ChatPage::Update(struct mpdclient &c, unsigned events) noexcept
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

const std::string &
ChatPage::GetPrefix() noexcept
{
	if (!prefix.empty())
		return prefix;

	if (!options.chat_prefix.empty()) {
		/* Options are encoded in the "locale" charset */
		prefix = LocaleToUtf8{options.chat_prefix};
		return prefix;
	}

	const char *user = getenv("USER");
	if (user == nullptr)
		user = "nobody";

	prefix = fmt::format("<{}>"sv, user);
	return prefix;
}

inline void
ChatPage::SendMessage(struct mpdclient &c, const char *msg) noexcept
{
	const std::string full_msg = fmt::format("{}{}"sv, GetPrefix(), (std::string_view)LocaleToUtf8{msg});

	(void) mpdclient_cmd_send_message(c, chat_channel, full_msg.c_str());
}

inline Co::InvokeTask
ChatPage::EnterMessage(struct mpdclient &c)
{
	const auto message = co_await TextInputDialog{
		screen, _("Your message"),
	};

	/* the user entered an empty line */
	if (message.empty())
		co_return;

	if (CheckChatSupport(c))
		SendMessage(c, message.c_str());
	else
		Alert(_("Message could not be sent"));
}

bool
ChatPage::OnCommand(struct mpdclient &c, Command cmd)
{
	CoCancel();

	if (TextPage::OnCommand(c, cmd))
		return true;

	if (cmd == Command::PLAY) {
		CoStart(EnterMessage(c));
		return true;
	}

	return false;
}

const PageMeta screen_chat = {
	"chat",
	N_("Chat"),
	Command::SCREEN_CHAT,
	screen_chat_init,
};
