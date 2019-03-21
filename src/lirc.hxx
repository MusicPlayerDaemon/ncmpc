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

#ifndef LIRC_H
#define LIRC_H

#include "AsioServiceFwd.hxx"

#include <boost/asio/posix/stream_descriptor.hpp>

class LircInput {
	boost::asio::posix::stream_descriptor d;

#if BOOST_VERSION >= 107000
	boost::asio::io_context &io_context;
#endif

	struct lirc_config *lc = nullptr;

public:
	explicit LircInput(boost::asio::io_service &io_service);
	~LircInput();

	auto &get_io_context() noexcept {
#if BOOST_VERSION >= 107000
		return io_context;
#else
		return d.get_io_service();
#endif
	}

private:
	void AsyncWait() {
		d.async_read_some(boost::asio::null_buffers(),
				  std::bind(&LircInput::OnReadable, this,
					    std::placeholders::_1));
	}

	void OnReadable(const boost::system::error_code &error);
};

#endif
