
#define MPD_HOST_ENV "MPD_HOST"
#define MPD_PORT_ENV "MPD_PORT"

#define NCMPCRC_ENV  "NCMPCRC"


typedef struct 
{
  char *host;
  int   port;
  int   reconnect;
  int   debug;
  int   find_wrap;

  int   enable_colors;
  int   bg_color;
  int   title_color;
  int   line_color;
  int   list_color;
  int   progress_color;
  int   status_color;
  int   alert_color;

} options_t;

extern options_t options;

options_t *options_init(void);
options_t *options_parse(int argc, const char **argv);



