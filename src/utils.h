#ifndef UTILS_H
#define UTILS_H


/* functions for lists containing strings */
GList *string_list_free(GList *string_list);
GList *string_list_find(GList *string_list, const gchar *str);
GList *string_list_remove(GList *string_list, const gchar *str);

/* create a string list from path - used for completion */
#define GCMP_TYPE_DIR       (0x01 << 0)
#define GCMP_TYPE_FILE      (0x01 << 1)
#define GCMP_TYPE_PLAYLIST  (0x01 << 2)
#define GCMP_TYPE_RFILE     (GCMP_TYPE_DIR | GCMP_TYPE_FILE)
#define GCMP_TYPE_RPLAYLIST (GCMP_TYPE_DIR | GCMP_TYPE_PLAYLIST)

GList *gcmp_list_from_path(mpdclient_t *c,
			   const gchar *path,
			   GList *list,
			   gint types);

#endif
