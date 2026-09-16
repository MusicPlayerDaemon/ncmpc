// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "util/LocaleConvert.hxx"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <clocale>

using std::string_view_literals::operator""sv;

class LocaleConvertTest : public testing::Test {
	std::string saved_locale;

protected:
	void SetUp() override {
		saved_locale = std::setlocale(LC_CTYPE, nullptr);
		ASSERT_NE(std::setlocale(LC_CTYPE, "C"), nullptr);
	}

	void TearDown() override {
		EXPECT_NE(std::setlocale(LC_CTYPE, saved_locale.c_str()), nullptr);
	}
};

class Utf8LocaleConvertTest : public LocaleConvertTest {
protected:
	void SetUp() override {
		LocaleConvertTest::SetUp();
		if (HasFatalFailure())
			return;

		for (const char *name : {"C.UTF-8", "C.utf8", "en_US.UTF-8", ".UTF-8"})
			if (std::setlocale(LC_CTYPE, name) != nullptr)
				return;

		GTEST_SKIP() << "No UTF-8 locale available";
	}
};

/** Check both UTF-8-to-locale APIs, including the returned end pointer. */
static void
ExpectUtf8ToLocale(std::string_view src, std::string_view expected)
{
	EXPECT_EQ(ConvertUtf8ToLocaleString(src), expected);

	std::array<char, 128> buffer;
	buffer.fill('#');
	ASSERT_LT(expected.size(), buffer.size());

	const char *end = ConvertUtf8ToLocaleTruncate(buffer, src);
	ASSERT_EQ(end, buffer.data() + expected.size());
	EXPECT_EQ((std::string_view{buffer.data(), expected.size()}), expected);
	EXPECT_TRUE(std::all_of(end, buffer.cend(), [](char ch) {
		return ch == '#';
	}));
}

/** Truncation must preserve whole characters and stay inside the buffer. */
static void
ExpectTruncatedUtf8(std::string_view src)
{
	std::array<char, 32> buffer;
	buffer.fill('#');
	const auto dest = std::span{buffer}.subspan(1, 16);

	const char *end = ConvertUtf8ToLocaleTruncate(dest, src);
	ASSERT_GT(end, dest.data());
	ASSERT_LE(end, dest.data() + dest.size());

	const std::string_view result{dest.data(), end};
	ASSERT_LT(result.size(), src.size());
	EXPECT_EQ(result, src.substr(0, result.size()));
	// a continuation byte here would mean the last character was split.
	EXPECT_NE(static_cast<unsigned char>(src[result.size()]) & 0xc0, 0x80);
	EXPECT_EQ(buffer.front(), '#');
	EXPECT_TRUE(std::all_of(end, buffer.cend(), [](char ch) {
		return ch == '#';
	}));
}

TEST_F(LocaleConvertTest, Empty)
{
	ExpectUtf8ToLocale({}, {});
	EXPECT_EQ(ConvertLocaleToUtf8String({}), ""sv);
}

TEST_F(LocaleConvertTest, Ascii)
{
	constexpr auto text = "Hello, world!\t123\n"sv;
	ExpectUtf8ToLocale(text, text);
	EXPECT_EQ(ConvertLocaleToUtf8String(text), text);
}

TEST_F(LocaleConvertTest, EmbeddedNull)
{
	constexpr auto text = "a\0b\0"sv;
	ExpectUtf8ToLocale(text, text);
	EXPECT_EQ(ConvertLocaleToUtf8String(text), text);
}

TEST_F(LocaleConvertTest, UnrepresentableCharacter)
{
	ExpectUtf8ToLocale("caf\xc3\xa9!"sv, "caf?!"sv);
}

TEST_F(LocaleConvertTest, InvalidLocaleInput)
{
	EXPECT_EQ(ConvertLocaleToUtf8String("a\xff" "b"sv), "a?b"sv);
}

TEST_F(LocaleConvertTest, TruncateAscii)
{
	ExpectTruncatedUtf8("abcdefghijklmnopqrstuvwxyz0123456789"sv);
}

TEST_F(Utf8LocaleConvertTest, Utf8ToLocale)
{
	// two-, three-, and four-byte UTF-8 characters, including one at EOF.
	constexpr auto text = "caf\xc3\xa9 \xe6\xbc\xa2 \xf0\x9f\x98\x80"sv;
	ExpectUtf8ToLocale(text, text);
}

TEST_F(Utf8LocaleConvertTest, LocaleToUtf8)
{
	constexpr auto text = "caf\xc3\xa9 \xe6\xbc\xa2 \xf0\x9f\x98\x80!"sv;
	EXPECT_EQ(ConvertLocaleToUtf8String(text), text);
}

TEST_F(Utf8LocaleConvertTest, LocaleToUtf8TrailingMultibyte)
{
	/* mbrtoc8() can consume the final input character before
	   emitting all of its UTF-8 code units; those pending units
	   must still be drained */
	for (const auto text : {"caf\xc3\xa9"sv,
				"\xe6\xbc\xa2"sv,
				"\xf0\x9f\x98\x80"sv}) {
		SCOPED_TRACE(text);
		EXPECT_EQ(ConvertLocaleToUtf8String(text), text);
	}
}

TEST_F(Utf8LocaleConvertTest, InvalidInputRecovery)
{
	constexpr auto text = "a\xff" "b"sv;
	ExpectUtf8ToLocale(text, "a?b"sv);
	EXPECT_EQ(ConvertLocaleToUtf8String(text), "a?b"sv);
}

TEST_F(Utf8LocaleConvertTest, IncompleteLocaleInput)
{
	for (const auto text : {"a\xc3"sv, "a\xe6\xbc"sv, "a\xf0\x9f\x98"sv}) {
		SCOPED_TRACE(text);
		EXPECT_EQ(ConvertLocaleToUtf8String(text), "a?"sv);
	}
}

TEST_F(Utf8LocaleConvertTest, TruncateMultibyte)
{
	std::string text;
	for (unsigned i = 0; i < 10; ++i)
		text += "\xc3\xa9\xe6\xbc\xa2\xf0\x9f\x98\x80";

	ExpectTruncatedUtf8(text);
}
