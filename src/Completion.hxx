/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef COMPLETION_HXX
#define COMPLETION_HXX

#include <set>
#include <string>

class Completion {
protected:
	using List = std::set<std::string>;
	List list;

public:
	Completion() = default;

	Completion(const Completion &) = delete;
	Completion &operator=(const Completion &) = delete;

	bool empty() const {
		return list.empty();
	}

	void clear() {
		list.clear();
	}

	template<typename T>
	void emplace(T &&value) {
		list.emplace(std::forward<T>(value));
	}

	template<typename T>
	void remove(T &&value) {
		auto i = list.find(std::forward<T>(value));
		if (i != list.end())
			list.erase(i);
	}

	struct Range {
		using const_iterator = List::const_iterator;
		const_iterator _begin, _end;

		bool operator==(const Range other) const {
			return _begin == other._begin && _end == other._end;
		}

		bool operator!=(const Range other) const {
			return !(*this == other);
		}

		const_iterator begin() const {
			return _begin;
		}

		const_iterator end() const {
			return _end;
		}
	};

	struct Result {
		std::string new_prefix;

		Range range;
	};

	Result Complete(const std::string &prefix) const;

	virtual void Pre(const char *value) = 0;
	virtual void Post(const char *value, Range range) = 0;
};

#endif
