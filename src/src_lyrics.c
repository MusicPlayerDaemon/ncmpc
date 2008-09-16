/* 
 *	
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Thu Dec 28 23:17:38 2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "src_lyrics.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PLUGIN_DIR_USER "/.ncmpc/plugins"

int get_text_line(formed_text *text, unsigned num, char *dest, size_t len)
{
	int linelen;

	memset(dest, '\0', len*sizeof(char));
	if (num >= text->lines->len - 1)
		return -1;

	if (num == 0) {
		linelen = g_array_index(text->lines, int, num);
		memcpy(dest, text->text->str, linelen*sizeof(char));
	} else if (num == 1) { //dont ask me why, but this is needed....
		linelen = g_array_index(text->lines, int, num)
			- g_array_index(text->lines, int, num-1);
		memcpy(dest, &text->text->str[g_array_index(text->lines, int, num-1)],
		       linelen*sizeof(char));
	} else {
		linelen = g_array_index(text->lines, int, num+1)
			- g_array_index(text->lines, int, num);
		memcpy(dest, &text->text->str[g_array_index(text->lines, int, num)],
		       linelen*sizeof(char));
	}

	dest[linelen] = '\n';
	dest[linelen + 1] = '\0';

	return 0;
}

void add_text_line(formed_text *dest, const char *src, int len)
{

	// need this because g_array_append_val doesnt work with literals
	// and expat sends "\n" as an extra line everytime
	if(len == 0) {
		dest->val = strlen(src);
		if(dest->lines->len > 0) dest->val += g_array_index(dest->lines, int,
								    dest->lines->len-1);
		g_string_append(dest->text, src);
		g_array_append_val(dest->lines, dest->val);
		return;
	}

	if(len > 1 || dest->val == 0) {
			dest->val = len;
			if(dest->lines->len > 0) dest->val += g_array_index(dest->lines, int,
									    dest->lines->len-1);
	} else if (len < 6 && dest->val != 0)
		dest->val = 0;

	if (dest->val > 0) {
		g_string_append_len(dest->text, src, len);
		g_array_append_val(dest->lines, dest->val);
	}
}

void formed_text_init(formed_text *text)
{
	if (text->text != NULL)
		g_string_free(text->text, TRUE);
	text->text = g_string_new("");

	if (text->lines != NULL)
		g_array_free(text->lines, TRUE);
	text->lines = g_array_new(FALSE, TRUE, 4);

	text->val = 0;
}

#ifdef ENABLE_LYRSRC_LEOSLYRICS
int deregister_lyr_leoslyrics ();
int register_lyr_leoslyrics (src_lyr *source_descriptor);
#endif

#ifdef ENABLE_LYRSRC_HD
int deregister_lyr_hd ();
int register_lyr_hd (src_lyr *source_descriptor);
#endif

static int src_lyr_plugins_load(void);

void src_lyr_stack_init(void)
{
	src_lyr_stack = g_array_new (TRUE, FALSE, sizeof (src_lyr*));

#ifdef ENABLE_LYRSRC_HD
	src_lyr *src_lyr_hd = malloc (sizeof (src_lyr));
	src_lyr_hd->register_src_lyr = register_lyr_hd;
	g_array_append_val (src_lyr_stack, src_lyr_hd);
#endif
#ifdef ENABLE_LYRSRC_LEOSLYRICS
	src_lyr *src_lyr_leoslyrics = malloc (sizeof (src_lyr));
	src_lyr_leoslyrics->register_src_lyr = register_lyr_leoslyrics;
	g_array_append_val (src_lyr_stack, src_lyr_leoslyrics);
#endif

#ifndef DISABLE_PLUGIN_SYSTEM

	src_lyr_plugins_load ();
#endif
}

int src_lyr_init(void)
{
	int i = 0;

	src_lyr_stack_init ();

	while (g_array_index (src_lyr_stack, src_lyr*, i) != NULL) {
		src_lyr *i_stack;
		i_stack = g_array_index (src_lyr_stack, src_lyr*, i);
		i_stack->register_src_lyr (i_stack);
		i++;
	}
	return 0;
}

int get_lyr_by_src (int priority, char *artist, char *title)
{
	if(src_lyr_stack->len == 0) return -1;
	g_array_index (src_lyr_stack, src_lyr*, priority)->get_lyr (artist, title);
	return 0;
}

static int src_lyr_load_plugin_file(const char *file)
{
	GString *path;
	src_lyr *new_src;
	src_lyr_plugin_register register_func;

	path = g_string_new (PLUGIN_DIR_SYSTEM);
	g_string_append (path, "/");
	g_string_append (path, file);

	new_src = malloc(sizeof(src_lyr));
	new_src->module = g_module_open (path->str, G_MODULE_BIND_LAZY);
	if (!g_module_symbol (new_src->module, "register_me", (gpointer*) &register_func))
		return -1;
	new_src->register_src_lyr = register_func;
	g_array_append_val (src_lyr_stack, new_src);
	return 0;
}

static void src_lyr_plugins_load_from_dir(GDir *plugin_dir)
{
	const gchar *cur_file;

	for (;;) {
		cur_file = g_dir_read_name (plugin_dir);
		if (cur_file == NULL) break;
		src_lyr_load_plugin_file (cur_file);
	}
}

static int src_lyr_plugins_load(void)
{
	GDir *plugin_dir;
	GString *user_dir_path;

	plugin_dir = g_dir_open (PLUGIN_DIR_SYSTEM, 0, NULL);
	if (plugin_dir == NULL)
		return -1;
	src_lyr_plugins_load_from_dir (plugin_dir);

	user_dir_path = g_string_new (g_get_home_dir());
	g_string_append (user_dir_path, PLUGIN_DIR_USER);

	plugin_dir = g_dir_open (user_dir_path->str, 0, NULL);
	if (plugin_dir == NULL)
		return -1;
	src_lyr_plugins_load_from_dir (plugin_dir);

	return 0;
}
