/*
 *  CVS Version: $Id: interface.c,v 1.25 2014/01/09 04:32:16 benny Exp $
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
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/ethtool.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_backend.h>

#include "interface.h"

/*
 * Prototypes
 */
static int load_if_prefix(clicon_handle h, char *ifname);
static int set_if_ipv4defaults(clicon_handle h, char *ifname);

/*
 * Global variables
 */
static char *newif = NULL;

struct {
    char *key;
    char *procfmt;
} iface_flags[] = {
    {
	"interface[].unit[].ipv4.rp_filter",
	"/proc/sys/net/ipv4/conf/$name/rp_filter"
    },
    
    {
	"interface[].unit[].ipv4.send_redirects",
	"/proc/sys/net/ipv4/conf/$name/send_redirects"
    },

    {
	"interface[].unit[].ipv4.proxy_arp",
	"/proc/sys/net/ipv4/conf/$name/proxy_arp"
    },

    {
	NULL,
	NULL
    },
};

/*
 * Interface commit callback. 
 */
int
interface_commit(clicon_handle h, char *db, trans_cb_type tt, lv_op_t op, char *key, void *arg)
{
    cg_var *cgv;

    if (op != LV_SET)
	return 0;
    
    /* Get interface name, and store in global 'newif' variable
       for processing in postcommit callback */
    cgv = dbvar2cv (db, key, "name");
    if (cgv == NULL) {
	clicon_err(OE_DB, errno , "Failed get interface name from database");
	return -1;
    }
    newif = strdup(cv_string_get(cgv));
    cv_free(cgv);

    return 0;
}

/*
 * IPv4 interface flags commit callback. 
 */
int
interface_ipv4flg_commit(clicon_handle h,
			 char *db, 
			 trans_cb_type tt, 
			 lv_op_t op,
			 char *key,
			 void *arg)
{
    int retval = -1;
    char *path = NULL;
    cg_var *status = NULL;
    FILE *f;

    if ((path = lvmap_fmt(db, (char *)arg, key)) == NULL)
	goto catch;

    if ((f = fopen(path, "w")) == NULL) {
	clicon_err(OE_CFG, errno, "%s: fopen(%s)", path, __FUNCTION__);
	goto catch;
    }

    if ((status = dbvar2cv (db, key, "status")) == NULL)
	goto catch;
    
    fprintf(f, "%d", cv_int_get(status));
    retval = 0;
    
catch:
    if (f)
	fclose(f);
    if (status) 
	cv_reset(status);

    if (path)
	free(path);
    return retval;
}



/*
 * Plugin initialization
 */
int
plugin_init(clicon_handle h)
{
    int i;
    char *key;
    int retval = -1;

    key = "interface[].unit[]";
    if (dbdep(h, TRANS_CB_COMMIT, interface_commit,
	      (void *)NULL, 1, key) == NULL) {
	clicon_debug(1, "interface: Failed to create dependency '%s'", key);
	goto done;
    }
    clicon_debug(1, "interface: Created dependency '%s'", key);
    for (i = 0; iface_flags[i].key; i++) {
	key = iface_flags[i].key;
	if (dbdep(h, TRANS_CB_COMMIT, interface_ipv4flg_commit,
		  (void *)iface_flags[i].procfmt, 1, key) == NULL) {
	    clicon_debug(1, "Failed to create dependency '%s'", key);
	    goto done;
	}
	clicon_debug(1, "Created dependency '%s'", key);
    }
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
 * Plugin post-commit. If a new interface has been created, set interface
 * defaults.
 */
int
transaction_end(clicon_handle h)
{
    if (newif) {
	set_if_ipv4defaults(h, newif);
	free(newif);
	newif = NULL;
    }
    return 0;
	
}

/*
 * Reset system state
 * The system 'state' should be the same as the contents of running_db
 */
int
plugin_reset(clicon_handle h)
{
    int j, i, len;
    struct clicon_if *iflist;
    char *pfx[] = ROST_IFTYPES;
    
    for (j=0; pfx[j]; j++) {
	if (clicon_iflist_get(pfx[j], __FUNCTION__, &iflist, &len) < 0)
	    return -1;
	for (i=0; i<len; i++){
	    if (load_if_prefix(h, iflist[i].ci_name) < 0)
		return -1;
	    if (set_if_ipv4defaults(h, iflist[i].ci_name) < 0)
		return -1;
	}
    }

    return 0;
}


/*
 * Add keys in running db for known interfaces
 */
static int
load_if_prefix(clicon_handle h, char *ifname)
{
    int retval = -1;
    char *cmd;
    char *db = clicon_running_db(h);
    struct db_spec *dbspec = clicon_dbspec_key(h);

    if ((cmd = chunk_sprintf(__FUNCTION__, 
			     "interface[].unit[] $!name=(string)\"%s\" $!unit=(int)0", 
			     ifname)) == NULL){
	clicon_err(OE_UNIX, errno, "chunk");
	goto err;
    }
    if (db_lv_op(dbspec, db, LV_SET, cmd, NULL) < 0)
	goto err;

    /* Set interface status to up. 
     * XXX Should we? Or should we check actual status and register in database
     */
    cmd = chunk_sprintf(__FUNCTION__, "ip link set dev %s up", ifname);
    if (cmd == NULL){
	clicon_err(OE_UNIX, errno, "chunk");
	goto err;
    }
    clicon_proc_run(cmd, NULL, 0);
    
    retval = 0;
err:
    unchunk_group(__FUNCTION__);
    return retval;
}

static int
set_if_ipv4defaults(clicon_handle h, char *ifname)
{
    int retval = -1;
    char *procpath, *dbkey;
    int i;
    FILE *f;
    char *db = clicon_running_db(h);
    struct db_spec *dbspec = clicon_dbspec_key(h);
    struct {
	char *key;
	int value;
    } defaults[] = {	/* XXX Should be macros from a header file somewhere */
	{ "proxy_arp", 1 },
	{ "send_redirects", 1 },
	{ "rp_filter", 0 },
	{ "accept_source_route", 1 },
	{ "accept_redirects", 1 },
	{ "forwarding", 1 },
	{ "log_martians", 0 },
/*	{ "mc_forwarding", 1 },*/
	{ "secure_redirects", 1 },
	{ "shared_media", 1 },
    };
    const char *procfmt = "/proc/sys/net/ipv4/conf/%s/%s"; 
	 
    for (i=0; i < sizeof(defaults)/sizeof(*defaults); i++) {
	procpath = chunk_sprintf(__FUNCTION__, procfmt,
				 ifname, defaults[i].key);
	dbkey = chunk_sprintf(__FUNCTION__, 
			      "interface[].unit[].ipv4.%s $!name=(string)\"%s\" $!unit=(int)0 $status=(int)%d", 
			      defaults[i].key,
			      ifname,
			      defaults[i].value);
	if (procpath == NULL || dbkey == NULL) {
	    clicon_err(OE_UNIX, errno, "chunk");
	    goto err;
	}
	
	/* Write to /proc */
	if ((f = fopen(procpath, "w")) == NULL) {
	    clicon_err(OE_UNIX, errno, "fopen(%s)", procpath);
	    goto err;
	}
	fputs((defaults[i].value ? "1" : "0"), f);
	fclose(f);
	unchunk(procpath);
	
	/* Set DB valie */
	if (db_lv_op(dbspec, db, LV_SET, dbkey, NULL) < 0)
	    goto err;
	unchunk(dbkey);
    }

    retval = 0;
err:
    unchunk_group(__FUNCTION__);
    return retval;
}
