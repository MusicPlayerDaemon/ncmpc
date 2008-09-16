#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

#include "list_window.h"
#include "command.h"

#include <ncurses.h>

struct screen;
struct mpdclient;

/* sound an audible and/or visible bell */
void screen_bell(void);

/* read a characher from the status window */
int screen_getch(WINDOW *w, const char *prompt);

/* read a string from the status window */
char *screen_getstr(WINDOW *w, const char *prompt);
char *screen_readln(WINDOW *w, const char *prompt, const char *value,
		    GList **history, GCompletion *gcmp);
char *screen_readln_masked(WINDOW *w, const char *prompt);
char *screen_read_pasword(WINDOW *w, const char *prompt);
/* query user for a string and find it in a list window */
int screen_find(struct screen *screen,
		struct list_window *lw,
		int rows,
		command_t findcmd,
		list_window_callback_fn_t callback_fn,
		void *callback_data);

gint screen_auth(struct mpdclient *c);

void screen_display_completion_list(struct screen *screen, GList *list);

void set_xterm_title(const char *format, ...);

#endif
