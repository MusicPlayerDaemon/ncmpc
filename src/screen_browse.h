
void clear_highlights(mpdclient_filelist_t *filelist);
void sync_highlights(mpdclient_t *c, mpdclient_filelist_t *filelist);
void set_highlight(mpdclient_filelist_t *filelist, 
		   mpd_Song *song, 
		   int highlight);


char *browse_lw_callback(int index, int *highlight, void *filelist);

int browse_handle_select(screen_t *screen, 
			 mpdclient_t *c,
			 list_window_t *lw,
			 mpdclient_filelist_t *filelist);
int browse_handle_select_all (screen_t *screen, 
		     mpdclient_t *c,
		     list_window_t *lw,
		     mpdclient_filelist_t *filelist);
int browse_handle_enter(screen_t *screen, 
			mpdclient_t *c,
			list_window_t *lw,
			mpdclient_filelist_t *filelist);

#ifdef HAVE_GETMOUSE
int browse_handle_mouse_event(screen_t *screen, 
			      mpdclient_t *c,
			      list_window_t *lw,
			      mpdclient_filelist_t *filelist);
#else
#define browse_handle_mouse_event(s,c,lw,filelist) (0)
#endif



