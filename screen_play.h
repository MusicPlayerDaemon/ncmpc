
int play_get_selected(void);

int playlist_move_song(mpd_client_t *c, int old_index, int new_index);
int playlist_add_song(mpd_client_t *c, mpd_Song *song);
int playlist_delete_song(mpd_client_t *c, int index);

screen_functions_t *get_screen_playlist(void);

