#ifndef UTILS_H
#define UTILS_H

/* functions for lists containing strings */
GList *string_list_free(GList *string_list);
GList *string_list_find(GList *string_list, gchar *str);
GList *string_list_remove(GList *string_list, gchar *str);

/* create a string list from path - used for completion */
GList *gcmp_list_from_path(mpdclient_t *c, gchar *path, GList *list);

#endif
