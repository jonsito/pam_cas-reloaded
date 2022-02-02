/*

Author: Attila Sukosd <attila@cc.dtu.dk>
License: BSD

Changelog:

- 26/Jan/2012 v0.3 Configuration file support added
- 25/Jan/2012 v0.2 CAS full user+pass login and serviceTicket login implementation
- 25/Jan/2012 v0.1 Initial version

*/

// CONFIGURATION
#define MIN_TICKET_LEN 20

// Support authentication against CAS
#define PAM_SM_AUTH

#include <string.h>
#include <syslog.h>

#include <security/pam_modules.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

#include "url.h"
#include "cas.h"
#include "config.h"

int pam_sm_authenticate(pam_handle_t *pamhandle, int flags, int arg, const char **argv) {
	
	char *user, *pw;
    struct CAS cas;
	CAS_configuration c;
    int ret = 0;

	if (pam_get_user(pamhandle, (const char**)&user, NULL) != PAM_SUCCESS) {
		log_msg(LOG_ERR, "User does not exist!");
		return PAM_AUTH_ERR;
	}

	if (pam_get_item(pamhandle, PAM_OLDAUTHTOK, (const void**)&pw)  != PAM_SUCCESS) {
		log_msg(LOG_ERR, "Cannot get the password!");
		return PAM_AUTH_ERR;
	}

	if (pw == NULL) {
		if (pam_get_item(pamhandle, PAM_AUTHTOK, (const void**)&pw) != PAM_SUCCESS) {
			log_msg(LOG_ERR, "Cannot get  the password 2!");
			return PAM_AUTH_ERR;
		}
	}

	if (pw == NULL) {
        log_msg(LOG_ERR, "Did not get password, check the PAM configuration!");
		return PAM_AUTH_ERR;
	}

    //	log_msg(LOG_NOTICE, "Got user: %s pass: %s\n", user, pw);
    ret = load_config(&c, PAM_CAS_CONFIGFILE);
    if (!ret) {
        log_msg(LOG_ERR,  "Failed to load configuration at %s!", PAM_CAS_CONFIGFILE);
        return PAM_AUTH_ERR;
    }
    // JAMC 20220130 from cluck's fork security patch
    ret = -1; // mark error by default
	CAS_init(&cas, c.CAS_BASE_URL, c.SERVICE_URL, c.SERVICE_CALLBACK_URL);

	if (c.ENABLE_ST && strncmp(pw, "ST-", 3) == 0 && strlen(pw) > MIN_TICKET_LEN) { // Possibly serviceTicket?
    log_msg(LOG_INFO, "serviceTicket found. Doing serviceTicket validation!");
		ret = CAS_serviceValidate(&cas, pw, user);		
	} else if (c.ENABLE_PT && strncmp(pw, "PT-", 3) == 0 && strlen(pw) > MIN_TICKET_LEN) {
        // Possibly a proxyTicket?
        log_msg(LOG_INFO, "proxyTicket found. Doing proxyTicket validation!");
		ret = CAS_proxyValidate(&cas, pw, user);
	} else if (c.ENABLE_PT && strncmp(pw, "PGT-",4) == 0 && strlen(pw) > MIN_TICKET_LEN) {
        // Possibly a proxy granting ticket
		log_msg(LOG_INFO, "pgTicket found. Doing proxy-ing and proxyTicket validation!");
		ret = CAS_proxy(&cas, pw, user);
	} else if (c.ENABLE_UP) {
		log_msg(LOG_INFO, "user+pass combo login attemp");
        ret = CAS_login(&cas, user, pw);
	}

    CAS_cleanup(&cas);
	if (ret >= 0) {
        log_msg(LOG_INFO, "CAS user %s logged in successfully! ret: %d", user, ret);
        return PAM_SUCCESS;
    } else {
        log_msg(LOG_INFO, "Failed to authenticate CAS user %s. ret: %d", user, ret);
        return PAM_AUTH_ERR;
    }
}

int pam_sm_setcred(pam_handle_t *pamhandle, int flags, int argc, const char **argv) {
	return PAM_SUCCESS;
}

