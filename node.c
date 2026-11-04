/* Httpfs node handling routines */ 

/* This program is free software; you can redistribute it and/or
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
#include <stdlib.h>
#include <errno.h>

#include "globals.h"

#include <hurd/hurd_types.h>
#include <hurd/netfs.h>

/* make an instance of `struct netnode' with the specified parameters,
   return NULL on error */
struct netnode *httpfs_make_netnode (char type, char *url, char *conn_req,
		char *comm_buf, char *file_name)
{
	struct netnode *nn;

	nn = (struct netnode *)malloc (sizeof (struct netnode));
	if (!nn)
		return NULL;
	memset (nn, 0, sizeof (struct netnode));
	nn->type = type;
	nn->url = strdup (url); 
	nn->conn_req = strdup (conn_req);
	nn->comm_buf = strdup (comm_buf);
	nn->file_name = strdup(file_name);
	nn->ents = NULL;
	nn->noents = FALSE;

	if (!(nn->url && nn->conn_req && nn->comm_buf)) {
		free (nn->url);
		free (nn->conn_req);
		free (nn->comm_buf);
		free (nn->file_name);
		free (nn);
		return NULL;
	}

	return nn;
}

/* free an instance of `struct netnode' */
void free_netnode (struct netnode *node) {
	struct node *nd, *next;

	free (node->url);   
	free (node->conn_req);
	free (node->comm_buf);
	free (node->file_name);
	for (nd = node->ents; nd; nd = next) {
		next = nd->next;
		free (nd);
	}
}

/* make an instance of `struct node' with the specified parameters,
   return NULL on error */
struct node *httpfs_make_node (char type, char *url, char *conn_req,
		char *comm_buf,char *file_name)
{
	struct netnode *nn;
	struct node *nd;

	nn = httpfs_make_netnode (type,url, conn_req, comm_buf, file_name);
	if (!nn)
		return NULL;

	nd = netfs_make_node (nn);
	if (!nd) {
		free (nn);
		return NULL;
	}
	nd->next = NULL;
	nd->prevp = NULL;
	nd->owner = httpfs->uid;

	/*  Hold a reference to the new dir's node.  */
	netfs_nref (nd);

	/* fill in stat info for the node */
	nd->nn_stat.st_mode = (S_IRUSR | S_IRGRP | S_IROTH) & ~httpfs->umask;
	nd->nn_stat.st_mode |= (type==HTTP_DIR||type==HTTP_DIR_NOT_FILLED) ? S_IFDIR : S_IFREG;
	nd->nn_stat.st_nlink = 1;
	nd->nn_stat.st_uid = httpfs->uid;
	nd->nn_stat.st_gid = httpfs->gid;
	nd->nn_stat.st_rdev = 0;
	nd->nn_stat.st_size = 0;
	nd->nn_stat.st_blksize = 0;
	nd->nn_stat.st_blocks = 0;
	nd->nn_stat.st_ino = httpfs->next_inode++;
	fshelp_touch (&nd->nn_stat,  TOUCH_ATIME | TOUCH_MTIME | TOUCH_CTIME,
			httpfs_maptime);

	return nd;
}

/* free an instance of `struct node' */
void free_node (struct node *node) {
	free_netnode (node->nn);
	free (node);
}

