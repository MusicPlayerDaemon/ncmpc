// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef IO_PATH_HXX
#define IO_PATH_HXX

#include <string>

#include <string.h>

namespace PathDetail {

#ifdef _WIN32
static constexpr char SEPARATOR = '\\';
#else
static constexpr char SEPARATOR = '/';
#endif

[[gnu::pure]]
inline size_t
GetLength(const char *s) noexcept
{
	return strlen(s);
}

[[gnu::pure]]
inline size_t
GetLength(const std::string &s) noexcept
{
	return s.length();
}

template<typename... Args>
inline size_t
FillLengths(size_t *lengths, Args&&... args) noexcept;

template<typename First, typename... Args>
inline size_t
FillLengths(size_t *lengths, First &&first, Args&&... args) noexcept
{
	size_t length = GetLength(std::forward<First>(first));
	*lengths++ = length;
	return length + FillLengths(lengths, std::forward<Args>(args)...);
}

template<>
inline size_t
FillLengths(size_t *) noexcept
{
	return 0;
}

inline std::string &
Append(std::string &dest, const std::string &value, size_t length) noexcept
{
	return dest.append(value, 0, length);
}

inline std::string &
Append(std::string &dest, const char *value, size_t length) noexcept
{
	return dest.append(value, length);
}

template<typename... Args>
inline std::string &
AppendWithSeparators(std::string &dest, const size_t *lengths,
		     Args&&... args) noexcept;

template<typename First, typename... Args>
inline std::string &
AppendWithSeparators(std::string &dest, const size_t *lengths,
		     First &&first, Args&&... args) noexcept
{
	dest.push_back(SEPARATOR);
	return AppendWithSeparators(Append(dest, std::forward<First>(first),
					   *lengths),
				    lengths + 1,
				    std::forward<Args>(args)...);
}

template<>
inline std::string &
AppendWithSeparators(std::string &dest, const size_t *) noexcept
{
	return dest;
}

} // namespace PathDetail

template<typename First, typename... Args>
std::string
BuildPath(First &&first, Args&&... args) noexcept
{
	constexpr size_t n = sizeof...(args);

	size_t lengths[n + 1];
	const size_t total = PathDetail::FillLengths(lengths, first, args...);

	std::string result;
	result.reserve(total + n);
	PathDetail::Append(result, std::forward<First>(first), lengths[0]);
	PathDetail::AppendWithSeparators(result, lengths + 1,
					 std::forward<Args>(args)...);
	return result;
}

#endif
