#ifndef WREADLN_H
#define WREADLN_H

/* max size allocated for a line */
extern unsigned int wrln_max_line_size;

/* max items stored in the history list */
extern unsigned int wrln_max_history_length;

/* a callback function for KEY_RESIZE */
extern GVoidFunc wrln_resize_callback;

/* called after TAB is pressed but before g_completion_complete */
typedef void (*wrln_gcmp_pre_cb_t) (GCompletion *gcmp, gchar *buf);
extern wrln_gcmp_pre_cb_t wrln_pre_completion_callback;

/* post completion callback */
typedef void (*wrln_gcmp_post_cb_t) (GCompletion *gcmp, gchar *s, GList *l);
extern wrln_gcmp_post_cb_t wrln_post_completion_callback;

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
