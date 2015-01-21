#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "misc.h"
#include "bytestring.h"
#include "a_string.h"
#include "cardpeek_public_key.h"
#include "pathconfig.h"
#include "lua_ext.h"
#include "ui.h"
#include "http_download.h"
#include <glib.h>
#include <glib/gstdio.h>
#include "system_info.h"
#include "cardpeek_update.h"
#include "win32/config.h"

/********************************************************
 *
 * new_path()
 */

static char *new_path(unsigned basepath, const char *basename, const char *extension)
{
    const char *basepath_string = path_config_get_string(basepath);
    char *n_path;
    unsigned n_path_len;

    if (basepath_string==NULL)
        return NULL;

    n_path_len = strlen(basepath_string)+1;
    if (basename)
    {
        n_path_len += strlen(basename)+1;
        if (extension)
            n_path_len += strlen(extension)+1;
    }
    n_path = g_malloc(n_path_len);

    strcpy(n_path,basepath_string);

    if (basename)
    {
        if (basepath_string[strlen(basepath_string)-1]!='/')
            strcat(n_path,"/");
        if (*basename=='/') basename++;
        strcat(n_path,basename);
        if (extension)
        {
            strcat(n_path,extension);
        }
    }

    return n_path;
}


/******************************************************
 *
 *  tokenizer_* private routines to parse update files
 */

static char *tokenizer_string_alloc(char *value, int len)
{
    char *ret = g_malloc(len+1);
    char *ptr;
    int i;

    ptr = ret;

    for (i=0;i<len;i++)
    {
        if (value[i]=='\\' && i<len-1)
        {
            i++;
            switch (value[i]) 
            {
                case 'n':
                    *ptr++ = '\n';
                    break;
                case 'r':
                    *ptr++ = '\r';
                    break;
                case 't':
                    *ptr++ = '\t';
                    break;
                default:
                    *ptr++ = value[i];
            }
        }
        else
            *ptr++ = value[i];
    }
    *ptr=0;
    return ret;
}

static int tokenizer_getc(char **mem, unsigned *mem_len)
{
    int c;

    if (*mem_len==0) return -1;
    (*mem_len)--;
    c=**mem;
    (*mem)++;
    return c;
}

static char *tokenizer_get_record(char **ptr, unsigned *ptr_len)
{
    int c = tokenizer_getc(ptr,ptr_len);
    char *start = *ptr;
    int c_count = 0;

    if (c!='@') return NULL;
    
    while ((c = tokenizer_getc(ptr,ptr_len))>0) 
    {
        if (c=='\n')
            return tokenizer_string_alloc(start,*ptr-start-1);
        /* avoid abnormal length */
        if (c_count++>1000)
            break;
    } 
    return NULL;
}

static char *tokenizer_get_field(const char *field, char **ptr, unsigned *ptr_len)
{
    int c = tokenizer_getc(ptr,ptr_len);
    char *start = *ptr;
    int c_count = 0; 

    if (c!='.') return NULL;

    while ((c = tokenizer_getc(ptr,ptr_len))>0)
    {
        if (c=='=') break;
    }

    if (strncmp(start,field,*ptr-start-1)!=0) return NULL;

    start = *ptr;
    while ((c = tokenizer_getc(ptr,ptr_len))>0)
    {
        if (c=='\n') 
            return tokenizer_string_alloc(start,*ptr-start-1);
        if (c=='\\')
        {
            if (tokenizer_getc(ptr,ptr_len)<0) break;
        }
        /* avoid abnormal length */
        if (c_count++>4000)
            break;
    }
    return NULL;
}

/******************************************************
 *
 *  Crypto routines
 */

#include <openssl/ssl.h>
#include <openssl/err.h>

static int verify_signature(const bytestring_t *source, const bytestring_t *signature)
{
    static const char *pubkey_value = CARDPEEK_PUBLIC_KEY;
    BIO *bp;
    RSA* pubkey;
    int retval;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;

    if ((bp = BIO_new_mem_buf((void*)pubkey_value,strlen(pubkey_value)))==NULL)
    {
        log_printf(LOG_ERROR,"Failed to build BIO for memory object");
        return 0;
    }

    if ((pubkey = PEM_read_bio_RSA_PUBKEY(bp, NULL, NULL, NULL))==NULL)
    {
        log_printf(LOG_ERROR,"Failed to load public in memory");
        BIO_free(bp);
        return 0;
    }
    BIO_free(bp);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256,bytestring_get_data(source),bytestring_get_size(source));
    SHA256_Final(digest,&sha256);

    retval= RSA_verify(NID_sha256,
            digest,
            SHA256_DIGEST_LENGTH,
            (unsigned char *)bytestring_get_data(signature),
            bytestring_get_size(signature),
            pubkey);

    if (retval==0)
    {
        log_printf(LOG_ERROR,
                   "Signature verification failed for update information (%s)\n",
                   ERR_error_string(ERR_get_error(),NULL));
    }

    RSA_free(pubkey);

    return retval;
}

static int sha256sum(const char *filename, unsigned char *digest)
{
    SHA256_CTX sha256;
    unsigned char buffer[4096];
    int fd;
    int rlen;
    
    if ((fd = open(filename,O_RDONLY | O_BINARY))<0)
        return 0;

    
    SHA256_Init(&sha256);
    while ((rlen=read(fd,buffer,4096))>0)
    {
        SHA256_Update(&sha256,buffer,rlen);    
    }
    SHA256_Final(digest,&sha256);
    close(fd);

    return 1; 
}

/******************************************************
 *
 *  update_* routines to parse update files
 */

typedef struct _update_item_t {
    char *file;
    char *url;
    char *required_version;
    bytestring_t digest;
    char *message;
    struct _update_item_t *next;
} update_item_t;

typedef struct {
    char *update_version;
    int item_count;
    update_item_t *items; 
} update_t;

static update_item_t *update_item_new()
{
    update_item_t *item = g_new0(update_item_t,1);

    bytestring_init(&item->digest,8);

    return item;
}

static void update_item_free(update_item_t *item)
{
    if (item->file) free(item->file);
    if (item->url) free(item->url);
    if (item->required_version) free(item->required_version);
    bytestring_release(&item->digest);
    if (item->message) free(item->message);
    memset(item,0,sizeof(update_item_t));
    g_free(item);
}

static update_t *update_new()
{
    return g_new0(update_t,1);
}

static void update_clear(update_t *update)
{
    update_item_t *item = update->items;
    update_item_t *next;

    while (item)
    {
        next = item->next;
        update_item_free(item);
        item = next;
    }
    if (update->update_version) g_free(update->update_version);
    memset(update,0,sizeof(update_t));
}

static void update_free(update_t *update)
{
    update_clear(update);
    g_free(update);
}

static int update_load(update_t *update, const char *data, unsigned data_len)
{
    char *ptr = (char *)data;
    unsigned ptr_len = data_len;
    char *data_end;
    char *value;
    update_item_t *item = NULL;
    bytestring_t *signature;
    bytestring_t *source;
    int signature_verified;

#define fail_update(m) { log_printf(LOG_ERROR,"Failed to parse update information: %s",m); \
                         goto update_new_fail; }


    if ((value = tokenizer_get_record(&ptr,&ptr_len))==NULL) fail_update("Missing header");
    if (strcmp(value,"header")!=0) fail_update("Incorrect header");
    g_free(value);
    
    if ((value = tokenizer_get_field("version",&ptr,&ptr_len))==NULL) fail_update("missing version");
    update->update_version = value; /* don't free value */

    data_end = ptr;
    while ((value = tokenizer_get_record(&ptr,&ptr_len))!=NULL)
    {
        if (strcmp(value,"update")!=0) break;
        g_free(value);

        item = update_item_new();

        if ((value = tokenizer_get_field("file",&ptr,&ptr_len))==NULL) 
            fail_update("missing file name");
        item->file = value;

        if ((value = tokenizer_get_field("url",&ptr,&ptr_len))==NULL) 
            fail_update("missing url");
        item->url = value;

        if ((value = tokenizer_get_field("required_version",&ptr,&ptr_len))==NULL) 
            fail_update("missing required version");
        item->required_version = value;

        if ((value = tokenizer_get_field("digest",&ptr,&ptr_len))==NULL) 
            fail_update("missing digest");
        bytestring_assign_digit_string(&item->digest,value);
        g_free(value);

        if (bytestring_get_size(&item->digest)!=SHA256_DIGEST_LENGTH) 
            fail_update("incorrect digest length");

        if ((value = tokenizer_get_field("message",&ptr,&ptr_len))==NULL)
            fail_update("missing message");
        item->message = value;

        update->item_count++;
        item->next = update->items;
        update->items = item;
        item = NULL;
        data_end = ptr;
    }
    if (value==NULL) 
        fail_update("missing section");
    if (strcmp(value,"authentication")!=0) 
        fail_update("missing authentication");
    g_free(value);

    if ((value = tokenizer_get_field("signature",&ptr,&ptr_len))==NULL) 
        fail_update("missing signature");

    signature = bytestring_new(8);
    bytestring_assign_digit_string(signature,value);
    g_free(value);

    log_printf(LOG_DEBUG,"Verifying %d bit signature on update information (%d bytes), representing %d files.",
               bytestring_get_size(signature)*8,data_end-data,update->item_count);


    source =  bytestring_new(8);
    bytestring_assign_data(source,data_end-data,(const unsigned char *)data);

    if (verify_signature(source,signature)==0)
    {
        log_printf(LOG_ERROR,"Signature verification failed on update information.");
        signature_verified = 0;
        update_clear(update);
    }
    else
    {
        log_printf(LOG_INFO,"Signature verification succeeded on update information.");
        signature_verified = 1;
    }

    bytestring_free(signature);
    bytestring_free(source);
  
    return signature_verified;

update_new_fail:
    if (value) g_free(value);
    if (update) update_clear(update);
    if (item) update_item_free(item);
    return 0;
}

static int update_filter_files(update_t *update)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    char* filename;
    update_item_t **prev = &(update->items);
    update_item_t *item = update->items;
    int excluded = 0;

    while (item)
    {
        filename = new_path(PATH_CONFIG_FOLDER_CARDPEEK,item->file,NULL);

        if (sha256sum(filename,digest)!=0)
        {
            if (memcmp(digest,bytestring_get_data(&item->digest),SHA256_DIGEST_LENGTH)!=0)
            {
                log_printf(LOG_INFO,"File %s needs to be updated.",filename);
                prev = &(item->next);
                item = item->next;
            }
            else
            {
                /* log_printf(LOG_DEBUG,"No update needed for %s",filename); */
                *prev = item->next;                    
                update_item_free(item);
                excluded++;
                item = *prev;
                update->item_count--;
            }
        }
        else
        {
            log_printf(LOG_INFO,"File %s is proposed for creation in current script updates.",filename);
            prev = &(item->next);
            item = item->next;
        }

        g_free(filename);
    }
    return excluded;
}

static int update_filter_version(update_t *update, const char *version)
{
    update_item_t **prev = &(update->items);
    update_item_t *item = update->items;
    int excluded = 0;

    while (item)
    {
        if (version_to_bcd(item->required_version)>version_to_bcd(version))
        {
            *prev = item->next;
            update_item_free(item);
            excluded++;
            item = *prev;
            update->item_count--;
        }
        else
        {
            prev = &(item->next);
            item = item->next;
        }
    }
    return excluded;
}

static a_string_t *file_get_contents(const char *fname)
{
    struct stat st;
    int fd = open(fname,O_RDONLY | O_BINARY);
    a_string_t *res; 

    if (fd<0) return NULL;

    if (fstat(fd,&st)<0) 
    {
	log_printf(LOG_ERROR,"Could not stat file '%s'",fname);
        close(fd);
        return NULL;
    }
    
    res = a_strnnew(st.st_size,NULL);

    if (read(fd,res->_data,st.st_size)!=st.st_size)
    {
	log_printf(LOG_ERROR,"Could not read content of file '%s'",fname);
        close(fd);
        a_strfree(res);
        return NULL;
    }
    close(fd);

    return res;
}

/*************/

const char DEFAULT_UPDATE_URL[] = "http://downloads.pannetrat.com/updates/cardpeek.update";

int cardpeek_update_check(void)
{
    int retval = 0;
    time_t now = time(NULL);
    time_t next_update = (time_t)luax_variable_get_integer("cardpeek.updates.next_update");
    time_t next_update_distance;

    if (!luax_variable_is_defined("cardpeek.updates"))
    {
        luax_variable_set_boolean("cardpeek.updates.auto_update",TRUE);
        luax_variable_set_integer("cardpeek.updates.next_update",0);
        luax_variable_set_strval("cardpeek.updates.url",DEFAULT_UPDATE_URL);
        luax_variable_set_integer("cardpeek.updates.first_update",(int)now);
        luax_config_table_save();
    }

    if (luax_variable_get_boolean("cardpeek.updates.auto_update")!=TRUE)
    {
        log_printf(LOG_INFO,"cardpeek scripts auto-update is disabled.");
        return 0;
    }

    if (now<next_update)
    {
        next_update_distance = (next_update-now)/(3600*24);
        if (next_update_distance<1)
            log_printf(LOG_DEBUG,"cardpeek scripts will be scheduled for update in less than a day.");
        else
            log_printf(LOG_DEBUG,"cardpeek scripts will be scheduled for update in about %u day(s).",next_update_distance);
        return 0;
    }

    switch (ui_question("Cardpeek is configured to check for script updates periodically.\n"
                        "Do you whish to perform this check now?",
                        "Yes","No, ask me again later","No, always use the local copy",NULL))
    {
        case 0:
            retval = 1;
            break;
        case 1:
            luax_variable_set_integer("cardpeek.updates.next_update",(int)(now+(24*3600)));
            luax_config_table_save();
            break;
        case 2:
            luax_variable_set_boolean("cardpeek.updates.auto_update",FALSE);
            luax_config_table_save();
            break;
    }

    return retval;
}

int cardpeek_update_perform(void)
{
    const char* cardpeek_update_file = path_config_get_string(PATH_CONFIG_FILE_CARDPEEK_UPDATE);
    a_string_t *contents;
    update_t *update;
    int remove;
    update_item_t *item;
    time_t now = time(NULL);
    int updated = 0;
    char *url = NULL;
    char *local_file;
    char *local_dnld;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    a_string_t *url_request;
    unsigned first_update;

    first_update = (unsigned)luax_variable_get_integer("cardpeek.updates.first_update");
    
    /* STEP 1: get cardpeek.update file */

    url=luax_variable_get_strdup("cardpeek.updates.url");

    if (url==NULL)
        url = g_strdup(DEFAULT_UPDATE_URL);

    log_printf(LOG_INFO,"Fetching '%s'",url);

    url_request = a_strnew(NULL);
    a_sprintf(url_request,"%s?u=%x%x&v=%s&s=%s",url,first_update,system_name_hash(),VERSION,system_type());

    if (http_download(a_strval(url_request),cardpeek_update_file)==0)
    {
        g_free(url);
        return 0;
    }
    g_free(url);
    a_strfree(url_request);


    /* STEP 2: parse file */

    if ((contents=file_get_contents(cardpeek_update_file))==NULL)
    {
        log_printf(LOG_ERROR,"failed to read update file information.");
        unlink(cardpeek_update_file);
        return 0;
    }

    update = update_new();

    if ((update_load(update,a_strval(contents),a_strlen(contents)))==0)
    {
        unlink(cardpeek_update_file);
        a_strfree(contents);
        update_free(update);
        return 0;
    }
    a_strfree(contents);

    /* log_printf(LOG_DEBUG,"Updates correctly loaded from '%s'",cardpeek_update_file); */
    if ((remove = update_filter_version(update,VERSION))>0)
        log_printf(LOG_WARNING,"%d updates will not be installed because they require a newer version of Cardpeek.");
    
    remove = update_filter_files(update);

    if (update->item_count)
        log_printf(LOG_INFO,"A total of %d files will be updated, %d files are kept unchanged.",update->item_count,remove);
    else
        log_printf(LOG_INFO,"No files will be updated, %d files are kept unchanged.",remove);

    item = update->items;

    while (item)
    {
        local_dnld = new_path(PATH_CONFIG_FOLDER_CARDPEEK,item->file,".download");
        local_file = new_path(PATH_CONFIG_FOLDER_CARDPEEK,item->file,NULL);

        if (http_download(item->url,local_dnld)!=0)
        {        
            if (sha256sum(local_dnld,digest))
            {
                if (memcmp(digest,bytestring_get_data(&item->digest),SHA256_DIGEST_LENGTH)==0)
                {
                    unlink(local_file);

                    if (rename(local_dnld,local_file)==0)
                    {
                        log_printf(LOG_INFO,"Successfuly updated %s", local_file);
                        updated++;
                    }
                    else
                    {
                        log_printf(LOG_ERROR,"Failed to copy %s to %s: %s", 
                                local_dnld,
                                local_file, 
                                strerror(errno));
                    }
                }
                else
                {
                    log_printf(LOG_WARNING,"File %s was not updated: authentication failed.",local_file);
                }
            }
            unlink(local_dnld);
        }
        g_free(local_dnld);
        g_free(local_file);
        item =  item->next; 
    }

    if (updated == update->item_count)
    {    
        luax_variable_set_integer("cardpeek.updates.next_update",(int)(now+7*(24*3600)));  
        luax_config_table_save();
    }

    unlink(cardpeek_update_file);
    update_free(update);

    /* STEP 3: finish */

    return 1;

}

