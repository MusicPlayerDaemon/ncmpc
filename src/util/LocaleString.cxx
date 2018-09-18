/*
 * Copyright 2018 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LocaleString.hxx"

#include <cwchar>

#include <string.h>

bool
IsIncompleteCharMB(const char *s, size_t n)
{
	auto mb = std::mbstate_t();
	const std::size_t length = std::mbrlen(s, n, &mb);
	return length == std::size_t(-2);
}

std::size_t
StringLengthMB(const char *s, size_t byte_length)
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
CharSizeMB(const char *s, size_t n)
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
PrevCharMB(const char *start, const char *reference)
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
AtCharMB(const char *s, size_t length, size_t i)
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
StringWidthMB(const char *s, size_t length)
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
StringWidthMB(const char *s)
{
	return StringWidthMB(s, strlen(s));
}

const char *
AtWidthMB(const char *s, size_t length, size_t width)
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
