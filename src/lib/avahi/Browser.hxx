// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <avahi-client/lookup.h>

#include <memory>

namespace Avahi {

struct ServiceBrowserDeleter {
	void operator()(AvahiServiceBrowser *b) noexcept {
		avahi_service_browser_free(b);
	}
};

using ServiceBrowserPtr = std::unique_ptr<AvahiServiceBrowser,
					  ServiceBrowserDeleter>;

} // namespace Avahi
