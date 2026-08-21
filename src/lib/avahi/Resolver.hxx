// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <avahi-client/lookup.h>

#include <memory>

namespace Avahi {

struct ServiceResolverDeleter {
	void operator()(AvahiServiceResolver *r) noexcept {
		avahi_service_resolver_free(r);
	}
};

using ServiceResolverPtr = std::unique_ptr<AvahiServiceResolver,
					   ServiceResolverDeleter>;

} // namespace Avahi
