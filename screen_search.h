
void search_open(screen_t *screen, mpd_client_t *c);
void search_close(screen_t *screen, mpd_client_t *c);

void search_paint(screen_t *screen, mpd_client_t *c);
void search_update(screen_t *screen, mpd_client_t *c);

int  search_cmd(screen_t *screen, mpd_client_t *c, command_t cmd);
