/* HTTP protocol routines */

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
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <string.h>
#include <malloc.h>
#include <stddef.h>

#include "globals.h"

#include <hurd/hurd_types.h>
#include <hurd/netfs.h>

/* do a DNS lookup for NAME and store result in *ENT */
error_t lookup_host (char *url, struct hostent **ent) 
{
	if ( (*ent = gethostbyname(url)) == NULL )
	{
		fprintf(stderr,"wrong host name\n");
		return EINVAL;
	}
	return 0;
}

/* open a connection with the remote web server */
error_t open_connection(struct netnode *node, int *fd,off_t *head_len) 
{
	/* HTTP GET command returns head and body so we have to prune the
	 * head. *HEAD_LEN indicates the header length, for pruning upto that */
	
	error_t err;
	struct hostent *hptr;
	struct sockaddr_in dest;
	ssize_t written;
	size_t towrite;
	char buffer[4096];
	ssize_t bytes_read;
	char *token,*mesg;
	int code;
	char delimiters0[] = " ";
	char delimiters1[] = "\n";

	bzero(&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons (port);

	if ( !strcmp(ip_addr,"0.0.0.0") )
	{
		/* connection is not through a proxy server
		 * find IP addr. of remote server */
		err = lookup_host (node->url, &hptr);
		if (err)
		{
			fprintf(stderr,"Could not find IP addr %s\n",node->url);
			return err;
		}
		dest.sin_addr = *(struct in_addr *)hptr->h_addr;
	}
	else
	{
		/* connection is through the proxy server
		 * need not find IP of remote server
	         * find IP of the proxy server */
		if ( inet_aton(ip_addr,&dest.sin_addr) == 0 )
		{
			fprintf(stderr,"Invalid IP for proxy\n");
			return -1;
		}
	}

	if (debug_flag)
		fprintf (stderr, "trying to open %s:%d/%s\n", node->url,
				port, node->conn_req);

	*fd = socket (AF_INET, SOCK_STREAM, 0);
	if (*fd == -1)
	{
		fprintf(stderr,"Socket creation error\n");
		return errno;
	}

	err = connect (*fd, (struct sockaddr *)&dest, sizeof (dest));
	if (err == -1)
	{
		fprintf(stderr,"Cannot connect to remote host\n");
		return errno;
	}

	/* Send a HEAD request find header length */
	sprintf(buffer,"HEAD %s HTTP/1.0\r\nHost: %s\r\n\r\n",node->conn_req,node->url);
	towrite = strlen (buffer);
	written = TEMP_FAILURE_RETRY (write (*fd, buffer, towrite));
	if ( written == -1 || written < towrite )
	{
		fprintf(stderr,"Could not send an HTTP request to host\n");
		return errno;
	}
	
	bytes_read = read(*fd,buffer,sizeof(buffer));
	if ( bytes_read < 0 ) 
	{
		fprintf(stderr,"Error with HEAD read\n");
		return errno;
	}

	*head_len = bytes_read;
	token = strtok(buffer,delimiters0);
	token = strtok(NULL,delimiters0);
	sscanf(token,"%d",&code);
	token = strtok(NULL,delimiters1);
	mesg = strdup(token);
	if ( code != 200 ) 
	{
		/* page does not exist */
		fprintf(stderr,"Error Page not Accesible\n");
		fprintf(stderr,"%d %s\n",code,mesg);
		return EBADF;
	}
	
	close(*fd);
	
	/* Send the GET request for the url */
	*fd = socket (AF_INET, SOCK_STREAM, 0);
	if (*fd == -1)
	{
		fprintf(stderr,"Socket creation error\n");
		return errno;
	}

	err = connect (*fd, (struct sockaddr *)&dest, sizeof (dest));
	if (err == -1)
	{
		fprintf(stderr,"Cannot connect to remote host\n");
		return errno;
	}

	towrite = strlen (node->comm_buf);

	/* guard against EINTR failures */
	written = TEMP_FAILURE_RETRY (write (*fd, node->comm_buf, towrite));
	written += TEMP_FAILURE_RETRY (write (*fd, "\n\n",  2));
	if (written == -1 || written < (towrite+2)) 
	{
		fprintf(stderr,"Could not send GET request to remote host\n");
		return errno;
	}
	return 0;
}

/* fetch a directory node from the web server
 * DIR should already be locked */
error_t fill_dirnode (struct netnode *dir) 
{
	error_t err = 0;
	struct node *nd, **prevp;
	struct files *go;
	char *comm_buf,*url,*conn_req,*f_name,*temp,*temp1;

	if (debug_flag)
		fprintf (stderr, "filling out dir %s\n", dir->file_name);
	
	if ( dir->type == HTTP_DIR_NOT_FILLED ) {
		/* it is an unfilled directory so send a GET request for that
		 * directory and parse the incoming HTML stream to get the file 
		 * and directories within that
		 * and Fill the intermediate data-structure *file */
		err = parse(dir);
		if ( err )
			return err;
		dir->type = HTTP_DIR;
	}

	
	dir->noents = TRUE;
	dir->num_ents = 0;
	prevp = &dir->ents;
	
	for(go=list_of_entries;go!=NULL;go=go->next)
	{
		/* *file linked list contains all the file info obtained from
		 * parsing the <a href="..">
		 * select the ones belonging to this particular directory
		 * and fill its node */
		
		if(strcmp(dir->file_name,go->parent)==0)
		{
			/* got a file in this directory 
			 * directory under consideration is dir->file_name
			 * so have to fetch all files whose parent is
			 * dir->file_name, i.e. dir->file_name==go->parent */
			
			if ( go->f_type == HTTP_URL ) 
			{
				/* its an url 
				 * url is shown as regular file 
				 * its name is altered by changing / to .
				 * www.gnu.org/gpl.html will be changed to
				 * www.gnu.org.gpl.html */
				char *slash;
				conn_req=(char *)malloc((strlen(go->f_name)+8)*sizeof(char));
				slash = strchr(go->f_name, '/');
				if (slash)
					url = strndup(go->f_name, slash - go->f_name);
				else
					url = strdup(go->f_name);
				f_name = strdup(go->f_name);
				int i;
				for (i = 0; f_name[i] != '\0'; i++)
					if (f_name[i] == '/')
						f_name[i] = '.';
				
				sprintf(conn_req,"%s%s","http://",go->f_name);
			}
			else 
			{	
				/* its not an url */
				f_name = strdup(go->f_name);
				url=strdup(dir->url);
				if ( go != list_of_entries )
				{
					size_t conn_req_size = strlen(dir->conn_req) + strlen(go->f_name) + 1;
					if( go->f_type==HTTP_DIR || go->f_type==HTTP_DIR_NOT_FILLED )
						conn_req_size++; /* We'll need to add a trailing slash later. */
					conn_req=(char *)malloc(conn_req_size*sizeof(char));
					sprintf(conn_req,"%s%s",dir->conn_req,go->f_name);
				}
				else
				{
					if ( dir_tok[no_of_slashes] == NULL ) 
					{
						/* the file corresponding to base url
						 * user has given a file explicitly in
						 * the url */
						size_t conn_req_size = strlen(dir->conn_req) + strlen(go->f_name) + 1;
						if( go->f_type==HTTP_DIR || go->f_type==HTTP_DIR_NOT_FILLED )
							conn_req_size++; /* We'll need to add a trailing slash later. */
						conn_req=(char *)malloc(conn_req_size*sizeof(char));
						sprintf(conn_req,"%s%s",dir->conn_req,go->f_name);
					}
					else 
					{
						/* the file corresponding to base url
						 * user has not given a file explicitly 
						 * the url so its the index.html */
						size_t conn_req_size = strlen(dir->conn_req) + 1;
						if( go->f_type==HTTP_DIR || go->f_type==HTTP_DIR_NOT_FILLED )
							conn_req_size++; /* We'll need to add a trailing slash later. */
						conn_req=(char *)malloc(conn_req_size*sizeof(char));
						sprintf(conn_req,"%s",dir->conn_req);
					}
				}
				if( go->f_type==HTTP_DIR || go->f_type==HTTP_DIR_NOT_FILLED ) 
					/* the filled file is directory so it has to end
					 * with a / */
					strcat(conn_req,"/");
			}
			comm_buf=(char *)malloc((strlen(conn_req)+20)*sizeof(char));
			sprintf(comm_buf,"GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",conn_req,url);

			nd = httpfs_make_node (go->f_type,url,conn_req,comm_buf,f_name);
			if (!nd)
			{
				err = ENOMEM;
				return err;
			}
			free(comm_buf);
			free(conn_req);
			free(f_name);
			*prevp = nd;
			nd->prevp = prevp;
			prevp = &nd->next;
			dir->num_ents++;
			if (dir->noents)
				dir->noents = FALSE;
		}
	}
	return err;
}

