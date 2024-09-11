// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "LocaleExtract.hxx"
#include "LocaleString.hxx"

#include <cassert>

std::pair<const char *, wchar_t>
ExtractFirstCharMB(std::string_view s, wchar_t fallback) noexcept
{
	if (s.empty())
		return {s.data(), fallback};

	wchar_t w;
	std::mbstate_t state{};
	std::size_t n = std::mbrtowc(&w, s.data(), s.size(), &state);
	if (n == 0)
		return {s.data() + 1, 0};

	if (n == static_cast<std::size_t>(-1))
		return {s.data() + 1, fallback};

	if (n == static_cast<std::size_t>(-2))
		return {s.data() + s.size(), fallback};

	return {s.data() + n, w};
}

std::pair<const char *, wchar_t>
ExtractLastCharMB(std::string_view s, wchar_t fallback) noexcept
{
	if (s.empty())
		return {s.data(), fallback};

	const char *const end = s.data() + s.size();

	const char *p = LastCharMB(s);
	assert(p >= s.data());
	assert(p < end);

	wchar_t w;
	std::mbstate_t state{};
	std::size_t n = std::mbrtowc(&w, p, end - p, &state);
	if (n == 0)
		return {p, 0};

	if (n == static_cast<std::size_t>(-1) ||
	    n == static_cast<std::size_t>(-2))
		return {p, fallback};

	return {p, w};
}
