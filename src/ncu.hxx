// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

/*
 * Basic libnucrses initialization.
 */

#ifndef NCU_H
#define NCU_H

void
ncu_init();

void
ncu_deinit();

class ScopeCursesInit {
public:
	ScopeCursesInit() {
		ncu_init();
	}

	~ScopeCursesInit() {
		ncu_deinit();
	}
};

#endif
