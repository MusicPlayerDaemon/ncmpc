// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef QUEUE_HXX
#define QUEUE_HXX

#include "SongPtr.hxx"

#include <mpd/client.h>

#include <vector>

#include <assert.h>

struct MpdQueue {
	/* queue version number (obtained from mpd_status) */
	unsigned version = 0;

	using Vector = std::vector<SongPtr>;

	/* the list */
	Vector items;

	using size_type = Vector::size_type;

	size_type size() const noexcept {
		return items.size();
	}

	bool empty() const noexcept {
		return items.empty();
	}

	/** remove and free all songs in the playlist */
	void clear() noexcept;

	const struct mpd_song &operator[](size_type i) const noexcept {
		assert(i < size());

		return *items[i];
	}

	struct mpd_song &operator[](size_type i) noexcept {
		assert(i < size());

		return *items[i];
	}

	[[gnu::pure]]
	const struct mpd_song *GetChecked(int i) const noexcept;

	void push_back(const struct mpd_song &song) noexcept {
		items.emplace_back(mpd_song_dup(&song));
	}

	void Replace(size_type i, const struct mpd_song &song) noexcept {
		items[i].reset(mpd_song_dup(&song));
	}

	void RemoveIndex(size_type i) noexcept {
		items.erase(std::next(items.begin(), i));
	}

	void Move(unsigned dest, unsigned src) noexcept;

	/**
	 * Find a song by its reference.  This method compares
	 * #mpd_song references, so this method makes only sense for
	 * songs references which were obtained from this container.
	 *
	 * @return the song position
	 */
	[[gnu::pure]]
	size_type FindByReference(const struct mpd_song &song) const noexcept;

	/**
	 * Find a song by its id.
	 *
	 * @return the song position
	 */
	[[gnu::pure]]
	int FindById(unsigned id) const noexcept;

	/**
	 * Find a song by its URI.
	 *
	 * @return the song position
	 */
	[[gnu::pure]]
	int FindByUri(const char *uri) const noexcept;

	/**
	 * Like FindByUri(), but return the song id, not the song position
	 *
	 * @return the song id
	 */
	[[gnu::pure]]
	int FindIdByUri(const char *uri) const noexcept {
		int i = FindByUri(uri);
		if (i >= 0)
			i = mpd_song_get_id(items[i].get());
		return i;
	}

	[[gnu::pure]]
	bool ContainsUri(const char *uri) const noexcept {
		return FindByUri(uri) >= 0;
	}
};

#endif
