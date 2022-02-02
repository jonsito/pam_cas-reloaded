#include "cas.h"
#include "dit_upm.h"

#define DEBUG 1
#define DEBUG_CONTENT 1

int CAS_init(struct CAS *c, char *CAS_URL, char *service, char *callback) {
	strcpy(c->CAS_URL, CAS_URL);
	URL_init(&c->u);	
	c->service = service;
	c->service_callback = callback;
	return 1;
}

/* This is one nasty function to parse HTML/XML code to with, and the best part is: no need for a complex parser!
  */
int CAS_find_part(struct string *s, char *startmatch, char endchar, char *dest, int dstsize) {
	char *ptr = NULL;
	char *sp = NULL;
	char *ep = NULL;
	ptr = strstr(s->ptr, startmatch);
	if (ptr) {
		sp = ptr + strlen(startmatch);
		ep = strchr(sp, endchar);
		if ((ep-sp) < dstsize) {
			*dest = '\0';
			strncat(dest, sp, ep-sp);
			return 1;
		}
		return -1;
	}
	return 0;
}

int CAS_find_loginticket(struct string *s, char *ticket, int size) {
	return CAS_find_part(s, "name=\"lt\" value=\"" , '"', ticket, size);
}

int CAS_find_serviceticket(struct string *s, char *ticket, int size) {
	return CAS_find_part(s, "CASTGC=", ';', ticket, size);
}

int CAS_find_user(struct string *s, char *user, int size) {
	return CAS_find_part(s, "<cas:user>", '<', user, size);
}

int CAS_find_session(struct string *s, char *session, int size) {
	return CAS_find_part(s, "login;jsessionid=" , '\"', session, size);
}


int CAS_find_execution(struct string *s, char *execution, int size) {
	return CAS_find_part(s, " name=\"execution\" value=\"" , '\"', execution, size);
}

int CAS_find_pgt(struct string *s, char *pt, int size) {
	return CAS_find_part(s, "<cas:proxyTicket>", '<', pt, size);
}

int CAS_login(struct CAS *c, char *uname, char *pass) {
	char URL[1000];
	char lt[1024]; // login ticket
    char st[1024]; // service ticket
	char sess[2048]; // session ticket
	char execution[2048]; // siu.upm.es returns about "1500" bytes on execution parameter
	int ret = 0;
	//int ret2 = 0;
	struct string content;
	init_string(&content);
	
	if ( (c->service!=NULL) || (strlen(c->service)>=0))
		sprintf(URL, "%s/login?service=%s", c->CAS_URL, c->service);
	else
		sprintf(URL, "%s/login", c->CAS_URL);	

	URL_GET_request(&c->u, URL, &content);
    log_content(LOG_DEBUG, "get done, return from get: %s\n", content.ptr);

    // cas protocol states that login ticket is optional
    // so try to extract from server data.
    // if no login ticket provided generate unique one
	ret = CAS_find_loginticket(&content, lt, sizeof(lt));
    if (!ret) ret = ditupm_generateLoginTicket(uname, lt, sizeof(lt));
	CAS_find_session(&content, sess, sizeof(sess));
	CAS_find_execution(&content, execution, sizeof(execution));

	log_msg(LOG_INFO, "got session: %s\n", sess);
	log_msg(LOG_INFO, "got execution: %s\n", execution);

	free(content.ptr);
	content.len = 0;

	if (!ret) {
        log_msg(LOG_INFO, "Could not get login ticket!\n");
		return -1;
    }
	log_msg(LOG_INFO, "LoginTicket: %s\n", lt);
	log_msg(LOG_INFO, "Using service: %s\n", c->service);
	init_string(&content);

    /*
     * check this that comes from github original code. seems to be errorneous
    if ( (c->service!=NULL) || (strlen(c->service)>0))
        sprintf(URL, "%s;jsessionid=%s&service=%s", URL, sess,c->service);
    else
		sprintf(URL, "%s;jsessionid=%s", URL, sess);
     */

	URL_add_form(&c->u, "username", uname);
	URL_add_form(&c->u, "password", pass);
	URL_add_form(&c->u, "lt", lt);
	URL_add_form(&c->u, "execution", execution);
	URL_add_form(&c->u, "_eventId", "submit");
    // do not submit "submit button :-)
    // URL_add_form(&c->u, "submit", "LogIn");
    URL_add_form(&c->u, "geolocation", "");


	if ( (c->service) && (strlen(c->service)>0))
		URL_add_form(&c->u, "service", c->service);

	log_msg(LOG_INFO, "Sending post for user/pass authentication: %s\n", uname);
	log_msg(LOG_INFO, "with lt: %s\n", lt);
	URL_POST_request(&c->u, URL, &content);
	log_content(LOG_DEBUG, "post done, return from post: %s\n", content.ptr);

    // When no service is provided, siu.upm.es does not provide
    // neither service ticket nor redirection to service server.
    // so this call may fail
    if((c->service) && (strlen(c->service)>0) ) {
        memset(st,0,sizeof(st));
        ret = CAS_find_serviceticket(&content, st, sizeof(st));
        if (!ret) {
            log_msg(LOG_INFO, "Could not get service ticket!\n");
            return -2;
        }
    } else {
        // when no service provided, siu.upm.es just shows a page with auth info. So parse it
        ditupm_parseReceivedData(&content);
        ret=ditupm_check(lt);
        if (ret<0) {
            log_msg(LOG_INFO, "Dit-UPM checkings failed\n");
            return ret;
        }
    }
    // clean up
    free(content.ptr);
    content.len = 0;
	log_msg(LOG_INFO, "Successfully logged in!\n");
    // if service provided, use received service ticket to validate service
    // and parse received xml to retrieve ditupm atributes
    if (strlen(lt)!=0) {
        ret = CAS_serviceValidate(c, lt, uname);
    }

	return ret;
}

int CAS_serviceValidate(struct CAS *c, char *ticket, char *u) {
	char URL[1000];
	char user[512];
	int ret = 0;
    struct string content;
    init_string(&content);

	if ( (c->service != NULL) && (strlen(c->service))>0 )
		sprintf(URL, "%s/serviceValidate?service=%s&ticket=%s&pgtUrl=%s", c->CAS_URL, c->service, ticket, c->service_callback);
	else
		sprintf(URL, "%s/serviceValidate?ticket=%s", c->CAS_URL, ticket);
    URL_GET_request(&c->u, URL, &content);
	log_content(LOG_DEBUG, "serviceValidate: %s", content.ptr);


	ret = CAS_find_user(&content, user, 512);
	if (ret) {
		if (u != NULL) { // User comparison, strictly speaking not needed, but better be safe than sorry
			if (strncmp(user, u, strlen(user)) == 0)
				ret = 1;
			else
				ret = 0;
		} else 
			ret = 2;
	} 
	free(content.ptr);
	return ret;
}

int CAS_proxy(struct CAS *c, char *pgt, char *u) {
	char URL[1000];
	char pt[512];
	int ret = 0;
	struct string content;
	init_string(&content);

	sprintf(URL, "%s/proxy?targetService=%s&pgt=%s", c->CAS_URL, c->service, pgt);
	
	URL_GET_request(&c->u, URL, &content);
	log_content(LOG_DEBUG, "PGT: %s", content.ptr);
	ret = CAS_find_pgt(&content, pt, 512);
	log_msg(LOG_INFO, "ProxyTicket: %s", pt);

	if (ret) {
		if (pt != NULL) {
			ret = CAS_proxyValidate(c, pt, u);
		} else 
			ret = 2;
	}
	free(content.ptr);
	return ret;
}

int CAS_proxyValidate(struct CAS *c, char *ticket, char *u) {
        char URL[1000];
        char user[512];
        int ret = 0;
        struct string content;
        init_string(&content);

        if (c->service != NULL)
                sprintf(URL, "%s/proxyValidate?service=%s&ticket=%s", c->CAS_URL, c->service, ticket);
        else
                sprintf(URL, "%s/proxyValidate?ticket=%s", c->CAS_URL, ticket);

        URL_GET_request(&c->u, URL, &content);
	    log_content(LOG_DEBUG, "proxyValidate: %s", content.ptr);

        ret = CAS_find_user(&content, user, 512);
        if (ret) {
                if (u != NULL) {
                        if (strncmp(user, u, strlen(user)) == 0)
                                ret = 1;
                        else
                                ret = 0;
                } else
                	ret = 2;
        }
        free(content.ptr);
        return ret;
}

int CAS_cleanup(struct CAS *c) {
	URL_cleanup(&c->u);	
	return 1;
}


