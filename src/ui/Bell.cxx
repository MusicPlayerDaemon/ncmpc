#include "Bell.hxx"
#include "Options.hxx"

#include <curses.h>

void
Bell() noexcept
{
	if (ui_options.audible_bell)
		beep();
	if (ui_options.visible_bell)
		flash();
}
