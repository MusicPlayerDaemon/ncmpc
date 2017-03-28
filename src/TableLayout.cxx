/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "TableLayout.hxx"
#include "TableStructure.hxx"

#include <math.h>

void
TableLayout::Calculate(unsigned screen_width) noexcept
{
	if (structure.columns.empty())
		/* shouldn't happen */
		return;

	std::fill(columns.begin(), columns.end(), TableColumnLayout{});

	if (screen_width <= structure.columns.front().min_width) {
		/* very narrow window, there's only space for one
		   column */
		columns.front().width = screen_width;
		return;
	}

	/* check how many columns fit on the screen */

	unsigned remaining_width = screen_width - structure.columns.front().min_width - 1;
	size_t n_visible = 1;
	float fraction_sum = structure.columns.front().fraction_width;

	for (size_t i = 1; i < structure.columns.size(); ++i) {
		auto &c = structure.columns[i];

		if (remaining_width < c.min_width)
			/* this column doesn't fit, stop here */
			break;

		++n_visible;
		fraction_sum += c.fraction_width;
		remaining_width -= c.min_width;

		if (remaining_width == 0)
			/* no room for the vertical line */
			break;

		/* subtract the vertical line */
		--remaining_width;
	}

	/* distribute the remaining width */

	for (size_t i = n_visible; i > 0;) {
		auto &c = structure.columns[--i];

		unsigned width = c.min_width;
		if (fraction_sum > 0 && c.fraction_width > 0) {
			unsigned add = lrint(remaining_width * c.fraction_width
					     / fraction_sum);
			if (add > remaining_width)
				add = remaining_width;

			width += add;
			remaining_width -= add;
			fraction_sum -= c.fraction_width;
		}

		columns[i].width = width;
	}
}
