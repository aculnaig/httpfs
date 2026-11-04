/* HTTP filesystem header declarations */

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

#ifndef __HTTPFS_H__
#define __HTTPFS_H__

#include <maptime.h>
#include <sys/stat.h>
#include <netdb.h>

#include <hurd/hurd_types.h>

#include "config.h"

/* handle all initial parameter parsing */
error_t httpfs_parse_args (int argc, char **argv);

/* Intermediate data structure for Files and Directory information parsed from
 * the HTML stream */
struct files {
	char *f_name; /* parsed files name */
	char *parent; /* which directory it is in */
	char f_type; /* its type file or directory or url */
	int f_size; /* size */
	struct files *next; /* linked list */
};

/* private data per `struct node' */
struct netnode {
	char *url; /* www.gnu.org/ */
	char *conn_req; /* http://www.gnu.org/ */
	char *comm_buf; /* GET http://www.gnu.org/ HTTP/1.0 \n\n */
	char *file_name;
	enum {
	HTTP_FILE = '0', /* regular file */
	HTTP_DIR = '1', /* a directory which is filled */
	
	/* unfilled directories -- do not know what comes under them now
	 * to be filled when an ls occurs for that directory */
	HTTP_DIR_NOT_FILLED = '2', /* unfilled directory */
	
	/* external urls treated as files -- file system read requests
	 * supported on them */
	HTTP_URL = '3' /* external url */
	} type;

	/* directory entries if this is a directory */
	struct node *ents;
	boolean_t noents;
	unsigned int num_ents;
};

/* The filesystem data type */
struct httpfs {
	struct node *root;
	/* stat information */
	mode_t umask;
	uid_t uid;
	gid_t gid;
	ino_t next_inode;
};

/* parse the html stream to extract file names and dir names and other info 
 * html parser is required to extract <a href=".."> tags
 * this tags gives file names or directory names in web server
 * it can also be an url
 * parsed files and directory names are used to fill a particular directory node
 * */
error_t parse(struct netnode *node);

/* extract the file names and directory names or url from a given string 
 * parse function calls this an argument of href can be a file or directory or 
 * a path to a file. have to extract the files directories from the parsed 
 * string */
void extract(char *string,char *parent);

/* do a DNS lookup for NAME and store result in *ENT */
error_t lookup_host (char *url, struct hostent **ent);

/* store the remote socket in *FD and establish the connection and send the 
 * http GET request */
error_t open_connection(struct netnode *node, int *fd,off_t *head_len);

/* make an instance of `struct netnode' with the specified parameters,
 * return NULL on error */
struct netnode *httpfs_make_netnode (char type, char *url, char *conn_req, char 				*comm_buf, char *file_name);

/* fetch a directory node from the web server
 * DIR should already be locked  called in netfs_get_dirents */
error_t fill_dirnode (struct netnode *dir);

/* free an instance of `struct netnode' */
void free_netnode (struct netnode *node);

/* make an instance of `struct node' with the specified parameters,
 * return NULL on error */
struct node *httpfs_make_node (char type, char *url, char *conn_req,
			       char *comm_buf, char *file_name);
			      

/* free an instance of `struct node' */
void free_node (struct node *node);

#endif /* __HTTPFS_H__ */

