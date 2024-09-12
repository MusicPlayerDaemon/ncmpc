// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "TitleBar.hxx"
#include "ProgressBar.hxx"
#include "StatusBar.hxx"
#include "page/Container.hxx"
#include "page/FindSupport.hxx"
#include "dialogs/ModalDock.hxx"
#include "ui/Point.hxx"
#include "ui/Window.hxx"
#include "event/IdleEvent.hxx"
#include "co/InvokeTask.hxx"

#include <curses.h>

#include <map>
#include <memory>
#include <utility>

enum class Command : unsigned;
struct mpd_song;
struct mpdclient;
struct PageMeta;
class Page;
class ModalDialog;
class DelayedSeek;
class EventLoop;

class ScreenManager final : public ModalDock, public PageContainer {
	/**
	 * This event defers Paint() calls until after the EventLoop
	 * has handled all other events.
	 */
	IdleEvent paint_event;

	struct Layout;

	TitleBar title_bar;
public:
	UniqueWindow main_window;

private:
	ProgressBar progress_bar;

public:
	StatusBar status_bar;

private:
	using PageMap = std::map<const PageMeta *,
				 std::unique_ptr<Page>>;
	PageMap pages;
	PageMap::iterator current_page = pages.begin();

	const PageMeta *mode_fn_prev;

	/**
	 * The task for QueryPassword().
	 */
	Co::InvokeTask query_password_task;

	ModalDialog *modal = nullptr;

	char *buf;
	size_t buf_size;

	/**
	 * True if #query_password_task is currently running.
	 * Meanwhile, the task must not be canceled.
	 */
	bool query_password_busy = false;

	/**
	 * Does the main area (the current page) need to be repainted?
	 */
	bool main_dirty = true;

public:
	FindSupport find_support{*this};

	explicit ScreenManager(EventLoop &_event_loop, const Layout &layout) noexcept;
	explicit ScreenManager(EventLoop &_event_loop) noexcept;
	~ScreenManager() noexcept;

	auto &GetEventLoop() const noexcept {
		return paint_event.GetEventLoop();
	}

	void Init(struct mpdclient *c) noexcept;
	void Exit() noexcept;

	const PageMeta &GetCurrentPageMeta() const noexcept {
		return *current_page->first;
	}

	PageMap::iterator MakePage(const PageMeta &sf) noexcept;

	void OnResize() noexcept;

	[[gnu::pure]]
	bool IsVisible(const PageMeta &page) const noexcept {
		return &page == current_page->first;
	}

	[[gnu::pure]]
	bool IsVisible(const Page &page) const noexcept {
		return &page == current_page->second.get();
	}

	void Switch(const PageMeta &sf, struct mpdclient &c) noexcept;
	void Swap(struct mpdclient &c, const struct mpd_song *song) noexcept;

	bool CancelModalDialog() noexcept;

	bool OnModalDialogKey(int key);

	void PaintTopWindow() noexcept;

	void Update(struct mpdclient &c, const DelayedSeek &seek) noexcept;
	void OnCommand(struct mpdclient &c, DelayedSeek &seek, Command cmd);

#ifdef HAVE_GETMOUSE
	void OnTitleBarMouse(struct mpdclient &c, int x, mmask_t bstate);
	void OnProgressBarMouse(struct mpdclient &c, DelayedSeek &seek,
				int x, mmask_t bstate);
	void OnMouse(struct mpdclient &c, DelayedSeek &seek,
		     Point p, mmask_t bstate);
#endif

	void SchedulePaint() noexcept {
		paint_event.Schedule();
	}

	/**
	 * Ask the user to enter a password for the MPD connection.
	 */
	void QueryPassword(struct mpdclient &c) noexcept;

private:
	Co::InvokeTask _QueryPassword(struct mpdclient &c);
	void OnCoComplete(std::exception_ptr error) noexcept;

	Page &GetCurrentPage() noexcept {
		return *current_page->second;
	}

	void NextMode(struct mpdclient &c, int offset) noexcept;

	void Paint() noexcept;

public:
	// virtual methods from ModalDock
	void ShowModalDialog(ModalDialog &m) noexcept override;
	void HideModalDialog(ModalDialog &m) noexcept override;
	void CancelModalDialog(ModalDialog &m) noexcept override;

	// virtual methods from PageContainer
	void SchedulePaint(Page &page) noexcept override;
	void Alert(std::string message) noexcept override;
};
