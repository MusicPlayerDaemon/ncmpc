#ifndef WREADLN_H
#define WREADLN_H

/* max size allocated for a line */
extern unsigned int wrln_max_line_size;

/* max items stored in the history list */
extern unsigned int wrln_max_history_length;

/* a callback function for KEY_RESIZE */
extern GVoidFunc wrln_resize_callback;

/* Note, wreadln calls curs_set() and noecho(), to enable cursor and 
 * disable echo. wreadln will not restore these settings when exiting! */
char *wreadln(WINDOW *w,           /* the curses window to use */
	      char *prompt,        /* the prompt string or NULL */
	      char *initial_value, /* initial value or NULL for a empty line
				    * (char *) -1 => get value from history */
	      int x1,              /* the maximum x position or 0 */
	      GList **history,     /* a pointer to a history list or NULL */ 
	      GCompletion *gcmp    /* a GCompletion structure or NULL */
	      );


#endif
