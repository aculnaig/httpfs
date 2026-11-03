/* Httpfs HTML Parser routines 
 * the html parser is based on libxml2 library
 * the libxml2 provides a HTML parser and SAX interfaces for the parser */

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
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/SAX.h>

#include "httpfs.h"

#define MAXBUF 4096

char *parent;

void startElement(void *ctx, const xmlChar *name, const xmlChar **atts)
{
	/* this function is called at the start of a tag */
	int i;
	/* check if its an anchor tag */
	if ( ! (strcmp(name,"a")&&strcmp(name,"A")) )
	{
	if (atts!=NULL)
	{
		for(i=0;(atts[i]!=NULL);i++)
		{
			if ( ! strcasecmp(atts[i],"href") )
			{
				/* got an a href */
				i++;
				if (atts[i]!=NULL) 
				/* get the file/directories from atts[i] 
				 * call extract() */
					extract((char *)atts[i],parent);
			}
			else
				i++;
		}
	}
	}
}
			

htmlSAXHandler empty = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	startElement,/* only intereted in begining of anchor tags */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

	
error_t parse(struct netnode *node)
{
	/* SAX HTML Parsing Variables */
	htmlSAXHandlerPtr emptysax = &empty;
	error_t err;
	static int remote_fd;
	off_t head_len=0,offset=0;
	char buffer[MAXBUF];
	int bytes_read=0;
	struct files *temp_entry;
	htmlDocPtr doc;
	htmlParserCtxtPtr ctxt;

	parent = strdup(node->file_name);
	if ( debug_flag )
		fprintf(stderr,"In the HTML parser for parsing %s\n",parent);
	
	/* Create a file for base url */
	if ( list_of_entries == NULL )
	{
		/* The node of the url entered */
		list_of_entries = (struct files *)malloc(sizeof(struct files));
		list_of_entries->f_name = strdup(dir_tok[no_of_slashes-1]);
		list_of_entries->parent = strdup("tmp");
		list_of_entries->f_size = 0;//content_len;
		list_of_entries->f_type = HTTP_FILE;
		list_of_entries->next = NULL;
		this_entry = list_of_entries;
	}

	/* get the HTML stream corresponding to the directory
	 * send a GET request */
	err = open_connection(node,&remote_fd,&head_len);
	if ( err )
		return err;
	bytes_read = pread(remote_fd,buffer,sizeof(char)*6,offset);

	if ( bytes_read > 0 ) 
	{
		/* Parser operates in Push mode */
		ctxt = htmlCreatePushParserCtxt(emptysax,NULL,buffer,bytes_read,"abracabadra",0); //file name is irrelevant abracabadra
		
		while ( (bytes_read=pread(remote_fd,buffer,sizeof(buffer),offset)) > 0) 	{
			htmlParseChunk(ctxt,buffer,bytes_read,0);
		}
		htmlParseChunk(ctxt,buffer,0,1);
		doc = ctxt->myDoc;
		htmlFreeParserCtxt(ctxt);
		if ( doc != NULL )
		{
			fprintf(stderr,"Returned non null value\n");
			xmlFreeDoc(doc);
		}
	}
	
	for ( temp_entry=list_of_entries;temp_entry!=NULL;temp_entry=temp_entry->next) {
		if ( !(strcmp(temp_entry->f_name,node->file_name)) )
		{
		/* the directory is filled make a note in *files data structure
		 * also */
			temp_entry->f_type = HTTP_DIR;
			break;
		}
	}
	return 0;
}

