#ifndef LIST_WINDOW_H
#define LIST_WINDOW_H

typedef char *  (*list_window_callback_fn_t)   (int index, 
						int *highlight,
						void *data);

typedef struct
{
  WINDOW *w;
  int rows, cols;

  int start;
  int selected;
  int clear;
  int repaint;

} list_window_t;


/* create a new list window */
list_window_t *list_window_init(WINDOW *w, int width, int height);

/* destroy a list window (returns NULL) */
list_window_t *list_window_free(list_window_t *lw);

/* reset a list window (selected=0, start=0, clear=1) */
void list_window_reset(list_window_t *lw);

/* paint a list window */
void list_window_paint(list_window_t *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);

/* perform basic list window commands (movement) */
int list_window_cmd(list_window_t *lw, int rows, command_t cmd);


/* select functions */
void list_window_set_selected(list_window_t *lw, int n);
void list_window_previous(list_window_t *lw);
void list_window_next(list_window_t *lw, int length);
void list_window_first(list_window_t *lw);
void list_window_last(list_window_t *lw, int length);
void list_window_previous_page(list_window_t *lw);
void list_window_next_page(list_window_t *lw, int length);

/* find a string in a list window */
int  list_window_find(list_window_t *lw, 
		      list_window_callback_fn_t callback,
		      void *callback_data,
		      char *str,
		      int wrap);

/* find a string in a list window (reversed) */
int
list_window_rfind(list_window_t *lw, 
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  char *str,
		  int wrap,
		  int rows);

#endif
