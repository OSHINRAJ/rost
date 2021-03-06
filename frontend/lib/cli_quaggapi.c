/*
 *  CVS Version: $Id: cli_quaggapi.c,v 1.7 2013/08/09 13:27:46 olof Exp $
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_cli.h>

/* lib */
#include "cli_quaggapi.h"
#include "quaggapi.h"
#include "system.h"

/*
 *
 * CLI supports functions
 *
 */
static int                               
cli_quagga_exec(clicon_handle h, void *arg, cvec *vr, char *sockpath)
{
    int retcode;
    char *cmd = NULL;
    char *output = NULL;
    struct quaggapi_batch *batch = NULL;
    char path[PATH_MAX];
    char *dir;
    
    /* Get Quagga socket directory */
    if (clicon_option_exists(h, "QUAGGA_DIR"))
	dir = clicon_option_str(h, "QUAGGA_DIR");
    else
	dir = QUAGGA_DIR;
    snprintf(path, sizeof(path), "%s/%s", dir, sockpath);

    cmd = cgv_fmt_string (vr, (char *)arg);
    if (cmd == NULL)
	return 0;
  
    batch = quaggapi_strexec(path, cli_output_formatted, 0, cmd);
    if (batch == NULL) {
	rost_err(errno, "strexec");
	goto catch;
    }
    
    if (batch->numexec < 0) {
	rost_err(0, "numexec <0");
	goto catch;
    }

    output = batch->cmds[batch->numexec-1].output;
    retcode = batch->cmds[batch->numexec-1].retcode;
    if (retcode != QUAGGA_SUCCESS) {
	if (output)
	    cli_output (stdout, "%% %s\n",  output);
	else
	    cli_output (stderr, "cli_rt_exec: %s\n",  strerror (errno));
    }
    else 
	if (output)
	    cli_output_formatted (output);
    
    free (cmd);
    quaggapi_free (batch);
    
    return retcode;
    
 catch:
  if (cmd)
    free (cmd);
  if (batch)
    quaggapi_free (batch);

  return -1;
}


int
cli_zebra_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, ZEBRA_API_SOCK);
}

int
cli_rip_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, RIP_API_SOCK);
}

int
cli_ripng_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, RIPNG_API_SOCK);
}

int
cli_ospf_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, OSPF_API_SOCK);
}

int
cli_rt_ospf6_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, OSPF6_API_SOCK);
}

int
cli_bgp_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, BGP_API_SOCK);
}

int
cli_isis_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return cli_quagga_exec (h, cv_string_get(arg), vars, ISIS_API_SOCK);
}

/*
 * Exec command for all daemons. 
 * All currently means zebra, rip, ospf and bgp
 */
int
cli_all_exec(clicon_handle h, cvec *vars, cg_var *arg)
{
    return
	cli_quagga_exec (h, cv_string_get(arg), vars, ZEBRA_API_SOCK) +
	cli_quagga_exec (h, cv_string_get(arg), vars, RIP_API_SOCK) +
	cli_quagga_exec (h, cv_string_get(arg), vars, OSPF_API_SOCK) +
	cli_quagga_exec (h, cv_string_get(arg), vars, BGP_API_SOCK);
} 

/* XXX Not really quagga only */
int
cli_show_debugging(clicon_handle h, cvec *vars, cg_var *arg)
{
#if 1
    puts("Not implemented");
#else
    cli_rt_zebra_exec("show debugging zebra", 0, NULL);
    cli_rt_rip_exec("show debugging rip", 0, NULL);
    cli_rt_ospf_exec("show debugging ospf", 0, NULL);
    cli_rt_bgp_exec("show debugging bgp", 0, NULL);
#ifdef ROST_ISIS_SUPPORT
    cli_rt_isis_exec("show debugging isis", 0, NULL);
#endif
#ifdef ROST_IPV6_SUPPORT
    cli_rt_ripng_exec("show debugging ripng", 0, NULL);
/*    cli_rt_ospf6d_exec("show debugging ospf6d", 0, NULL);*/
#endif
#endif
    return 0;
}

