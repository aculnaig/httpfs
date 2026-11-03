/* httpfs routine for extracting file and directory info from a stirng
 * to be filled to a node */

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
#include<string.h>
#include<stdio.h>
#include<error.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<netdb.h>
#include<stdlib.h>
#include<unistd.h>

#include"httpfs.h"

int check_dups(char *name,char *parent)
{
	/* if the file is already filled need not fill it again 
	 * this function checks for such duplicates */
	struct files *temp_entry;
	for ( temp_entry=list_of_entries ; temp_entry!=NULL ; temp_entry=temp_entry->next ) {
		if ( ! (strcmp(name,temp_entry->f_name)) && ! (strcmp(parent,temp_entry->parent)) )
			return 0;
	}
	return 1;
}


void fill_files(char *name,char *parent,int size,char type)
{
	/* This function fills the extracted file names to the
	 * intermediate data structure *files */
	
	struct files *temp_entry;
	temp_entry=(struct files *)malloc(sizeof(struct files));
	temp_entry->f_name=strdup(name);
	temp_entry->parent=strdup(parent);
	temp_entry->f_size=size;
	temp_entry->f_type=type;
	temp_entry->next=NULL;
       	this_entry->next=temp_entry;
	this_entry=temp_entry;
	return;
	
}

void extract(char *_string,char *parent)
{
	/* This function extracts file/directory info from string and fills
	 * into the intermediate data structure *files */

	int unfilled_dir=0;
	int i=0;
	const char delimiters[]="/";
	char *temp;
	struct hostent *x;
	char *token;
	char *name;
	int no_of_slashes_here=0;
	char *string = strdupa(_string);
	
	/* string can be a directory, file, url or path to a file */
	if ( ! (strncasecmp(string,"http://", 7)) )
	{
		/* Ignore absolute URLs */
#if 0
		/* URL */
		string = string + 7;
		if ( check_dups(string,parent) )
			fill_files(string,parent,0,HTTP_URL);
#endif
		return;
	}

	/* Drop internal link */
	token = strchr(string, '#');
	if (token)
		*token = 0;
	/* Drop GET form data */
	token = strchr(string, '?');
	if (token)
		*token = 0;

	if( string[0] == '.' && string[1] == '/')
	{
		/* Skip leading ./// */
		string++;
		while (string[0] == '/')
			string++;
	}

	if(string[0]=='/')
	{
		/* it is a path starting from root directory */
		temp = strdupa(string);

		while ( strchr(temp,'/') != NULL ) 
		{
			/* find the no. of slashes in string
			 * indicates to what level it goes in
			 * we cannot support a top level directory */
			no_of_slashes_here++;
			strcpy(temp,strchr(temp,'/'));
			temp++;
		}
		if ( no_of_slashes_here < no_of_slashes ) 
		{
			/* not going to support top level directory
			 * from a given directory
			 * www.xyz.org/file/temp is url
			 * and string contains /tars/
			 * its a directory at a top level from /file/temp/ 
			 * so cannot be supported
			 * because cd .. will take away out from the 
			 * translated node and hence file system itself */
			return;
		}
		if ( strlen(string) > 1 )
			string++;
		else
		{
			return;
		}

		token = strdupa(string);
		for ( i=0 ; i<no_of_slashes-1 ; i++ ) 
		{
			/* extract file and directory from the string
			 * like /file/temp/a.html
			 * extract file fill it as a directory
			 * extract temp fill it as a directory under file/
			 * extract a.html fill it as a file under temp/ */
			
			temp = strdupa(token); 
			if ( strcmp(dir_tok[i],strtok(temp,"/")) )
				return;
			strcpy(token,strchr(token,'/'));
			token++;
		}
		parent = strdupa("tmp");
		string = strdupa(token);
	}

	if( string[strlen(string)-1]=='/' )
	{
	/* its a directory but its not filled since string ends with /
	 * to be filled on demand make a note that its unfilled directory */
		unfilled_dir = 1;
	}
	token=strdupa(string);
	while(strchr(token,'/')!=NULL)
	{
		/* Handles Directories */
		temp=strdupa(token);
		name=strtok(temp,delimiters);
		
		strcpy(token,strchr(token,'/'));
		token=token+sizeof(char);
		if ( check_dups(name,parent) ) 
		{
			fill_files(name,parent,0,HTTP_DIR);
			if ( unfilled_dir )
				this_entry->f_type = HTTP_DIR_NOT_FILLED;
		}
		parent=strdupa(name);
	}

	if(strlen(token)>0)
	{
		/* Handle the File coming after the directory 
		 * like /file/temp/a.html
		 * a.html is filled here
		 * Also Handle if there is no slash at all -- just file name 
		 * or it can be a url */
		name=strtok(token,delimiters);
		if ( check_dups(name,parent) )
			fill_files(name,parent,0,HTTP_FILE);
	}

	return;
}

