// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include "ObjectFlags.hxx"

#include <string>

union InetAddress;
typedef struct AvahiStringList AvahiStringList;

namespace Avahi {

struct ObjectFlags;

class ServiceExplorerListener {
public:
	virtual void OnAvahiNewObject(const std::string &key,
				      const char *host_name,
				      const InetAddress &address,
				      AvahiStringList *txt,
				      ObjectFlags flags) noexcept = 0;
	virtual void OnAvahiRemoveObject(const std::string &key) noexcept = 0;
	virtual void OnAvahiAllForNow() noexcept {}
};

} // namespace Avahi
