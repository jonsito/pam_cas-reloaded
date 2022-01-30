//
// Created by jantonio on 30/1/22.
//
#include <string.h>

#include <security/pam_modules.h>

#define __DIT_UPM_C__
#include "dit_upm.h"

/**
 * extract user name from cas received data
 * @return user name or null on faillure
 */
static char *getUserName(char *cas_data[]) {
    return NULL;
}

/**
 * extract full name from cas received data
 * @return gecos or null on faillure
 */
static char *getGecos(char *cas_data[]) {
    return NULL;
}

/**
 * extract user ID from CAS data
 * make sure that uid is greater than 32768, to avoid collision with ldap users uid
 * @param cas_data
 * @return user id or -1 on error
 */
static int getUserID(char *cas_data[]) {
    return -1;
}

/**
 * extract primary group id from CAS received data
 * @param cas_data
 * @return primary group id ( student, teacher, staff, other ), or -1 on error
 */
static int getGroupID(char *cas_data[]) {
    return 0;
}

/**
 * extract alternate groups for given user
 * @param cas_data
 * @return
 */
static int *getGroups(char *cas_data[]) {
    static int groups[16];
    memset(groups,0,sizeof(groups));
    return groups;
}

/**
 * check if user belongs to ETSIT-UPM and is member of any allowed group members
 * @param cas_data
 * @return 1:allowed 0:notAllowed -1:error
 */
static int isAllowed(char *cas_data[]) {
    return 0;
}

/**
 * Check if user directory exists.
 *  if exists, return userdir current userid
 *  else create with provided userid
 *  return resulting home's userid
 * @param username name used to check /home/<username>
 * @param uid evaluated user id from CAS server
 * @param gid evaluated group id from CAS server
 * @return home's userID, -1:error
 */
static int checkAndCreateHome(char *username, int uid, int gid) {
    return uid;
}

/**
 * After user/pass auth success, check if user meets requirements to login into Dit-UPM students labs
 * @param args results from CAS server auth request
 * @return 1:success, 0:faillure, -1:error
 */
int ditupm_check(char *args[]) {
    // first of all check membresy
    int res=isAllowed(args);
    if (res!=1) return res;
    // extract user name, gecos, userid, gid and secondary groups
    char *username= getUserName(args);
    if (!username) return -1;
    char *gecos= getGecos(args);
    if (!gecos) return -1;
    int userid= getUserID(args);
    if (userid<=0) return -1;
    int groupid= getGroupID(args);
    if (groupid<=0) return -1;
    int *groups= getGroups(args);
    if (!groups[0]) return -1;
    // now check for home
    res=checkAndCreateHome(username, userid,groupid);
    if (res<0) return -1;
    userid=res;
    // populate pam structs with evaluated data
    // pending
    // and return success
    return PAM_SUCCESS;
}

/**
 * get received parameters from CAS login request, and prepare it to be parsed
 * @return parsed data
 */
char **eval_receivedCASData(){
    return NULL;
}


/* end of file */
#undef __DIT_UPM_C__