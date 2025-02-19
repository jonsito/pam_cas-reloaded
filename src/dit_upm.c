//
// Created by jantonio on 30/1/22.
//
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <openssl/md5.h>
#include <security/pam_modules.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "config.h"
#include "cas.h"
#include "url.h"
#define __DIT_UPM_C__
#include "dit_upm.h"

#define DEBUG 1
#define DEBUG_CONTENT 1

struct siu_casData {
    char item[128];
    char value[256];
};

static struct siu_casData casItems[]= {
        {"cn",""}, // gecos
        {"displayName",""},
        {"eduPersonAffiliation",""}, // groups names
        {"eduPersonPrimaryAffiliation",""}, // primary group
        {"eduPersonUniqueId",""},
        {"employeeType",""}, // Estudiante, Profesor, Laboral, ...
        {"givenName",""}, // nombre
        {"mail",""}, // correo
        {"o",""}, // organizacion
        {"schacGender",""},
        {"schacHomeOrganization",""},
        {"schacHomeOrganizationType",""},
        {"schacPersonalUniqueID",""},
        {"schacSn1",""}, // apellido1
        {"schacSn2",""}, // apellido2
        {"sn",""}, // apellidos
        {"uid",""}, // login
        {"upmCentre",""}, // centros de adscripcion [09: etsit]
        {"upmClassifCode",""}, // tipos de adscripcion
        {"upmPersonalUniqueID",""}, //DNI
        { "",""}
};

static int ditupm_find_value(struct string *s,int index) {
    char *ptr = NULL;
    char *sp = NULL;
    char *ep = NULL;
    char key[128];
    snprintf(key,128,"<td><kbd><span>%s",casItems[index].item);
    ptr = strstr(s->ptr, key);
    memset(casItems[index].value,0,sizeof(casItems[index].value));
    if (ptr) {
        sp=strstr(ptr,"<td><code><span>[");
        ep=strstr(ptr,"]</span></code></td>");
        strncpy(casItems[index].value, sp+17, ep-(sp+17));
        return 1; // found
    }
    return 0; // not found
}

/**
 * extract user name from cas received data
 * @return user name or null on faillure
 */
static char *getUserName() { return casItems[16].value; }

/**
 * extract full name from cas received data
 * @return gecos or null on faillure
 */
static char *getGecos() { return casItems[0].value; }

/**
 * extract user ID from CAS data (upmPersonalUniqueID : DNI )
 * make sure that uid is greater than 32768, to avoid collision with ldap users uid
 * @return user id or -1 on error
 */
static int getUserID() {
    int uid=0;
    char *pt=casItems[19].value; // upmPersonalUniqueID
    for(;*pt;pt++) {
        if (!isdigit(*pt)) continue;
        uid = (int) strtol(pt,NULL,10);
        break;
    }
    if (uid==0) return -1; // cannot extract uid from DNI. mark error
    if (uid<32768) { // DNI too low. little dirty trick to avoid collision with ldap uid values (16384-32767)
        uid+=32768; // expect do not collide :-(
    }
    return uid;
}

/**
 * extract primary group id from CAS received data
 * @param cas_data
 * @return primary group id student(1000), teacher(1001), staff(1002) , or -1 on error
 */
static int getGroupID() {
    char *group=casItems[3].value; // eduPersonPrimaryAffiliation
    if (strncmp("student",group,256)==0) return 1000;
    if (strncmp("faculty",group,256)==0) return 1001;
    if (strncmp("staff",group,256)==0) return 1002;
    // else group not allowed
    return -1;
}

/**
 * extract alternate groups for given user
 * @return
 */
static char *getGroups() {  return casItems[2].value; }

/**
 * check if user belongs to ETSIT-UPM and is member of any allowed group members
 * @param cas_data
 * @return 0:allowed -1: not allowed
 */
static int isAllowed(CAS_configuration *c) {
    if (!strstr(casItems[17].value,c->upmCentre)) return -1; // upmCentre: 09:ETSIT
    // iteramos entre los valores employeeType recibidos, y comparamos con los permitidos en la configuracion
    for (char *pt=casItems[5].value;*pt;pt++) {
        if (*pt==',') continue;
        if (strchr(c->employeeType,*pt)) return 0;
    }
    return -1; // arriving here means employeeType not allowed
}

/**
 * Check if user directory exists.
 *  if exists, return userdir current userid
 *  else create with provided userid
 *  return resulting home's userid
 * @param userData struct passwd that contains evaluated user CAS data
 * @return home's userID, -1:error
 */
static int checkAndCreateHome(struct passwd * userData) {
    char cmdline[1024];
    struct stat st;
    int res=stat(userData->pw_dir,&st);
    if (res>=0) {
        // la carpeta existe: ajustamos datos de uid y gid
        userData->pw_uid=st.st_uid;
        userData->pw_gid=st.st_gid;
        return 0; // success
    }
    // el home del usuario no existe. lo creamos con los datos recibidos
    res=mkdir(userData->pw_dir,0755);
    if (res<0) return -1; // cannot create home directory
    snprintf(cmdline,sizeof(cmdline),"cp -r /etc/skel/* %s",userData->pw_dir);
    res=system(cmdline);
    if (res<0) return -1; // cannot create skeleton directory structure
    snprintf(cmdline,sizeof(cmdline),"chown -R %d.%d %s",userData->pw_uid,userData->pw_gid,userData->pw_dir);
    res=system(cmdline);
    if (res<0) return -1; // cannot set uid/gid
    return (int) userData->pw_uid; // success
}

/**
 * CAS spec says that login ticket is optional, and may or not be generated by server.
 * So let's us create one when server does not provide us it
 * login is created as "LT-" plus md5 sum of username+time
 * @param user
 * @param pw
 * @param lt
 * @param size
 * @return 0:error 1:success
 */
int ditupm_generateLoginTicket(char *user, char *lt, size_t size) {
    MD5_CTX c;
    char buff[512];
    unsigned char md5[MD5_DIGEST_LENGTH];
    memset(buff,0,sizeof(buff));
    memset(lt,0,size);
    snprintf(buff,sizeof(buff),"%s-%ld",user,time(NULL));
    MD5_Init(&c);
    MD5_Update(&c, buff, strlen(buff));
    MD5_Final(md5, &c);
    sprintf(lt,"LT-");
    for( int n=0;n<MD5_DIGEST_LENGTH;n++) {
        sprintf(lt+strlen(lt),"%02X",md5[n]);
    }
    log_msg(LOG_DEBUG, "evaluated login ticket is: %s\n", lt);
    return 1;
}

/**
 * After user/pass auth success, check if user meets requirements to login into Dit-UPM students labs
 * @return 1:success, 0:faillure, -1:error
 */
int ditupm_check(char *loginTicket) {
    CAS_configuration c;
    load_config(&c,PAM_CAS_CONFIGFILE);
    struct passwd userData;
    // first of all check membresy
    int res=isAllowed(&c);
    if (res<0) return res;
    char *home=calloc(128,sizeof(char));
    if (!home) return -1; // malloc error
    // extract user name, gecos, userid, gid and secondary groups. on error, mark and return
    if (! (userData.pw_name = getUserName())) return -1;
    // store login ticket in password field, to translate to PAM structure
    userData.pw_passwd=strdup(loginTicket);
    snprintf(home,128,"/home/%s",userData.pw_name);
    if (! (userData.pw_gecos= getGecos())) return -1;
    if ((userData.pw_uid= getUserID())<=0) return -1;
    if ((userData.pw_gid= getGroupID())<=0) return -1;
    userData.pw_dir=home;
    userData.pw_shell="/bin/bash";
    if (!getGroups()) return -1; // if not in allowed groups
    // notice that checkAndCreateHome may change uid/gid if directory already exists
    if (checkAndCreateHome(&userData)<0) return -1; //home does not exists and cannot create
    // populate pam structs with evaluated data
    // pending
    fprintf(stderr,"Username: %s\n",userData.pw_name);
    fprintf(stderr,"Gecos: %s\n",userData.pw_gecos);
    fprintf(stderr,"UserID: %d\n",userData.pw_uid);
    fprintf(stderr,"GroupID: %d\n",userData.pw_gid);
    fprintf(stderr,"Username: %s\n",userData.pw_name);
    // and return success
    return 0;
}

/**
 * get received parameters from CAS login request, and prepare it to be parsed
 * @return parsed data
 */
int ditupm_parseReceivedData(struct string  *content){
    for (int n=0; strlen(casItems[n].item)!=0;n++) {
        ditupm_find_value(content,n);
        // fprintf(stderr,"%-28s - %s\n",casItems[n].item,casItems[n].value);
    }
    return 0;
}

/* end of file */
#undef __DIT_UPM_C__