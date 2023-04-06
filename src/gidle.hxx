// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#pragma once

#include "event/SocketEvent.hxx"

#include <mpd/client.h>

class MpdIdleHandler {
public:
	virtual void OnIdle(unsigned events) noexcept = 0;
	virtual void OnIdleError(enum mpd_error error,
				 enum mpd_server_error server_error,
				 const char *message) noexcept = 0;
};

class MpdIdleSource final {
	struct mpd_connection *connection;
	struct mpd_async *async;
	struct mpd_parser *parser;

	SocketEvent event;

	MpdIdleHandler &handler;

	unsigned io_events = 0;

	unsigned idle_events;

public:
	MpdIdleSource(EventLoop &event_loop,
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

	void OnSocketReady(unsigned flags) noexcept;

	void UpdateSocket() noexcept;
};
