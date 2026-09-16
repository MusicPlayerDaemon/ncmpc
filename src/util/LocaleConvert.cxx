// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "LocaleConvert.hxx"

#include <cassert>
#include <cuchar>

/**
 * Replacement definition for MB_CUR_MAX which is not a constant
 * expression; 8 is generous, because 4 is the practical limit
 */
static constexpr std::size_t MAX_MB_SIZE = 8;

char *
ConvertUtf8ToLocaleTruncate(std::span<char> dest, const std::string_view src) noexcept
{
	assert(dest.size() > MAX_MB_SIZE);

	mbstate_t state{};
	char *p = dest.data();
	const char *const end = p + dest.size() - MAX_MB_SIZE;

	for (const auto byte : src) {
		if (p > end)
			break;

		std::size_t result = std::c8rtomb(p, byte, &state);
		if (result == static_cast<std::size_t>(-1)) {
			*p++ = '?';
			state = {};
		} else if (result > 0) {
			p += result;
		}
	}

	return p;
}

std::string
ConvertUtf8ToLocaleString(const std::string_view src) noexcept
{
	std::string result;
	result.reserve(src.size());

	mbstate_t state{};
	for (const auto byte : src) {
		char buffer[MAX_MB_SIZE];

		std::size_t n = std::c8rtomb(buffer, byte, &state);
		if (n == static_cast<std::size_t>(-1)) {
			result.push_back('?');
			state = {};
		} else if (n > 0) {
			result.append(buffer, n);
		}
	}

	return result;
}

std::string
ConvertLocaleToUtf8String(std::string_view src) noexcept
{
	std::string result;
	result.reserve(src.size());

	mbstate_t state{};

	while (true) {
		char8_t c8;
		std::size_t n = std::mbrtoc8(&c8, src.data(), src.size(), &state);

		if (src.empty() && n != static_cast<std::size_t>(-3))
			break;

		if (n == static_cast<std::size_t>(-1)) {
			// error
			result.push_back('?');
			src = src.substr(1);
			state = {};
		} else if (n == static_cast<std::size_t>(-2)) {
			// incomplete sequence at end
			result.push_back('?');
			break;
		} else if (n == static_cast<std::size_t>(-3)) {
			// no input consumed yet
			result.push_back(static_cast<char>(c8));
		} else if (n == 0) {
			// null character
			result.push_back('\0');
			src = src.substr(1);
		} else {
			// n bytes consumed from input, output UTF-8 code unit
			result.push_back(static_cast<char>(c8));
			src = src.substr(n);
		}
	}

	return result;
}
