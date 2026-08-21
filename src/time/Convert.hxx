// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <chrono>

[[gnu::pure]]
std::chrono::steady_clock::duration
ToSteadyClockDuration(const struct timeval &tv) noexcept;
