
typedef struct
{
  char selected;
  mpd_InfoEntity *entity;
} filelist_entry_t;

typedef struct
{
  mpd_Connection *connection;
  mpd_Status     *status;

  mpd_Song       *song;
  int            song_id;
  int            song_updated;

  int            seek_song_id;
  int            seek_target_time;

  GList         *playlist;
  int            playlist_length;
  long long      playlist_id;
  int            playlist_updated;

  char           *cwd;
  GList          *filelist;
  int            filelist_length;
  int            filelist_updated;

} mpd_client_t;


int mpc_close(mpd_client_t *c);

mpd_client_t *mpc_connect(char *host, int port, char *passwd);
int mpc_reconnect(mpd_client_t *c, char *host, int port, char *passwd);

int mpc_update(mpd_client_t *c);
int mpc_update_playlist(mpd_client_t *c);

int mpc_update_filelist(mpd_client_t *c);
int mpc_filelist_set_selected(mpd_client_t *c);
int mpc_set_cwd(mpd_client_t *c, char *dir);

mpd_Song *mpc_playlist_get_song(mpd_client_t *c, int n);
char *mpc_get_song_name(mpd_Song *song);
char *mpc_get_song_name2(mpd_Song *song);
int mpc_playlist_get_song_index(mpd_client_t *c, char *filename);

int   mpc_error(mpd_client_t *c);
char *mpc_error_str(mpd_client_t *c);
