
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



list_window_t *list_window_init(WINDOW *w, int width, int height);
list_window_t *list_window_free(list_window_t *lw);


void list_window_reset(list_window_t *lw);
void list_window_set_selected(list_window_t *lw, int n);

void list_window_paint(list_window_t *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);




void list_window_previous(list_window_t *lw);
void list_window_next(list_window_t *lw, int length);
void list_window_first(list_window_t *lw);
void list_window_last(list_window_t *lw, int length);
void list_window_previous_page(list_window_t *lw);
void list_window_next_page(list_window_t *lw, int length);
int  list_window_find(list_window_t *lw, 
		      list_window_callback_fn_t callback,
		      void *callback_data,
		      char *str);


char *screen_readln(WINDOW *w, char *prompt);
