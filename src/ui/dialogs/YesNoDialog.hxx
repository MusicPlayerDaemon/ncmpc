// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "ModalDialog.hxx"
#include "co/AwaitableHelper.hxx"

#include <cstdint>
#include <string_view>

enum class YesNoResult : int8_t {
	CANCEL = -1,
	NO = false,
	YES = true,
};

/**
 * A #ModalDialog that asks the user a yes/no question.
 *
 * This dialog is supposed to be awaited from a coroutine using
 * co_await.  It suspends the caller while waiting for user input.
 */
class YesNoDialog final : public ModalDialog {
	const std::string_view prompt;

	std::coroutine_handle<> continuation;

	bool ready = false;
	YesNoResult result;

	using Awaitable = Co::AwaitableHelper<YesNoDialog, false>;
	friend Awaitable;

public:
	/**
	 * @param _prompt the human-readable prompt to be displayed
	 * (including question mark if desired); the pointed-by memory
	 * is owned by the caller and must remain valid during the
	 * lifetime of this dialog
	 */
	YesNoDialog(ModalDock &_dock, std::string_view _prompt) noexcept
		:ModalDialog(_dock), prompt(_prompt)
	{
		Show();
	}

	~YesNoDialog() noexcept {
		Hide();
	}

	/**
	 * Await completion of this dialog.
	 *
	 * @return a YesNoResult
	 */
	Awaitable operator co_await() noexcept {
		return *this;
	}

private:
	void SetResult(YesNoResult _result) noexcept {
		result = _result;
		ready = true;

		if (continuation)
			continuation.resume();
	}

	bool IsReady() const noexcept {
		return ready;
	}

	YesNoResult TakeValue() noexcept {
		return result;
	}

public:
	/* virtual methodds from Modal */
	void OnLeave(Window window) noexcept override;
	void OnCancel() noexcept override;
	void Paint(Window window) const noexcept override;
	bool OnKey(Window window, int key) override;
};
