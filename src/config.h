#ifndef _H_CONFIG_
#define _H_CONFIG_

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "ini.h"

typedef struct {
    // datos para comunicacion con CAS server
	char* SERVICE_URL;
	char* SERVICE_CALLBACK_URL;
	char* CAS_BASE_URL;
	int ENABLE_ST;
	int ENABLE_PT;
	int ENABLE_UP;
    // datos para control de acceso
    char* upmCentre;
    char* employeeType;
    char* eduPersonAffiliation;
} CAS_configuration;

int load_config(CAS_configuration *c, char *file);

#endif /* _H_CONFIG_ */
