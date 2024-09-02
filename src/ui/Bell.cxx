#include "Bell.hxx"
#include "Options.hxx"

#include <curses.h>

void
Bell() noexcept
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}
