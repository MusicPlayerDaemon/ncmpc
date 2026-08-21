// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

namespace Avahi {

/**
 * Flags of an object found by #ServiceExplorer, passed to
 * #ServiceExplorerListener.
 */
struct ObjectFlags {
	/**
	 * This record/service resides on and was announced by
	 * the local host.
	 */
	bool is_local;

	/**
	 * This service belongs to the same local client as
	 * the browser object.
	 */
	bool is_our_own;
};

} // namespace Avahi
