// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "LocaleString.hxx"

#include <cwchar>

#include <string.h>

bool
IsIncompleteCharMB(std::string_view s) noexcept
{
	auto mb = std::mbstate_t();
	const std::size_t length = std::mbrlen(s.data(), s.size(), &mb);
	return length == std::size_t(-2);
}

std::size_t
StringLengthMB(std::string_view _s) noexcept
{
	const char *s = _s.data();
	const char *const end = s + _s.size();
	auto state = std::mbstate_t();

	size_t length = 0;
	while (s < end) {
		wchar_t w;
		std::size_t n = std::mbrtowc(&w, s, end - s, &state);
		if (n == std::size_t(-2))
			break;

		if (n == std::size_t(-1) || n == 0) {
			++s;
		} else {
			s += n;
			++length;
		}
	}

	return length;

}

std::size_t
CharSizeMB(std::string_view s) noexcept
{
	auto mb = std::mbstate_t();
	const std::size_t length = std::mbrlen(s.data(), s.size(), &mb);
	if (length == std::size_t(-2))
		return s.size();

	if (length == std::size_t(-1))
		return 1;

	return length;
}

const char *
PrevCharMB(const char *start, const char *reference) noexcept
{
	const char *p = reference;

	while (p > start) {
		--p;

		auto mb = std::mbstate_t();
		const std::size_t length = std::mbrlen(p, reference - p, &mb);
		if (length != std::size_t(-1))
			break;
	}

	return p;
}

const char *
AtCharMB(std::string_view _s, size_t i) noexcept
{
	const char *s = _s.data();
	const char *const end = s + _s.size();
	auto state = std::mbstate_t();

	while (i > 0) {
		wchar_t w;
		std::size_t n = std::mbrtowc(&w, s, end - s, &state);

		if (n == std::size_t(-2)) {
			s += strlen(s);
			break;
		}

		--i;

		if (n == std::size_t(-1) || n == 0)
			++s;
		else
			s += n;
	}

	return s;
}

size_t
StringWidthMB(std::string_view _s) noexcept
{
	const char *s = _s.data();
	const char *const end = s + _s.size();
	auto state = std::mbstate_t();

	size_t width = 0;
	while (s < end) {
		wchar_t w;
		std::size_t n = std::mbrtowc(&w, s, end - s, &state);
		if (n == std::size_t(-2))
			break;

		if (n == std::size_t(-1) || n == 0) {
			++s;
		} else {
			s += n;
			int cw = wcwidth(w);
			if (cw > 0)
				width += cw;
		}
	}

	return width;
}

const char *
AtWidthMB(std::string_view _s, size_t width) noexcept
{
	const char *s = _s.data();
	const char *const end = s + _s.size();
	auto state = std::mbstate_t();

	while (width > 0 && s < end) {
		wchar_t w;
		std::size_t n = std::mbrtowc(&w, s, end - s, &state);
		if (n == std::size_t(-2))
			break;

		if (n == std::size_t(-1) || n == 0) {
			--width;
			++s;
		} else {
			int cw = wcwidth(w);
			if (cw > 0) {
				if (size_t(cw) > width)
					break;
				width -= cw;
			}

			s += n;
		}
	}

	return s;
}
