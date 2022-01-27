#ifndef _H_CAS_H_
#define _H_CAS_H_

#include <syslog.h>
#include "url.h"

#ifndef LOG_MSG
// Lets do some syslogging
#define LOG_MSG(DEST, FORMAT, ...) \
	syslog(DEST, "%s:%d: " FORMAT, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

struct CAS {
	char CAS_URL[500];
	struct URL_Request u;
	char *service;
	char *service_callback;
};

int CAS_init(struct CAS *c, char *CAS_URL, char *service, char *callback);
int CAS_login(struct CAS *c, char *uname, char *pass);
int CAS_serviceValidate(struct CAS *c, char *ticket, char *uname);
int CAS_proxy(struct CAS *c, char *ticket, char *uname);
int CAS_proxyValidate(struct CAS *c, char *ticket, char *uname);
int CAS_cleanup(struct CAS *c);

#endif /* _H_CAS_H_ */
