#ifndef WREADLN_H
#define WREADLN_H

/* max size allocated for a line */
extern guint wrln_max_line_size;

/* max items stored in the history list */
extern guint wrln_max_history_length;

/* custom wgetch function */
typedef int (*wrln_wgetch_fn_t) (WINDOW *w);
extern wrln_wgetch_fn_t wrln_wgetch;

/* completion callback data */
extern void *wrln_completion_callback_data;

/* called after TAB is pressed but before g_completion_complete */
typedef void (*wrln_gcmp_pre_cb_t) (GCompletion *gcmp, gchar *buf, void *data);
extern wrln_gcmp_pre_cb_t wrln_pre_completion_callback;

/* post completion callback */
typedef void (*wrln_gcmp_post_cb_t) (GCompletion *gcmp, gchar *s, GList *l,
                                     void *data);
extern wrln_gcmp_post_cb_t wrln_post_completion_callback;

/* Note, wreadln calls curs_set() and noecho(), to enable cursor and 
 * disable echo. wreadln will not restore these settings when exiting! */
gchar *wreadln(WINDOW *w,            /* the curses window to use */
	       gchar *prompt,        /* the prompt string or NULL */
	       gchar *initial_value, /* initial value or NULL for a empty line
				      * (char *) -1 = get value from history */
	       gint x1,              /* the maximum x position or 0 */
	       GList **history,     /* a pointer to a history list or NULL */ 
	       GCompletion *gcmp    /* a GCompletion structure or NULL */
	       );


#endif
