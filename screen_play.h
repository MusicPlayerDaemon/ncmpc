

void play_open(screen_t *screen, mpd_client_t *c);
void play_close(screen_t *screen, mpd_client_t *c);

void play_paint(screen_t *screen, mpd_client_t *c);
void play_update(screen_t *screen, mpd_client_t *c);

int  play_cmd(screen_t *screen, mpd_client_t *c, command_t cmd);

