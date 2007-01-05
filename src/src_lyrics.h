#ifndef SOURCE_LYRICS
#define SOURCE_LYRICS

#include <stdlib.h>
#include <glib.h>
#include "list_window.h"
#include "mpdclient.h"
#include <gmodule.h>

typedef struct _formed_text
{
        GString *text;
        GArray *lines;
        int val;
} formed_text;

void formed_text_init(formed_text *text);
void add_text_line(formed_text *dest, const char *src, int len);

typedef struct _retrieval_spec
{
        mpdclient_t *client;
        int way;
} retrieval_spec;



static int lyrics_text_rows = -1;
static list_window_t *lw = NULL;

GTimer *dltime;
short int lock;
formed_text lyr_text;

guint8 result;

/* result is a bitset in which the success when searching 4 lyrics is logged
countend by position - backwards
0: lyrics in database
1: proper access  to the lyrics provider
2: lyrics found
3: exact match
4: lyrics downloaded
5: lyrics saved
wasting 3 bits doesn't mean being a fat memory hog like kde.... does it?
*/


typedef struct src_lyr src_lyr;

struct src_lyr
{
  char *name;
  char *source_name;
  char *description;
  
  int (*register_src_lyr) (src_lyr *source_descriptor);
  int (*deregister_src_lyr) ();

  int (*check_lyr) (char *artist, char *title, char *url);
  int (*get_lyr) (char *artist, char *title);
  int (*state_lyr) ();

#ifndef DISABLE_PLUGIN_SYSTEM
  GModule *module;
#endif
};

typedef int (*src_lyr_plugin_register) (src_lyr *source_descriptor);

GArray *src_lyr_stack;

int src_lyr_stack_init ();
int src_lyr_init ();
int get_lyr_by_src (int priority, char *artist, char *title);

#endif
