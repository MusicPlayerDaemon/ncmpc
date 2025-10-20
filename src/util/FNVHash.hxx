// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

/*
 * Implementation of the Fowler-Noll-Vo hash function.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

template<typename T>
struct FNVTraits {};

template<>
struct FNVTraits<uint32_t> {
	typedef uint32_t value_type;
	typedef uint_fast32_t fast_type;

	static constexpr value_type OFFSET_BASIS = 2166136261u;
	static constexpr value_type PRIME = 16777619u;
};

template<>
struct FNVTraits<uint64_t> {
	typedef uint64_t value_type;
	typedef uint_fast64_t fast_type;

	static constexpr value_type OFFSET_BASIS = 14695981039346656037u;
	static constexpr value_type PRIME = 1099511628211u;
};

template<typename Traits>
struct FNV1aAlgorithm {
	typedef typename Traits::value_type value_type;
	typedef typename Traits::fast_type fast_type;

	[[gnu::hot]]
	static constexpr fast_type Update(fast_type hash, uint8_t b) noexcept {
		return (hash ^ b) * Traits::PRIME;
	}

	[[gnu::pure]] [[gnu::hot]]
	static value_type StringHash(const char *s) noexcept {
		using Algorithm = FNV1aAlgorithm<Traits>;

		fast_type hash = Traits::OFFSET_BASIS;
		while (*s)
			hash = Algorithm::Update(hash, *s++);

		return hash;
	}
};

[[gnu::pure]] [[gnu::hot]]
inline uint32_t
FNV1aHash32(const char *s) noexcept
{
	using Traits = FNVTraits<uint32_t>;
	using Algorithm = FNV1aAlgorithm<Traits>;
	return Algorithm::StringHash(s);
}

[[gnu::pure]] [[gnu::hot]]
inline uint64_t
FNV1aHash64(const char *s) noexcept
{
	using Traits = FNVTraits<uint64_t>;
	using Algorithm = FNV1aAlgorithm<Traits>;
	return Algorithm::StringHash(s);
}

[[gnu::pure]] [[gnu::hot]]
inline uint32_t
FNV1aHashFold32(const char *s) noexcept
{
	const uint64_t h64 = FNV1aHash64(s);

	/* XOR folding */
	const uint_fast32_t lo(h64);
	const uint_fast32_t hi(h64 >> 32);
	return lo ^ hi;
}
