
/* read a string from the status window */
char *screen_readln(WINDOW *w, char *prompt);

/* query user for a string and find it in a list window */
int screen_find(screen_t *screen,
		mpd_client_t *c,
		list_window_t *lw, 
		int rows,
		command_t findcmd,
		list_window_callback_fn_t callback_fn);


int my_waddstr(WINDOW *, const char *, int);
int my_mvwaddstr(WINDOW *, int, int, const char *, int);
