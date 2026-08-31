// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "FormRow.hxx"

#include <string>

namespace Co { template<typename T> class Task; }
class ModalDock;

class StringEditRow : public FormRow {
	std::string value;

public:
	template<typename V>
	StringEditRow(std::string_view _label_locale, V &&_value)
		:FormRow(_label_locale),
		 value(std::forward<V>(_value)) {}

	const std::string &GetValue() const noexcept {
		return value;
	}

	void Clear() noexcept {
		value.clear();
	}

	void Paint(const Window window, unsigned y, unsigned width,
		   bool selected) const noexcept;

	Co::Task<bool> Edit(ModalDock &modal_dock) noexcept;
};
