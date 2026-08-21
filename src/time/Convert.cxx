// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "Convert.hxx"

#ifdef _WIN32
#include <winsock.h>  /* for struct timeval */
#else
#include <sys/time.h>  /* for struct timeval */
#endif

std::chrono::steady_clock::duration
ToSteadyClockDuration(const struct timeval &tv) noexcept
{
	return std::chrono::steady_clock::duration(std::chrono::seconds(tv.tv_sec)) +
		std::chrono::steady_clock::duration(std::chrono::microseconds(tv.tv_usec));
}
