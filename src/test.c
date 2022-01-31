#include <stdio.h>
#include <stdlib.h>

#include "cas.h"
#include "config.h"

int main(int argc, char **argv) {
	struct CAS cas;
	int ret = 0;
	CAS_configuration c;

    if (argc!=3) {
        fprintf(stderr,"Usage %s <user> <password>\n",argv[0]);
        exit(1);
    }

	ret = load_config(&c, PAM_CAS_CONFIGFILE);
	if (!ret) {
		printf("Failed to load configuration!\n");
		exit(0);
	}

	printf("Loaded config:\n");
	printf("BASE_URL = %s\n", c.CAS_BASE_URL);
	printf("SERVICE_URL = %s\n", c.SERVICE_URL);
	printf("Enable serviceValidate = %d\n", c.ENABLE_ST);
	printf("Enable proxyValidate = %d\n", c.ENABLE_PT);
	printf("Enable user+pass = %d\n", c.ENABLE_UP);


	// Initialize the CAS backend
	CAS_init(&cas, c.CAS_BASE_URL, c.SERVICE_URL, c.SERVICE_CALLBACK_URL);

	// Attempt a full login with user/pass
	ret = CAS_login(&cas, argv[1], argv[2]);

	// Ret > 1 means we are authenticated 
	if (ret > 0)
		printf("We are now authenticated!\n");
	else
		printf("Failed to authenticate!\n");
	// Clean up resources
	CAS_cleanup(&cas);

	return 0;
}
