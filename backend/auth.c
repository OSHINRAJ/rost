/*
 *  CVS Version: $Id: auth.c,v 1.27 2014/01/09 04:32:16 benny Exp $
 *
 *  Copyright (C) 2009-2014 Olof Hagsand and Benny Holmgren
 *
 *  This file is part of ROST.
 *
 *  ROST is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ROST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along wth ROST; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifdef HAVE_ROST_CONFIG_H
#include "rost_config.h" /* generated by config & autoconf */
#endif /* HAVE_ROST_CONFIG_H */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pwd.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_backend.h>

/* lib */
//#include "system.h"
#include "auth.h"

/* Calback declarations */
static int auth_login_user_uid(clicon_handle, char *, lv_op_t, char *, void *);
static int auth_login_user_auth_passwd(clicon_handle, char *, lv_op_t, char *, void *);
static int auth_login_user_class(clicon_handle, char *, lv_op_t, char *, void *);

/* emulator variable */
static int rost_emulator = 0;

/* Map of key-callback pairs for system */
struct {
    char *key;
    trans_cb cb;
} auth_depmap[] = {

    {"system.login.user[].uid", auth_login_user_uid},
    {"system.login.user[].authentication.password", auth_login_user_auth_passwd},
    {"system.login.user[].class", auth_login_user_class},

    {NULL, NULL},
};

/*
 * Plugin initialization
 */
int
plugin_init(clicon_handle h)
{
    int i;
    char *key;
    int retval = -1;

    for (i = 0; auth_depmap[i].key != NULL; i++) {
	key = auth_depmap[i].key;
	if (dbdep(h, 0, auth_depmap[i].cb, (void *)NULL, key) == NULL){
	    clicon_debug(1, "Failed to create dependency '%s'", key);
	    goto done;
	}
	clicon_debug(1, "Created dependency '%s'", key);
    }	
    if (clicon_option_exists(h, "ROST_EMULATOR"))
	rost_emulator = clicon_option_int(h, "ROST_EMULATOR");
    retval = 0;
  done:
    return retval;
}

/*
 * Plugin start
 */
int
plugin_start(clicon_handle h, int argc, char **argv)
{
    return 0;
}

/*
 * Reset system state
 */
int
plugin_reset(clicon_handle h, char *db)
{
#if 1
    int retval = -1;
    dbspec_key *dbspec = clicon_dbspec_key(h);

#if 0 /* Keep until we know admin works, then remove */
    if (db_lv_op(dbspec, db, LV_SET, "system.login.user[].class $!user=(string)\"root\" $class=(string)\"superuser\"", 0, NULL) < 0)
	goto done;
#endif

    if (db_lv_op(dbspec, db, LV_SET, "system.login.user[].uid $!user=(string)\"admin\" $uid=(int)1001", NULL) < 0)
	goto done;
    if (auth_login_user_uid(h, db,  LV_SET, 
			      "system.login.user.0.uid", NULL) < 0)
	goto done;

    if (db_lv_op(dbspec, db, LV_SET, "system.login.user[].class $!user=(string)\"admin\" $class=(string)\"superuser\"", NULL) < 0)
	goto done;
    if (auth_login_user_class(h, db, LV_SET, 
			      "system.login.user.0.class", NULL) < 0)
	goto done;

    
#if 0
    if (db_lv_op(dbspec, db, LV_SET, "system.login.user[].authentication.password $!user=(string)\"admin\" $password=(string)\"\"", NULL) < 0)
	goto done;
#endif

    retval = 0;
done:

    return retval;
#else /* notyet */
    return 0;
#endif
}


/*
 * commit_hooks
 * Database may be inconsistent after a commit
 * 1. Need uid for all users: add existing uid if it does not exist.
 */
int
transaction_end(clicon_handle h)
{
#if 0  /* XXXX Is this still needed after the db-key changes??? */
    int   n, npairs;
    struct db_pair *pairs;
    char *mkey;
    char *user;
    char *e;
    int offset;
    int uid;
    struct passwd *pw;
    char *key;;
    char *lvec;
    size_t len;
    int retval = -1;
    
    offset = strlen("system.login.user.");
    if ((mkey = chunk_sprintf(label, "^system.login.user%s", "")) == NULL) {
	clicon_err(OE_DB, errno, "chunk_sprintf");
	goto done;
    }
    /* Get all keys/values for vector */
    npairs = db_regexp(dbname, mkey, label, &pairs, 0);
    if (npairs < 0)
        return -1;
    for (n=0; n<npairs; n++) {
	if (strlen(pairs[n].dp_key) < offset)
	    continue;
	if ((user = chunk_sprintf(label, "%s", pairs[n].dp_key + offset)) == NULL){
	    clicon_err(OE_DB, errno, "chunk_sprintf");
	    goto done;
	}
	if ((e = strchr(user, '.')) != NULL)
	    *e = '\0';
	if ((key = chunk_sprintf(label,
				 "system.login.user.%s.uid", user)) == NULL){
	    clicon_err(OE_DB, errno, "chunk_sprintf");
	    goto done;
	}
	if (db_get_alloc(dbname, key, 
			 (void**)&lvec, &len) < 0)
	    return -1;
	if (lvec)
	    free(lvec);
	if (len==0){ /* means not found */
	    uid = -1;
	    if ((pw = getpwnam(user)) != NULL)
		uid = pw->pw_uid;
	    endpwent();
	    if (uid>0){
		if ((key = chunk_sprintf(label,
					 "system.login.user.%s.uid $user=(string)\"%s\" $uid=(int)%d", 
					 user, user, uid)) == NULL){
		    clicon_err(OE_DB, errno, "chunk_sprintf");
		    goto done;
		}
		db_lv_op(dbname, LV_SET, key, NULL);
	    }
	}
	user=NULL;
    }
    retval = 0;
done:
    return retval;
#else
    return 0;
#endif
}


static const char *
gid_int2str(enum rost_gid gid)
{
    switch (gid){
    case SHOW_GID:
	return "show";
    case PING_GID:
	return "ping";
    case ADMIN_GID:
	return "admin";
    case QUAGGA_GID:
	return "quagga";
    case WHEEL_GID:
	return "wheel";
    }
    return "null";
}

const enum rost_gid
gid_str2int(char *name)
{
    if (strcmp(name, "show") == 0)
	return SHOW_GID;
    else
	if (strcmp(name, "ping") == 0)
	    return PING_GID;
	else
	    if (strcmp(name, "admin") == 0)
		return ADMIN_GID;
	    else
		if (strcmp(name, "quagga") == 0)
		    return QUAGGA_GID;
		else
		    if (strcmp(name, "wheel") == 0)
			return WHEEL_GID;
    return 0;
}



/*
 * auth_user_exist
 * Return 1 if exists and set uid, if not exists (or error) return 0.
 */
static int
auth_user_exist(char *user, int *uid)
{
    struct passwd *pw;
    int retval = 0;

    if ((pw = getpwnam(user)) != NULL){
	if (uid)
	    *uid = pw->pw_uid;
	retval = 1;
    }
    endpwent();
    return retval;
}

/*
 * auth_user_add
 * Add user to passwd file using shadow
 * If uid<0 generate uid
 * Return -1 on error, 0 on success.
 */
static int 
auth_user_add(char *user, int uid)
{
    char cmd[MAXNAMLEN];
    int retval = -1;

    if (uid >= 0)
	snprintf(cmd, sizeof(cmd), 
		 "%s -g users -m -d %s/%s -s %s -u %d %s", 
		 "/usr/sbin/useradd",
		 ROST_USERS_DIR, user,
		 ROST_BIN_DIR "/clicon_cli",
		 uid,
		 user);
    else
	snprintf(cmd, sizeof(cmd), 
		 "%s -g users -m -d %s/%s -s %s %s", 
		 "/usr/sbin/useradd",
		 ROST_USERS_DIR, user,
		 ROST_BIN_DIR "/clicon_cli",
		 user);
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: cmd: %s",
	       __FUNCTION__,  cmd);
    if (rost_emulator)
	return 0;
    if ((retval = system(cmd)) < 0){
	clicon_err(OE_UNIX, errno, "useradd");
	goto quit;
    }
    if (retval != 0){
	clicon_err(OE_UNIX, 0, "useradd code=%d", retval);
	goto quit;
    }
    retval = 0;
quit:
    return retval;
}

/*
 * auth_user_delete
 * Remove user from passwd file using shadow
 * Cowardly refuse to rm root
 */
static int 
auth_user_delete(char *user)
{
    char cmd[MAXNAMLEN];
    int retval = -1;

    if (strcmp(user, "root") == 0)
	return 0; /* Cowardly refuse to rm root */
    snprintf(cmd, sizeof(cmd), 
	     "%s -r %s", 
	     "/usr/sbin/userdel",
	     user);
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: cmd: %s",
	       __FUNCTION__,  cmd);
    if (rost_emulator)
	return 0;
    if ((retval = system(cmd)) < 0){
	clicon_err(OE_UNIX, errno, "userdel");
	goto quit;
    }
    if (retval != 0){
	clicon_err(OE_UNIX, 0, "userdel code=%d", retval);
	goto quit;
    }
    retval = 0;
quit:
    return retval;
}

/*
 * auth_user_passwd
 * Set user passwd. Supply encrypted passwd.
 */
static int 
auth_user_passwd(char *user, char *passwd)
{
    char cmd[MAXNAMLEN];
    int retval = -1;

    snprintf(cmd, sizeof(cmd), 
	     "%s -p '%s' %s", 
	     "/usr/sbin/usermod",
	     passwd,
	     user);
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: cmd: %s",
	       __FUNCTION__,  cmd);
    if (rost_emulator)
	return 0;
    if ((retval = system(cmd)) < 0){
	clicon_err(OE_UNIX, errno, "user_passwd: usermod");
	goto quit;
    }
    if (retval != 0){
	clicon_err(OE_UNIX, 0, "user_passwd: usermod code=%d", retval);
	goto quit;
    }
    retval = 0;
quit:
    return retval;
}

/*
 * auth_user_group_add
 * Add user group (not login). 
 * If append = 0, replace secondary group list, if append = 1, append it.
 */
static int
auth_user_group_mod(char *user, int gid, int append)
{
    char cmd[MAXNAMLEN];
    int retval = -1;
    const char *gname;

    if (gid < 0)
	snprintf(cmd, sizeof(cmd), 
		 "/usr/sbin/usermod %s -G \"\" %s", 
		 append?"-a":"",
		 user);
    else{
	gname = gid_int2str(gid);
	snprintf(cmd, sizeof(cmd), 
		 "/usr/sbin/usermod %s -G %s %s", 
		 append?"-a":"",
		 gname,
		 user);
    }
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: cmd: %s",
	       __FUNCTION__,  cmd);
    if (rost_emulator)
	return 0;
    if ((retval = system(cmd)) < 0){
	clicon_err(OE_UNIX, errno, "group_append: usermod");
	goto quit;
    }
    if (retval != 0){
	clicon_err(OE_UNIX, 0, "group_append: usermod code=%d", retval);
	goto quit;
    }
    retval = 0;
quit:
    return retval;
}



/*
 * User UID commit callback
 */
static int
auth_login_user_uid(clicon_handle h, char *db, 
		    lv_op_t op, char *key,  void *arg)
{
    cg_var *user = NULL;
    cg_var *uid = NULL;
    int retval = -1;

    uid = dbvar2cv (db, key, "uid");
    if (uid == NULL) 
	goto catch;
    
    user = dbvar2cv (db, key, "user");
    if (user == NULL) 
	goto catch;
    
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: user %s uid %d",
	       __FUNCTION__,  cv_string_get(user), cv_int_get(uid));
    if (!rost_emulator) {
	if (op == LV_SET) {
	    if (auth_user_exist(cv_string_get(user), NULL) == 0) {
		if (auth_user_add(cv_string_get(user), cv_int_get(uid)) < 0)
		    goto catch;
	    }
	} else if (op == LV_DELETE) {
	    if (auth_user_exist(cv_string_get(user), NULL)) {
		if (auth_user_delete(cv_string_get(user)) < 0)
		    goto catch;
	    }	    
	}
    }
    retval = 0;
    
catch:
    if (uid) 
	cv_free(uid);
    if (user) 
	cv_free(user);

    return retval;
}


/*
 * User password commit callback
 */
static int
auth_login_user_auth_passwd(clicon_handle h, char *db,
			    lv_op_t op, char *key,  void *arg)
{
    int retval = -1;
    cg_var *user = NULL;
    cg_var *passwd = NULL;

    user = dbvar2cv (db, key, "user");
    if (user == NULL) 
	goto catch;
    
    if (op == LV_SET) {
	passwd = dbvar2cv (db, key, "password");
	if (passwd == NULL) 
	    goto catch;
    }	
    
    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: user %s %s",
	       __FUNCTION__, cv_string_get(user),
	       ((op==LV_SET) ? cv_string_get(passwd) : "")
	);

    if (!rost_emulator) {
	if (auth_user_exist(cv_string_get(user), NULL) == 0) {
	    if (auth_user_add(cv_string_get(user), -1) < 0)
		goto catch;
	}
	if(auth_user_passwd(cv_string_get(user),
			    (op==LV_SET) ? cv_string_get(passwd) : "") < 0)
	    goto catch;
    }
    retval = 0;

catch:
    if (user) 
	cv_free(user);
    if (passwd) 
	cv_free(passwd);


    return retval;
}

/*
 * User UID commit callback
 */
static int
auth_login_user_class(clicon_handle h, char *db, 
		      lv_op_t op, char *key,  void *arg)
{
    int retval = -1;
    cg_var *user = NULL;
    cg_var *class = NULL;

    user = dbvar2cv (db, key, "user");
    if (user == NULL) 
	goto catch;
    
    if (op == LV_SET) {
	class = dbvar2cv (db, key, "class");
	if (class == NULL) 
	    goto catch;
    }	

    clicon_log(rost_emulator?LOG_NOTICE:LOG_DEBUG, "%s: user %s class %s",
	       __FUNCTION__,  cv_string_get(user), 
	       class ? cv_string_get(class) : "none");
    if (rost_emulator) {
	retval = 0;
	goto catch;
    }
    
    if (auth_user_exist(cv_string_get(user), NULL) == 0){
	if (auth_user_add(cv_string_get(user), -1) < 0)
	    goto catch;
    }
    
    /* Clear all groups. */
    if (auth_user_group_mod(cv_string_get(user), -1, 0) < 0)
	goto catch;
    
    /* If DEL, we're done */
    if (op == LV_DELETE) {
	retval = 0;
	goto catch;
    }

    if (strcmp(cv_string_get(class), "none") == 0) {
	/* Do nothing */ ;
    }	
    else if (strcmp(cv_string_get(class), "readonly") == 0) {
	if (auth_user_group_mod(cv_string_get(user), SHOW_GID, 1) < 0)
	    goto catch;
	if (auth_user_group_mod(cv_string_get(user), QUAGGA_GID, 1) < 0) /* XXX: See bug 8 */
	    goto catch;
    }	
    else if (strcmp(cv_string_get(class), "superuser") == 0){
	if (auth_user_group_mod(cv_string_get(user), SHOW_GID, 1) < 0)
	    goto catch;
	if (auth_user_group_mod(cv_string_get(user), PING_GID, 1) < 0)
	    goto catch;
	if (auth_user_group_mod(cv_string_get(user), ADMIN_GID, 1) < 0)
	    goto catch;
	if (auth_user_group_mod(cv_string_get(user), QUAGGA_GID, 1) < 0) 
	    goto catch;
	if (auth_user_group_mod(cv_string_get(user), WHEEL_GID, 1) < 0) 
	    goto catch;
    }
    else{
	clicon_err(OE_UNIX, 0, "group_append: No such class: %s", cv_string_get(class));
	goto catch;
    }

    retval = 0; 

catch:
    if (user) 
	cv_free(user);
    if (class) 
	cv_free(class);
    return retval;
}
