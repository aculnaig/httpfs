/* Http filesystem. Argument handling routines. */

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
#include <limits.h>
#include <unistd.h>
#include <error.h>
#include <argp.h>
#include <argz.h>

#include <hurd/netfs.h>

#include "httpfs.h"

/* Argument Parsing --Variables */
const char *argp_program_version="Http Translator Version 0.1";
static const char args_doc[] = "URL";

const char *argp_program_bug_address="arunsark@yahoo.com or gopika78@yahoo.com";
static char doc[]="A trivial Http Translator";
char *tail;

static const struct argp_option options[] =
{
  {"debug",'D',0,0,"Enable the Debug option"},
  {"proxy",'X',"STRING",0,"Specify IP address of proxy server"},
  {"port",'P',"NUMBER",0,"Specify a non-standard port"},
  {"mode",'M',"STRING",0,"Set to directory or file (--mode=dir or --mode=file)"},
  {0}
};

static error_t parse_opt (int opt, char *arg, struct argp_state *state)
{
  switch (opt)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
    case ARGP_KEY_SUCCESS:
    case ARGP_KEY_ERROR:
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage(state);
      return EINVAL;
    case 'D':
      	debug_flag = 1;
	break;  
    case 'X':
	ip_addr = arg;
	break;
    case 'P':
	port = (unsigned short) strtol(arg,&tail,10);
	if ( tail == arg ) {
		perror("Bad port number\n");
		return ARGP_ERR_UNKNOWN;
	}
	break;
    case 'M':
	if (!(strcmp(arg,"file")&&strcmp(arg,"File")&&strcmp(arg,"FILE")))
		mode = 0;
	else if (!(strcmp(arg,"dir")&&strcmp(arg,"Dir")&&strcmp(arg,"DIR")))
		mode = 1;
	else
		return ARGP_ERR_UNKNOWN;
	break;
   case ARGP_KEY_ARG:
      url = arg;
      break;
    }
  return 0;
}


static struct argp_child argp_children[] =
	{ {&netfs_std_startup_argp},{0} };

static struct argp parser =
	{ options,parse_opt,args_doc,doc,argp_children };

struct argp *netfs_runtime_argp = &parser;

error_t netfs_append_args (char **argz,size_t *argz_len);

error_t httpfs_parse_args(int argc,char **argv){
	return argp_parse(&parser,argc,argv,0,0,0);
}

