#include "config.h"
#include "misc.h"
#include "a_string.h"
#include "http_download.h"
#include "gui_inprogress.h"
#include <curl/curl.h>
#include <string.h>
#include <errno.h>

static int progress_download(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    GtkWidget *progress = (GtkWidget *)clientp;
    UNUSED(ultotal);
    UNUSED(ulnow);

    if (dltotal==0)
        return !gui_inprogress_pulse(progress);
    return !gui_inprogress_set_fraction(progress,dlnow/dltotal);
}


int http_download(const char *src_url, const char *dst_filename)
{
    CURL *curl;
    CURLcode res;
    GtkWidget *progress;  
    a_string_t *user_agent;
    a_string_t *progress_title;
    FILE *temp_fd;
    
    curl = curl_easy_init();
    
    if (!curl)
        return 0;

    progress_title    = a_strnew(NULL); 
    a_sprintf(progress_title,"Updating %s",dst_filename);
    
    user_agent = a_strnew(NULL);     
    a_sprintf(user_agent,"cardpeek/%s",VERSION);

    progress = gui_inprogress_new(a_strval(progress_title),"Please wait...");
    
    temp_fd = fopen(dst_filename,"w");

    curl_easy_setopt(curl,CURLOPT_URL,src_url);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA, temp_fd);
    curl_easy_setopt(curl,CURLOPT_USERAGENT, a_strval(user_agent));
    curl_easy_setopt(curl,CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl,CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl,CURLOPT_PROGRESSFUNCTION, progress_download);
    curl_easy_setopt(curl,CURLOPT_PROGRESSDATA, progress);

    res = curl_easy_perform(curl);

    fclose(temp_fd);

    if (res!=CURLE_OK)
    {
        log_printf(LOG_ERROR,"Failed to fetch %s: %s", src_url, curl_easy_strerror(res));
        unlink(dst_filename);
    }

    curl_easy_cleanup(curl);

    gui_inprogress_free(progress);

    a_strfree(user_agent);
    a_strfree(progress_title);

    return (res==CURLE_OK);
} 
