/*
 *  CVS Version: $Id: grub.c,v 1.25 2013/08/09 13:27:46 olof Exp $
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_cli.h>

/* rost lib */
#include <grub.h>

/* local */
#include "ios.h"


/*
 * Internal function to modify the grub menu.lst file. Both add and delete
 * callbacks from cligen use this by specifying 'grub_addimage' and
 * 'grub_delimage' respectively in the 'grub_mod' argument.
 */
static int 
cli_modify_boot(clicon_handle h, char *img, struct grub_conf *(*grub_mod)())
{
  int fd = -1;
  FILE *f = NULL;
  char file[] = "/tmp/grubXXXXXX";
  struct grub_conf *g;
  struct clicon_msg *msg;

  /* Add new image */
  if ((g = grub_mod(NULL, img)) == NULL)
    goto catch;
  
  /* Create new temp menu file */
  if ((fd = mkstemp(file)) < 0 || (f = fdopen(fd, "w")) < 0) {
    clicon_err(OE_UNIX, errno, "mkstemp/fdopen");
    goto catch;
  }
  if (grub_printmenu(g, f) <= 0) {
    clicon_err(OE_UNIX, 0, "Failed to generate menu.lst");
    goto catch;
  }
  fclose(f);
  f = NULL;
  fd = -1;

  /* copy file to grub location, using config daemon is required */
  if (geteuid() != 0) {
    msg = clicon_msg_copy_encode(file, GRUB_DIR "/menu.lst", __FUNCTION__);
    if (msg == NULL)
      goto catch;
    if (clicon_rpc_connect(msg, clicon_sock(h), 
			   NULL, 0, __FUNCTION__) < 0)
      goto catch;
  }
  else { /* root */
    if (file_cp(file, GRUB_DIR "/menu.lst") < 0) {
      clicon_err(OE_CFG, errno, "copy");
      goto catch;
    }
  }

 catch:
  unlink(file);
  if (g)
    grub_free(g);
  if (f)
    fclose(f);
  else if (fd >= 0)
    close(fd);
  unchunk_group(__FUNCTION__);
  return 0;
}

int 
cli_add_boot(clicon_handle h, cvec *vars, cg_var *arg)
{
    cg_var *cv1 = cvec_i(vars, 1);

    cli_modify_boot(h, cv_string_get(cv1), grub_addimage);
    return 0;
}
int 
cli_del_boot(clicon_handle h, cvec *vars, cg_var *arg)
{
    cg_var *cv1 = cvec_i(vars, 1);

    cli_modify_boot(h, cv_string_get(cv1), grub_delimage);
    return 0;
}
