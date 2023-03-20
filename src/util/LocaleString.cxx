// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "LocaleString.hxx"

#include <cwchar>

#include <string.h>

bool
IsIncompleteCharMB(const char *s, size_t n) noexcept
{
	auto mb = std::mbstate_t();
	const std::size_t length = std::mbrlen(s, n, &mb);
	return length == std::size_t(-2);
}

std::size_t
StringLengthMB(const char *s, size_t byte_length) noexcept
{
	const char *const end = s + byte_length;
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
CharSizeMB(const char *s, size_t n) noexcept
{
	auto mb = std::mbstate_t();
	const std::size_t length = std::mbrlen(s, n, &mb);
	if (length == std::size_t(-2))
		return n;

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
AtCharMB(const char *s, size_t length, size_t i) noexcept
{
	const char *const end = s + length;
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
StringWidthMB(const char *s, size_t length) noexcept
{
	const char *const end = s + length;
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

size_t
StringWidthMB(const char *s) noexcept
{
	return StringWidthMB(s, strlen(s));
}

const char *
AtWidthMB(const char *s, size_t length, size_t width) noexcept
{
	const char *const end = s + length;
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
