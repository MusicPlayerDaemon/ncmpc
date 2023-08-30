// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

/*
 * Implementation of the Fowler-Noll-Vo hash function.
 */

#pragma once

#include <cstdint>

template<typename T>
struct FNVTraits {};

template<>
struct FNVTraits<uint32_t> {
	using value_type = uint32_t;
	using fast_type = uint_fast32_t;

	static constexpr value_type OFFSET_BASIS = 2166136261u;
	static constexpr value_type PRIME = 16777619u;
};

template<>
struct FNVTraits<uint64_t> {
	using value_type = uint64_t;
	using fast_type = uint_fast64_t;

	static constexpr value_type OFFSET_BASIS = 14695981039346656037u;
	static constexpr value_type PRIME = 1099511628211u;
};

template<typename Traits>
struct FNV1aAlgorithm {
	using value_type = typename Traits::value_type;
	using fast_type = typename Traits::fast_type;

	[[gnu::hot]]
	static constexpr fast_type Update(fast_type hash, fast_type b) noexcept {
		return (hash ^ b) * Traits::PRIME;
	}

	[[gnu::pure]] [[gnu::hot]]
	static value_type StringHash(const char *s,
				     fast_type init=Traits::OFFSET_BASIS) noexcept {
		using Algorithm = FNV1aAlgorithm<Traits>;

		fast_type hash = init;
		while (*s)
			/* cast to uint8_t first to avoid problems
			   with signed char */
			hash = Algorithm::Update(hash,
						 static_cast<uint8_t>(*s++));

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
