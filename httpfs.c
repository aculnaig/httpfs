/* HTTP filesystem 

   Copyright (C) 2002 Arun V. XXX: Please put your full name here.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#define _GNU_SOURCE 1
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <maptime.h>
#include <errno.h>
#include <error.h>
#include <argp.h>

#include <hurd/netfs.h>

#include "httpfs.h"

/* global configuration parameters */
unsigned short port;
int debug_flag;
int mode;
int no_of_slashes = 0;
char *url, *conn_req;
char *ip_addr;
char *dir_tok[25];
struct files *list_of_entries = NULL, *this_entry;

struct httpfs *httpfs;		/* filesystem global pointer */
volatile struct mapped_time_value *httpfs_maptime;

char *netfs_server_name    = HTTPFS_SERVER_NAME;
char *netfs_server_version = HTTPFS_SERVER_VERSION;

int
main (int argc, char **argv)
{
  error_t err;
  mach_port_t bootstrap;
  char *temp_url, *temp, *run;
  char type;
  char *comm_buf; /* XXX: Is an http request limited to 200 bytes? */
  port = 80;
  debug_flag = 0;
  mode = 1;			/* means directory */
  ip_addr = strdup ("0.0.0.0");

  if (debug_flag)
    fprintf (stderr, "pid %d\n", getpid ());
  httpfs_parse_args (argc, argv);

  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (1, 0, "Must be started as a translator.");

  err = maptime_map (0, 0, &httpfs_maptime);
  if (err)
    error (1, 0, "Map time error.");

  if (strchr (url, '/') == NULL)
    error (1, 0, "Url must have a /, e.g., www.gnu.org/");

  conn_req = (char *) malloc ((strlen (url) + 7) * sizeof (char));
  if (! conn_req)
    error (1, errno, "Cannot malloc conn_req.");

  temp_url = strdup (url);
  if (! temp_url)
    error (1, errno, "Cannot duplicate url.");

  if (!strncmp (temp_url, "http://", 7))
    /* go ahead of http:// if given in url */
    temp_url = temp_url + 7;

  if (strchr (temp_url, '/') == NULL)
    error (1, 0, "Url must have a /, e.g., www.gnu.org/");

  /* XXX: strtok is not reentrant.  This will have to be fixed */
  temp = strdup (temp_url);
  url = strtok (temp, "/");

  /* Find the directories given in URL */
  temp = strdup (temp_url);
  no_of_slashes++;
  strcpy (temp, strchr (temp, '/'));
  temp++;
  while (strchr (temp, '/') != NULL)
    {
      /* go to the end of url */
      run = strdup (temp);
      dir_tok[no_of_slashes - 1] = strtok (run, "/");
      strcpy (temp, strchr (temp, '/'));
      temp++;
      no_of_slashes++;
    }
  if (strlen (temp))
    {
      /* user has input a specific html file in the url */
      dir_tok[no_of_slashes - 1] = strdup (temp);
      dir_tok[no_of_slashes] = NULL;
    }
  else
    {
      /* user has input just an url no file names specifed 
       * assume the base url request is to index.html */
      dir_tok[no_of_slashes - 1] = strdup ("index.html");
      dir_tok[no_of_slashes] = strdup ("index.html");
    }

  strcpy (conn_req, "http://");
  if (temp_url[strlen (temp_url) - 1] == '/')
    {
      strcat (conn_req, temp_url);
      err = asprintf (&comm_buf, "GET %s HTTP/1.0", conn_req);
    }
  else
    {
      while (strchr (temp_url, '/') != NULL)
	{
	  temp = strdup (temp_url);
	  strcat (conn_req, strtok (temp, "/"));
	  strcat (conn_req, "/");
	  strcpy (temp_url, strchr (temp_url, '/'));
	  temp_url++;
	}
      err = asprintf (&comm_buf, "GET %s%s HTTP/1.0", conn_req, temp_url);
    }
  if (err < 0)  /* check the return value of asprintf */
    error (1, errno, "Cannot allocate comm_buf.");

  httpfs = (struct httpfs *) malloc (sizeof (struct httpfs));
  if (! httpfs)
    error (1, errno, "Cannot allocate httpfs.");

  httpfs->umask = 0111;
  httpfs->uid = getuid ();
  httpfs->gid = getgid ();
  httpfs->next_inode = 0;
  if (mode)
    type = HTTP_DIR;
  else
    type = HTTP_FILE;

  /* XXX: why is tmp hardcoded? */
  httpfs->root = httpfs_make_node (type, url, conn_req, comm_buf, "tmp");
  netfs_init ();
  /* translator set to a directory */
  if (mode)
    {
      /* fill the directory node with files 
       * call parser for that 
       * only the current directory is filled
       * subdirectories within them are indicated by type
       * HTTP_DIR_UNFILLED, and are filled as on demand when an
       * ls request comes for them */
      err = parse (httpfs->root->nn);
      if (err)
	error (1, err, "Error in Parsing.");
    }

  if (debug_flag)
    fprintf (stderr, "entering the main loop\n");

  netfs_root_node = httpfs->root;
  netfs_startup (bootstrap, 0);
  for (;;)
    netfs_server_loop ();

  /* NOT REACHED */
  free (httpfs);
  return 0;
}
