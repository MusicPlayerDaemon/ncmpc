
#define MPD_HOST_ENV "MPD_HOST"
#define MPD_PORT_ENV "MPD_PORT"


typedef struct 
{
  char *host;
  int   port;
  int   reconnect;
  int   debug;

} options_t;

void options_init(void);
options_t *options_parse(int argc, char **argv);
options_t *get_options(void);

