// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "UriUtil.hxx"

#include <string.h>

const char *
GetUriFilename(const char *uri) noexcept
{
	const char *slash = strrchr(uri, '/');
	return slash != nullptr ? slash + 1 : uri;
}

std::string_view
GetParentUri(std::string_view uri) noexcept
{
	const auto slash = uri.rfind('/');
	if (slash == uri.npos)
		return {};

	return uri.substr(0, slash);
}
