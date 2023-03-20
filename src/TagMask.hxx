// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef TAG_MASK_HXX
#define TAG_MASK_HXX

#include <mpd/tag.h>

#include <initializer_list>

#include <stdint.h>

class TagMask {
	typedef uint_least32_t mask_t;
	mask_t value;

	explicit constexpr TagMask(uint_least32_t _value) noexcept
		:value(_value) {}

public:
	TagMask() = default;

	constexpr TagMask(enum mpd_tag_type tag) noexcept
		:value(mask_t(1) << mask_t(tag)) {}

	constexpr TagMask(std::initializer_list<enum mpd_tag_type> il) noexcept
		:value(0)
	{
		for (auto i : il)
			*this |= TagMask(i);
	}

	static constexpr TagMask None() noexcept {
		return TagMask(mask_t(0));
	}

	static constexpr TagMask All() noexcept {
		return ~None();
	}

	constexpr TagMask operator~() const noexcept {
		return TagMask(~value);
	}

	constexpr TagMask operator&(TagMask other) const noexcept {
		return TagMask(value & other.value);
	}

	constexpr TagMask operator|(TagMask other) const noexcept {
		return TagMask(value | other.value);
	}

	constexpr TagMask operator^(TagMask other) const noexcept {
		return TagMask(value ^ other.value);
	}

	constexpr TagMask &operator&=(TagMask other) noexcept {
		value &= other.value;
		return *this;
	}

	constexpr TagMask &operator|=(TagMask other) noexcept {
		value |= other.value;
		return *this;
	}

	constexpr TagMask &operator^=(TagMask other) noexcept {
		value ^= other.value;
		return *this;
	}

	constexpr bool TestAny() const noexcept {
		return value != 0;
	}

	constexpr bool Test(enum mpd_tag_type tag) const noexcept {
		return (*this & tag).TestAny();
	}

	void Set(enum mpd_tag_type tag) noexcept {
		*this |= tag;
	}

	void Unset(enum mpd_tag_type tag) noexcept {
		*this |= ~TagMask(tag);
	}
};

#endif
