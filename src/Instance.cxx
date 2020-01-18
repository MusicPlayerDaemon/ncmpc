/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "Instance.hxx"
#include "Options.hxx"

#include <signal.h>

#ifdef HAVE_TAG_WHITELIST

#include "strfsong.hxx"
#include "TagMask.hxx"

static constexpr TagMask global_tag_whitelist{
	/* these tags are used by SongPage.cxx */
	MPD_TAG_ARTIST,
	MPD_TAG_TITLE,
	MPD_TAG_ALBUM,
	MPD_TAG_COMPOSER,
	MPD_TAG_NAME,
	MPD_TAG_DISC,
	MPD_TAG_TRACK,
	MPD_TAG_DATE,
	MPD_TAG_GENRE,
	MPD_TAG_COMMENT,
};

#endif

Instance::Instance()
	:io_service(),
#ifndef _WIN32
	 sigterm(io_service, SIGTERM, SIGINT, SIGHUP),
	 sigwinch(io_service, SIGWINCH, SIGCONT),
#endif
	 client(io_service,
		options.host.empty() ? nullptr : options.host.c_str(),
		options.port,
		options.timeout_ms,
		options.password.empty() ? nullptr : options.password.c_str()),
	 seek(io_service, client),
	 reconnect_timer(io_service),
	 update_timer(io_service),
#ifndef NCMPC_MINI
	 check_key_bindings_timer(io_service),
#endif
	 screen_manager(io_service),
#ifdef ENABLE_LIRC
	 lirc_input(io_service),
#endif
	 user_input(io_service, *screen_manager.main_window.w)
{
	screen_manager.Init(&client);

	sigterm.async_wait([this](const auto &, int){
			this->io_service.stop();
		});

#ifndef _WIN32
	AsyncWaitSigwinch();
#endif

#ifdef HAVE_TAG_WHITELIST
	TagMask tag_mask = global_tag_whitelist;
	tag_mask |= SongFormatToTagMask(options.list_format.c_str());
	tag_mask |= SongFormatToTagMask(options.search_format.c_str());
	tag_mask |= SongFormatToTagMask(options.status_format.c_str());
#ifndef NCMPC_MINI
	tag_mask |= SongFormatToTagMask(options.xterm_title_format.c_str());
#endif

	client.WhitelistTags(tag_mask);
#endif
}

Instance::~Instance()
{
	screen_manager.Exit();
}

void
Instance::Run()
{
	screen_manager.Update(client, seek);

	io_service.run();
}
