// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct Window;
struct Size;
class ModalDock;

/**
 * Abstract base class for modal dialogs.  It is a dialog with the
 * user that receives all (keyboard) input until it is hidden (e.g. by
 * finishing it) or canceled.
 *
 * Instances of this class are owned by the code constructing it.
 * Usually, instances are constructed on the stack of a coroutine
 * which then awaits it.
 */
class ModalDialog {
	ModalDock &dock;

	bool visible = false;

protected:
	explicit ModalDialog(ModalDock &_dock) noexcept
		:dock(_dock) {}

	~ModalDialog() noexcept;

	/**
	 * Wrapper for ModalDock::Cancel() to be called from this
         * object.
	 */
	void Cancel() noexcept;

	/**
	 * Wrapper for ModalDock::ShowModalDialog() to be called from
         * this object.
	 *
	 * Call this to show and activate the dialog after
         * constructing it.
	 */
	void Show() noexcept;

	/**
	 * Wrapper for ModalDock::HideModalDialog() to be called from
         * this object.
	 *
	 * Call this when the dialog finishes (e.g. right before
         * resuming the calling coroutine).
	 */
	void Hide() noexcept;

public:
	/**
	 * For internal use only (to be called by the #ModalDock
         * implementation).
	 */
	void InvokeCancel() noexcept;

	/**
	 * Called when the user leaves the dialog.  This can be used
	 * to restore window settings (e.g. background color and
	 * cursor visibility).
	 */
	virtual void OnLeave(Window window) noexcept;

	/**
	 * The dialog is being canceled, either by the user or
	 * programmatically.
	 */
	virtual void OnCancel() noexcept = 0;

	/**
	 * The window was resized.  The implementation may override
         * this to update its layout.  Usually, Paint() is called
         * right after this method.
	 */
	virtual void OnResize(Window window, Size size) noexcept;

	/**
	 * Paint the dialog to the specified #Window.
	 *
	 * All state changes such as background color and cursor
         * visibility must be undone by OnLeave().
	 */
	virtual void Paint(Window window) const noexcept = 0;

	/**
	 * The user has pressed a key.
	 *
	 * @return true if the key press was handled
	 */
	virtual bool OnKey(Window window, int key) = 0;
};
