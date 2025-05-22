// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <sys/poll.h>

struct PollEvents {
	static constexpr unsigned READ = POLLIN;
	static constexpr unsigned WRITE = POLLOUT;
	static constexpr unsigned ERROR = POLLERR;
	static constexpr unsigned HANGUP = POLLHUP;

	/**
	 * A mask containing all events which indicate a dead socket
	 * connection (i.e. error or hangup).  It may still be
	 * possible to receive pending data from the socket buffer.
	 */
	static constexpr unsigned DEAD_MASK = ERROR|HANGUP;
};
