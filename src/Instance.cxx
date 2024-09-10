// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Instance.hxx"
#include "Options.hxx"
#include "event/SignalMonitor.hxx"
#include "util/Exception.hxx" // for GetFullMessage()

#include <signal.h>

#include "strfsong.hxx"
#include "TagMask.hxx"

static constexpr TagMask global_tag_whitelist{
	/* these tags are used by SongPage.cxx */
	MPD_TAG_ARTIST,
	MPD_TAG_TITLE,
	MPD_TAG_ALBUM,
	MPD_TAG_COMPOSER,
	MPD_TAG_PERFORMER,
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	MPD_TAG_CONDUCTOR,
	MPD_TAG_WORK,
#endif
	MPD_TAG_NAME,
	MPD_TAG_DISC,
	MPD_TAG_TRACK,
	MPD_TAG_DATE,
	MPD_TAG_GENRE,
	MPD_TAG_COMMENT,
};

Instance::Instance()
	:client(event_loop,
		options.host.empty() ? nullptr : options.host.c_str(),
		options.port,
		options.timeout_ms,
		options.password.empty() ? nullptr : options.password.c_str(),
		*this),
	 seek(event_loop, client),
	 reconnect_timer(event_loop, BIND_THIS_METHOD(OnReconnectTimer)),
	 update_timer(event_loop, BIND_THIS_METHOD(OnUpdateTimer)),
#ifndef NCMPC_MINI
	 check_key_bindings_timer(event_loop, BIND_THIS_METHOD(OnCheckKeyBindings)),
#endif
	 screen_manager(event_loop),
#ifdef ENABLE_LIRC
	 lirc_input(event_loop, *this),
#endif
	 user_input(event_loop, *screen_manager.main_window.w, *this)
{
	screen_manager.Init(&client);

#ifndef _WIN32
	SignalMonitorInit(event_loop);
	SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGHUP, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGWINCH, BIND_THIS_METHOD(OnSigwinch));
	SignalMonitorRegister(SIGCONT, BIND_THIS_METHOD(OnSigwinch));
#endif

	TagMask tag_mask = global_tag_whitelist;
	tag_mask |= SongFormatToTagMask(options.list_format.c_str());
	tag_mask |= SongFormatToTagMask(options.search_format.c_str());
	tag_mask |= SongFormatToTagMask(options.status_format.c_str());
#ifndef NCMPC_MINI
	tag_mask |= SongFormatToTagMask(options.xterm_title_format.c_str());
#endif

	client.WhitelistTags(tag_mask);
}

Instance::~Instance()
{
	screen_manager.Exit();
	SignalMonitorFinish();
}

void
Instance::Run()
{
	screen_manager.Update(client, seek);

	event_loop.Run();
}

bool
Instance::OnRawKey(int key) noexcept
try {
	return screen_manager.OnModalDialogKey(key);
} catch (...) {
	screen_manager.status_bar.SetMessage(GetFullMessage(std::current_exception()));
	return true;
}

bool
Instance::CancelModalDialog() noexcept
{
	return screen_manager.CancelModalDialog();
}
