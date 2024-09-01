// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct mpdclient;
class Completion;

#define GCMP_TYPE_DIR       (0x01 << 0)
#define GCMP_TYPE_FILE      (0x01 << 1)
#define GCMP_TYPE_PLAYLIST  (0x01 << 2)
#define GCMP_TYPE_RFILE     (GCMP_TYPE_DIR | GCMP_TYPE_FILE)
#define GCMP_TYPE_RPLAYLIST (GCMP_TYPE_DIR | GCMP_TYPE_PLAYLIST)

/**
 * Create a list suitable for #Completion from path.
 */
void
gcmp_list_from_path(struct mpdclient &c, const char *path,
		    Completion &completion,
		    int types);
