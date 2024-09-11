// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "ModalDialog.hxx"
#include "co/AwaitableHelper.hxx"
#include "ui/Point.hxx"
#include "History.hxx"

#include <functional>
#include <string>

class Completion;

/**
 * A #ModalDialog that asks the user to input text.
 *
 * This dialog is supposed to be awaited from a coroutine using
 * co_await.  It suspends the caller while waiting for user input.
 */
class TextInputDialog final : public ModalDialog {
	const std::string_view prompt;

	using ModifiedCallback = std::function<void(std::string_view value)>;
	ModifiedCallback modified_callback;

	/** the current value */
	std::string value;

	History *const history;
	Completion *const completion;

	History::iterator hlist, hcurrent;

	/** the origin coordinates in the window */
	mutable Point point;

	/** the screen width of the input field */
	mutable unsigned width;

	/** is the input masked, i.e. characters displayed as '*'? */
	const bool masked;

	bool ready = false;

	/** the byte position of the cursor */
	std::size_t cursor = 0;

	/** the byte position displayed at the origin (for horizontal
	    scrolling) */
	std::size_t start = 0;

	std::coroutine_handle<> continuation;

	using Awaitable = Co::AwaitableHelper<TextInputDialog, false>;
	friend Awaitable;

public:
	/**
	 * @param _prompt the human-readable prompt to be displayed
	 * (including question mark if desired); the pointed-by memory
	 * is owned by the caller and must remain valid during the
	 * lifetime of this dialog
	 *
	 * @param _value the initial value
	 *
	 * @param _masked do not display the text, show asterisks
	 * instead (for password entry)
	 */
	TextInputDialog(ModalDock &_dock,
			std::string_view _prompt,
			std::string &&_value={},
			History *_history=nullptr,
			Completion *_completion=nullptr,
			bool _masked=false) noexcept
		:ModalDialog(_dock), prompt(_prompt),
		 value(std::move(_value)),
		 history(_history), completion(_completion),
		 masked(_masked)
	{
		Show();

		if (history) {
			/* append the a new line to our history list */
			history->emplace_back();
			/* hlist points to the current item in the history list */
			hcurrent = hlist = std::prev(history->end());
		}
	}

	~TextInputDialog() noexcept {
		Hide();
	}

	void SetModifiedCallback(ModifiedCallback &&_modified_callback) noexcept {
		modified_callback = std::move(_modified_callback);
	}

	/**
	 * Await completion of this dialog.
	 *
	 * @return a std::string; if canceled by the user, returns an
	 * empty string
	 */
	Awaitable operator co_await() noexcept {
		return *this;
	}

private:
	bool IsReady() const noexcept {
		return ready;
	}

	std::string TakeValue() noexcept {
		return std::move(value);
	}

	void SetReady() noexcept;

	/** returns the screen column where the cursor is located */
	[[gnu::pure]]
	unsigned GetCursorColumn() const noexcept;

	/** move the cursor one step to the right */
	void MoveCursorRight() noexcept;

	/** move the cursor one step to the left */
	void MoveCursorLeft() noexcept;

	/** move the cursor to the end of the line */
	void MoveCursorToEnd() noexcept;

	void InsertByte(Window window, int key) noexcept;
	void DeleteChar(size_t x) noexcept;
	void DeleteChar() noexcept {
		DeleteChar(cursor);
	}

	void InvokeModifiedCallback() const noexcept {
		if (modified_callback != nullptr)
			modified_callback(value);
	}

public:
	/* virtual methodds from Modal */
	void OnLeave(Window window) noexcept override;
	void OnCancel() noexcept override;
	bool OnKey(Window window, int key) override;
	void Paint(Window window) const noexcept override;
};
