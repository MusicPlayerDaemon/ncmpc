// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

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

	bool empty() const noexcept {
		return list.empty();
	}

	void clear() noexcept {
		list.clear();
	}

	template<typename T>
	void emplace(T &&value) noexcept {
		list.emplace(std::forward<T>(value));
	}

	template<typename T>
	void remove(T &&value) noexcept {
		auto i = list.find(std::forward<T>(value));
		if (i != list.end())
			list.erase(i);
	}

	struct Range {
		using const_iterator = List::const_iterator;
		const_iterator _begin, _end;

		bool operator==(const Range &) const noexcept = default;

		const_iterator begin() const noexcept {
			return _begin;
		}

		const_iterator end() const noexcept {
			return _end;
		}
	};

	struct Result {
		std::string new_prefix;

		Range range;
	};

	Result Complete(const std::string &prefix) const noexcept;

	virtual void Pre(const char *value) noexcept = 0;
	virtual void Post(const char *value, Range range) noexcept = 0;
};
