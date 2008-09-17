#ifndef LIST_WINDOW_H
#define LIST_WINDOW_H

#include "command.h"

#include <ncurses.h>
#include <glib.h>

#define LW_ROW(lw) (lw ? lw->selected-lw->start :  0)

#define LW_HIDE_CURSOR    0x01

typedef const char *(*list_window_callback_fn_t)(unsigned index,
						 int *highlight,
						 void *data);

typedef struct list_window {
	WINDOW *w;
	unsigned rows, cols;

	unsigned start;
	unsigned selected;
	unsigned xoffset;
	int clear;
	int repaint;
	int flags;
} list_window_t;

typedef struct list_window_state {
	GList *list;
} list_window_state_t;


/* create a new list window */
struct list_window *list_window_init(WINDOW *w,
				     unsigned width, unsigned height);

/* destroy a list window (returns NULL) */
struct list_window *list_window_free(struct list_window *lw);

/* reset a list window (selected=0, start=0, clear=1) */
void list_window_reset(struct list_window *lw);

/* paint a list window */
void list_window_paint(struct list_window *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);

/* perform basic list window commands (movement) */
int list_window_cmd(struct list_window *lw, unsigned rows, command_t cmd);

/**
 * Scroll the window.  Returns non-zero if the command has been
 * consumed.
 */
int
list_window_scroll_cmd(struct list_window *lw, unsigned rows, command_t cmd);

void
list_window_center(struct list_window *lw, unsigned rows, unsigned n);

/* select functions */
void list_window_set_selected(struct list_window *lw, unsigned n);
void list_window_previous(struct list_window *lw, unsigned length);
void list_window_next(struct list_window *lw, unsigned length);
void list_window_first(struct list_window *lw);
void list_window_last(struct list_window *lw, unsigned length);
void list_window_previous_page(struct list_window *lw);
void list_window_next_page(struct list_window *lw, unsigned length);
void list_window_check_selected(struct list_window *lw, unsigned length);

/* find a string in a list window */
int  list_window_find(struct list_window *lw,
		      list_window_callback_fn_t callback,
		      void *callback_data,
		      const char *str,
		      int wrap);

/* find a string in a list window (reversed) */
int
list_window_rfind(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  int wrap,
		  unsigned rows);

/* list window states */
list_window_state_t *list_window_init_state(void);
list_window_state_t *list_window_free_state(list_window_state_t *state);
void list_window_push_state(list_window_state_t *state,
			    struct list_window *lw);
bool list_window_pop_state(list_window_state_t *state, struct list_window *lw);



#endif
