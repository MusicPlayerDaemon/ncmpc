/* ncmpc (Ncurses MPD Client)
   (c) 2004-2020 The Music Player Daemon Project
   Project homepage: http://musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MPD_GLIB_SOURCE_H
#define MPD_GLIB_SOURCE_H

#include "AsioServiceFwd.hxx"

#include <mpd/client.h>

#include <boost/asio/posix/stream_descriptor.hpp>

class MpdIdleHandler {
public:
	virtual void OnIdle(unsigned events) noexcept = 0;
	virtual void OnIdleError(enum mpd_error error,
				 enum mpd_server_error server_error,
				 const char *message) noexcept = 0;
};

class MpdIdleSource {
	struct mpd_connection *connection;
	struct mpd_async *async;
	struct mpd_parser *parser;

	MpdIdleHandler &handler;

	boost::asio::posix::stream_descriptor socket;

	unsigned io_events = 0;

	unsigned idle_events;

public:
	MpdIdleSource(boost::asio::io_service &io_service,
		      struct mpd_connection &_connection,
		      MpdIdleHandler &_handler) noexcept;
	~MpdIdleSource() noexcept;

	/**
	 * Enters idle mode.
	 *
	 * @return true if idle mode has been entered, false if not
	 * (e.g. I/O error)
	 */
	bool Enter() noexcept;

	/**
	 * Leaves idle mode and invokes the callback if there were events.
	 */
	void Leave() noexcept;

private:
	void InvokeCallback() noexcept {
		if (idle_events != 0)
			handler.OnIdle(idle_events);
	}

	void InvokeError(enum mpd_error error,
			 enum mpd_server_error server_error,
			 const char *message) noexcept {
		handler.OnIdleError(error, server_error, message);
	}

	void InvokeAsyncError() noexcept;

	/**
	 * Parses a response line from MPD.
	 *
	 * @return true on success, false on error
	 */
	bool Feed(char *line) noexcept;

	/**
	 * Receives and evaluates a portion of the MPD response.
	 *
	 * @return true on success, false on error
	 */
	bool Receive() noexcept;

	void OnReadable(const boost::system::error_code &error) noexcept;
	void OnWritable(const boost::system::error_code &error) noexcept;

	void AsyncRead() noexcept;
	void AsyncWrite() noexcept;
	void UpdateSocket() noexcept;
};

#endif
