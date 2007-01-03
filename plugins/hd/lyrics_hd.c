#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "src_lyrics.h"

char *check_lyr_hd(char *artist, char *title, int how)
{ //checking whether for lyrics file existence and proper access
        static char path[1024];
        snprintf(path, 1024, "%s/.lyrics/%s/%s.lyric", 
                        getenv("HOME"), artist, title);
    
    if(g_access(path, how) != 0) return NULL;
                 
        return path;
}		


int get_lyr_hd(char *artist, char *title)
{
        char *path = check_lyr_hd(artist, title, R_OK);
        if(path == NULL) return -1;
        
        FILE *lyr_file;	
        lyr_file = fopen(path, "r");
        if(lyr_file == NULL) return -1;
        
        char *buf = NULL;
        char **line = &buf;
        size_t n = 0;
        
        while(1)
        {
         n = getline(line, &n, lyr_file); 
         if( n < 1 || *line == NULL || feof(lyr_file) != 0 ) return 0;
         add_text_line(&lyr_text, *line, n);
         free(*line);
         *line = NULL; n = 0;
         }
        
        return 0;
}	

int register_me (src_lyr *source_descriptor)
{ 
  source_descriptor->check_lyr = check_lyr_hd;
  source_descriptor->get_lyr = get_lyr_hd;
  
  source_descriptor->name = "Harddisk";
  source_descriptor->description = "";
}
