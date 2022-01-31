#include "url.h"

//#define URL_DEBUG 1

void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL)
		return;
	
	s->ptr[0] = '\0';
}

void URL_init(struct URL_Request *u) {
	u->formpost = NULL;
	u->lastptr = NULL;
	u->headerlist = NULL;
	
	u->curl = curl_easy_init();
}

void URL_add_form(struct URL_Request *u, char *name, char *content) {
	//char *econtent = NULL;
	//econtent = curl_easy_escape(u->curl, content, 0);
	curl_formadd(&u->formpost, &u->lastptr, CURLFORM_COPYNAME, name, CURLFORM_COPYCONTENTS, content, CURLFORM_END);
	//curl_free(econtent);
}

void URL_add_header(struct URL_Request *u, char *str) {
	u->headerlist = curl_slist_append(u->headerlist, str);	
}

size_t URL_writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL)
		return -1;
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;
	
	return size*nmemb;
}

int URL_GET_request(struct URL_Request *u, char *url, struct string *out) {
	int len = 0;
#ifdef URL_DEBUG
	LOG_MSG(LOG_INFO, "GET URL: %s\n", url);
#endif
	
	curl_easy_setopt(u->curl, CURLOPT_URL, url);
	curl_easy_setopt(u->curl, CURLOPT_WRITEFUNCTION, URL_writefunc);
	curl_easy_setopt(u->curl, CURLOPT_WRITEDATA, out);
	curl_easy_setopt(u->curl, CURLOPT_FOLLOWLOCATION, 1); 
    curl_easy_setopt(u->curl, CURLOPT_COOKIEFILE, PAM_CAS_COOKIESFILE); /* just to start the cookie engine */


#ifdef SKIP_PEER_VERIFICATION
	curl_easy_setopt(u->curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
	curl_easy_setopt(u->curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif		

	u->res = curl_easy_perform(u->curl);
	//print_cookies(u->curl);
	return len; 
}

int URL_POST_request(struct URL_Request *u, char *url, struct string *out) {
        int len = 0;
        curl_easy_setopt(u->curl, CURLOPT_URL, url);

#ifdef URL_DEBUG
	LOG_MSG(LOG_INFO, "POST URL: %s\n", url);
#endif

	if (u->formpost != NULL) {
        // siu.upm.es does not uses multipart post, just urlencoded body
        // so need to translate it into proper string
        //curl_easy_setopt(u->curl, CURLOPT_HTTPPOST, u->formpost);
        char postdata[2048]; //should be enought
        memset(postdata,0,sizeof(postdata));
        for( struct curl_httppost *entry=u->formpost;entry->next;entry=entry->next) {
            strcat(postdata,entry->name);
            strcat(postdata,"=");
            strcat(postdata,entry->contents);
            if (entry->next) strcat(postdata,"&");
        }
        curl_easy_setopt(u->curl, CURLOPT_POSTFIELDS, postdata);
    }

	if (u->headerlist != NULL) {
        curl_easy_setopt(u->curl, CURLOPT_HTTPHEADER, u->headerlist);
    }
    curl_easy_setopt(u->curl, CURLOPT_USERAGENT, "Curl/7.79.1");
	curl_easy_setopt(u->curl, CURLOPT_FOLLOWLOCATION, 1); 
    curl_easy_setopt(u->curl, CURLOPT_WRITEFUNCTION, URL_writefunc);
    curl_easy_setopt(u->curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(u->curl, CURLOPT_COOKIEFILE, PAM_CAS_COOKIESFILE); /* just to start the cookie engine */
#ifdef SKIP_PEER_VERIFICATION
    curl_easy_setopt(u->curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    curl_easy_setopt(u->curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    u->res = curl_easy_perform(u->curl);
    return len;
}

void
print_cookies(struct URL_Request *u)
{
  CURLcode res;
  struct curl_slist *cookies;
  struct curl_slist *nc;
  int i;

  printf("Cookies, curl knows:\n");
  res = curl_easy_getinfo(u->curl, CURLINFO_COOKIELIST, &cookies);
  if (res != CURLE_OK) {
    fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
    exit(1);
  }
  nc = cookies, i = 1;
  while (nc) {
    printf("[%d]: %s\n", i, nc->data);
    nc = nc->next;
    i++;
  }
  if (i == 1) {
    printf("(none)\n");
  }
  curl_slist_free_all(cookies);
}

void URL_cleanup(struct URL_Request *u) {
	if (u->headerlist != NULL)
		curl_slist_free_all(u->headerlist);
	if (u->formpost != NULL)
		curl_formfree(u->formpost);
	curl_easy_cleanup(u->curl);
}


