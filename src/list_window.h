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

typedef struct {
	WINDOW *w;
	unsigned rows, cols;

	unsigned start;
	unsigned selected;
	unsigned xoffset;
	int clear;
	int repaint;
	int flags;
} list_window_t;

typedef struct {
	GList *list;
} list_window_state_t;


/* create a new list window */
list_window_t *list_window_init(WINDOW *w, unsigned width, unsigned height);

/* destroy a list window (returns NULL) */
list_window_t *list_window_free(list_window_t *lw);

/* reset a list window (selected=0, start=0, clear=1) */
void list_window_reset(list_window_t *lw);

/* paint a list window */
void list_window_paint(list_window_t *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);

/* perform basic list window commands (movement) */
int list_window_cmd(list_window_t *lw, unsigned rows, command_t cmd);


/* select functions */
void list_window_set_selected(list_window_t *lw, unsigned n);
void list_window_previous(list_window_t *lw, unsigned length);
void list_window_next(list_window_t *lw, unsigned length);
void list_window_first(list_window_t *lw);
void list_window_last(list_window_t *lw, unsigned length);
void list_window_previous_page(list_window_t *lw);
void list_window_next_page(list_window_t *lw, unsigned length);
void list_window_check_selected(list_window_t *lw, unsigned length);

/* find a string in a list window */
int  list_window_find(list_window_t *lw,
		      list_window_callback_fn_t callback,
		      void *callback_data,
		      const char *str,
		      int wrap);

/* find a string in a list window (reversed) */
int
list_window_rfind(list_window_t *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  int wrap,
		  unsigned rows);

/* list window states */
list_window_state_t *list_window_init_state(void);
list_window_state_t *list_window_free_state(list_window_state_t *state);
void list_window_push_state(list_window_state_t *state, list_window_t *lw);
bool list_window_pop_state(list_window_state_t *state, list_window_t *lw);



#endif
