// SPDX-License-Identifier: GPL-2.0-or-lateryes
// Copyright The Music Player Daemon Project

#pragma once

#include "Interface.hxx"

class Page;

/**
 * An abstract interface for classes which host one (or more) #Page
 * instances.
 */
class PageContainer : public Interface {
public:
	virtual void SchedulePaint(Page &page) noexcept = 0;
};
