// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "LocaleWord.hxx"
#include "LocaleExtract.hxx"
#include "LocaleString.hxx"

#include <cassert>
#include <concepts> // for std::regular_invocable
#include <cwctype> // for std::iswalpha(), std::iswdigit()

[[gnu::const]]
static bool
IsWordChar(wchar_t ch) noexcept
{
	return std::iswalnum(ch);
}

[[gnu::const]]
static bool
IsNonWordChar(wchar_t ch) noexcept
{
	return !IsWordChar(ch);
}

[[gnu::pure]]
static const char *
SkipLeftWhile(std::string_view s,
	      std::regular_invocable<wchar_t> auto f) noexcept
{
	while (!s.empty()) {
		static constexpr wchar_t fallback{};
		const auto [p, ch] = ExtractLastCharMB(s, fallback);
		assert(p >= s.data());
		assert(p < s.data() + s.size());

		if (!f(ch))
			break;

		s = {s.begin(), p};
	}

	return s.data() + s.size();
}

[[gnu::pure]]
static const char *
SkipRightWhile(std::string_view s,
	       std::regular_invocable<wchar_t> auto f) noexcept
{
	while (!s.empty()) {
		static constexpr wchar_t fallback{};
		const auto [p, ch] = ExtractFirstCharMB(s, fallback);
		assert(p > s.data());
		assert(p <= s.data() + s.size());

		if (!f(ch))
			break;

		s = {p, s.end()};
	}

	return s.data();
}

const char *
NextWordMB(std::string_view s) noexcept
{
	static constexpr wchar_t fallback{};

	auto [p, ch] = ExtractFirstCharMB(s, fallback);
	if (ch == fallback)
		return p;

	if (!IsWordChar(ch))
		p = SkipRightWhile({p, s.end()}, IsNonWordChar);

	return SkipRightWhile({p, s.end()}, IsWordChar);
}

const char *
LastWordMB(std::string_view s) noexcept
{
	static constexpr wchar_t fallback{};

	auto [p, ch] = ExtractLastCharMB(s, fallback);
	if (ch == fallback)
		return p;

	if (!IsWordChar(ch))
		p = SkipLeftWhile({s.begin(), p}, IsNonWordChar);

	return SkipLeftWhile({s.begin(), p}, IsWordChar);
}
