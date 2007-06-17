#include "src_lyrics.h"
#include "easy_download.h"
#include <expat.h>
#include <string.h>
#include "options.h"

#define LEOSLYRICS_SEARCH_URL "http://api.leoslyrics.com/api_search.php?auth=ncmpc&artist=%s&songtitle=%s"
#define LEOSLYRICS_CONTENT_URL "http://api.leoslyrics.com/api_lyrics.php?auth=ncmpc&hid=%s"
#define CREDITS "Lyrics provided by www.LeosLyrics.com"

char *hid;
XML_Parser parser, contentp;

int check_dl_progress(void *clientp, double dltotal, double dlnow,
                        double ultotal, double ulnow)
{
        if(g_timer_elapsed(dltime, NULL) >= options.lyrics_timeout || lock == 4)
        {	
                formed_text_init(&lyr_text);
                return -1;
        }

        return 0;
}	



static void check_content(void *data, const char *name, const char **atts)
{ 
        if(strstr(name, "text") != NULL)
        {

                result |= 16;
        }
}
        

static void check_search_response(void *data, const char *name,
                 const char **atts)
{
        if(strstr(name, "response") != NULL)
        {
        result |=2;
        return;
        }  
            
        if(result & 4)
        {
                if(strstr(name, "result") != NULL)
                {
                        if(strstr(atts[2], "hid") != NULL)
                        {
                                hid = strdup (atts[3]);
                        }
        
                        if(strstr(atts[2], "exactMatch") != NULL)
                        {
                                result |= 8;
                        }			
                }
        }
                        
}

static void end_tag(void *data, const char *name)
{
  //hmmmmmm		
}

  static void check_search_success(void *userData, const XML_Char *s, int len)
    {
        if(result & 2)	//lets first check whether we're right
        {		//we don't really want to search in the wrong string
                if(strstr((char*) s, "SUCCESS"))
                {
                result |=4;
                }
        }	
    }

static void fetch_text(void *userData, const XML_Char *s, int len) 
{
        if(result & 16)
        {
	  	if (s[0] == 13 ) return; //ignore any single carriage returns
                add_text_line(&lyr_text, s, len); 
        }
}

/*int deregister_lyr_leoslyrics ()
{
  
}*/

int check_lyr_leoslyrics(char *artist, char *title, char *url)
{
        char url_avail[256];

        //this replacess the whitespaces with '+'
        g_strdelimit(artist, " ", '+');
        g_strdelimit(title, " ", '+');
        
        //we insert the artist and the title into the url		
        snprintf(url_avail, 512, LEOSLYRICS_SEARCH_URL, artist, title);

        //download that xml!
        easy_download_struct lyr_avail = {NULL, 0,-1};	
        
        g_timer_start(dltime);
        if(!easy_download(url_avail, &lyr_avail, check_dl_progress)) return -1;
        g_timer_stop(dltime);

        //we gotta parse that stuff with expat
        parser = XML_ParserCreate(NULL);
        XML_SetUserData(parser, NULL);
        
        XML_SetElementHandler(parser, check_search_response, end_tag);
        XML_SetCharacterDataHandler(parser, check_search_success);
        XML_Parse(parser, lyr_avail.data, strlen(lyr_avail.data), 0);	
        XML_ParserFree(parser);	

        if(!(result & 4)) return -1; //check whether lyrics found

	CURL *curl = curl_easy_init ();
	char *esc_hid = curl_easy_escape (curl, hid, 0);
	free (hid);

        snprintf(url, 512, LEOSLYRICS_CONTENT_URL, esc_hid);

        return 0;
}

int get_lyr_leoslyrics(char *artist, char *title)
{
        char url_hid[256];
        if(dltime == NULL) dltime = g_timer_new();

        if(check_lyr_leoslyrics(artist, title, url_hid) != 0) return -1;
        
        easy_download_struct lyr_content = {NULL, 0,-1};  
        g_timer_continue(dltime);		
        if(!(easy_download(url_hid, &lyr_content, check_dl_progress))) return -1;
        g_timer_stop(dltime);
        
        contentp = XML_ParserCreate(NULL);
        XML_SetUserData(contentp, NULL);
        XML_SetElementHandler(contentp, check_content, end_tag);	
        XML_SetCharacterDataHandler(contentp, fetch_text);
        XML_Parse(contentp, lyr_content.data, strlen(lyr_content.data), 0);
        XML_ParserFree(contentp);

        return 0;
        
}
int register_lyr_leoslyrics (src_lyr *source_descriptor)
{ 
  source_descriptor->check_lyr = check_lyr_leoslyrics;
  source_descriptor->get_lyr = get_lyr_leoslyrics;
  
  source_descriptor->name = "Leoslyrics";
  source_descriptor->description = "powered by http://www.leoslyrics.com";
}
