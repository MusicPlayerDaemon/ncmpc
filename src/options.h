
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
  int   port;
  int   reconnect;
  int   debug;
  int   find_wrap;
  int   auto_center;
  int   wide_cursor;  
  int   enable_colors;
  int   enable_beep;

} options_t;

extern options_t options;

options_t *options_init(void);
options_t *options_parse(int argc, const char **argv);



