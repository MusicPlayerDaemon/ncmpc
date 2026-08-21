// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <avahi-common/strlst.h>

#include <string_view>

namespace Avahi {

/**
 * Cast an #AvahiStringList item to std::string_view.
 */
static inline std::string_view
ToStringView(const AvahiStringList &src) noexcept
{
	return {reinterpret_cast<const char *>(src.text), src.size};
}

} // namespace Avahi
