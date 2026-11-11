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

struct curl_read_request {
	char *buffer;
	loff_t offset;
};

static size_t curl_write_response_to_buffer(char *data, size_t size, size_t nmemb, void *userdata)
{
	struct curl_read_request *request = (struct curl_read_request *) userdata;
	if (!request) {
		return 0;
	}

	size_t total_size = size * nmemb - request->offset;
	if (total_size == 0) {
		return 0;
	}

	memcpy(request->buffer, data + request->offset, total_size);
	
	return total_size;
}

/* open a connection with the remote web server */
error_t open_connection(struct netnode *node, char *buffer, loff_t offset, off_t *head_len) 
{
	/* HTTP GET command returns head and body so we have to prune the
	 * head. *HEAD_LEN indicates the header length, for pruning upto that */
	error_t err = 0;
	long http_status_code = 200;
	double http_content_length = 0.0f;
	CURLcode curl_err = CURLE_OK;
	struct curl_read_request *request;

	request = malloc(sizeof(struct curl_read_request));
	memset(request, 0, sizeof(struct curl_read_request));
	request->buffer = buffer;
	request->offset = offset;

	CURL *curl = curl_easy_init();
	if (!curl) {
		return (err = ENOMEM);
	}

	if (!strcmp(ip_addr, "0.0.0.0")) {
		/* Connection is not through a proxy server. Do nothing. */
	} else {
		/* Connection is through a proxy server. */
		curl_easy_setopt(curl, CURLOPT_PROXY, ip_addr);
	}

	curl_easy_setopt(curl, CURLOPT_URL, node->url);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_response_to_buffer);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

	if (debug_flag)
		fprintf (stderr, "trying to open %s:%d\n", node->url,
				port);

	curl_err = curl_easy_perform(curl);
	if (curl_err != CURLE_OK) {
		fprintf (stderr, "curl_easy_perform(): %s\n", curl_easy_strerror(curl_err));
		err = EAGAIN;
		goto cleanup;
	}

	curl_err = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
	if (curl_err != CURLE_OK) {
		fprintf(stderr, "Cannot get HTTP status code: %s\n", curl_easy_strerror(curl_err));
		err = EAGAIN;
		goto cleanup;
	} else if (http_status_code != 200) {
		fprintf(stderr, "HTTP status code is: %ld\n", http_status_code);
		err = EIO;
		goto cleanup;
	}

	curl_err = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &http_content_length);
	if (curl_err != CURLE_OK) {
		fprintf(stderr, "Could not determine HTTP content length: %s\n", curl_easy_strerror(curl_err));
		err = EIO;
		goto cleanup;
	}

	*head_len = (off_t) http_content_length;

	err = 0;
	goto cleanup;

	cleanup:
		free(request);
		curl_easy_cleanup(curl);
		return err;
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

