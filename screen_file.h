
char *file_get_header(mpd_client_t *c);

void file_clear_highlight(mpd_client_t *c, mpd_Song *song);
void file_clear_highlights(mpd_client_t *c);

void file_open(screen_t *screen, mpd_client_t *c);
void file_close(screen_t *screen, mpd_client_t *c);

void file_paint(screen_t *screen, mpd_client_t *c);
void file_update(screen_t *screen, mpd_client_t *c);

int  file_cmd(screen_t *screen, mpd_client_t *c, command_t cmd);

