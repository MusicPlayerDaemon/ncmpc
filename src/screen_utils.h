#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

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
int screen_find(screen_t *screen,
		list_window_t *lw,
		int rows,
		command_t findcmd,
		list_window_callback_fn_t callback_fn,
		void *callback_data);

gint screen_auth(mpdclient_t *c);

void screen_display_completion_list(screen_t *screen, GList *list);

void set_xterm_title(const char *format, ...);

#endif
