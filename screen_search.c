#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "libmpdclient.h"
#include "config.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_search.h"

void 
search_open(screen_t *screen, mpd_client_t *c)
{
}

void 
search_close(screen_t *screen, mpd_client_t *c)
{
}

void 
search_paint(screen_t *screen, mpd_client_t *c)
{
}

void 
search_update(screen_t *screen, mpd_client_t *c)
{
}

int 
search_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  return 0;
}
