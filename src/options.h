
#define MPD_HOST_ENV "MPD_HOST"
#define MPD_PORT_ENV "MPD_PORT"

typedef struct 
{
  char *host;
  char *username;
  char *password;
  char *config_file;
  char *key_file;
  char *list_format;
  char *status_format;
  char *xterm_title_format;
  int   port;
  gboolean reconnect;
  gboolean debug;
  gboolean find_wrap;
  gboolean list_wrap;
  gboolean auto_center;
  gboolean wide_cursor;  
  gboolean enable_colors;
  gboolean audible_bell;       /* not implemented */
  gboolean visible_bell;       /* not implemented */
  gboolean enable_xterm_title; /* not implemented */

} options_t;

extern options_t options;

options_t *options_init(void);
options_t *options_parse(int argc, const char **argv);



