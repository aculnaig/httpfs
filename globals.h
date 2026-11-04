/*
 * httpfs global definitions
 *
 * Copyright (C) 2026 Free Software Foundation, Inc.
 * This file is part of the GNU Hurd.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "httpfs.h"

/* declaration of global configuration parameters */
extern int debug_flag;
extern volatile struct mapped_time_value *httpfs_maptime;

/* Protocol requirements */
extern char *url, *conn_req;
extern unsigned short port;
extern char *ip_addr;

/* set translator to a file or directory indicate it in mode
 * mode accepted as command line argument */
extern int mode;

/* no of / in the url indicates the level of directory to go into the web server * cannot move to a top level directory from the given url 
 * eg: www.xyz/file/temp/ is the url from here we cannot show the contents of 
 * the root directory / of file/ directory
 * only contents of temp/ and its subdirectories can be supported */
extern int no_of_slashes;

/* if the url points to particular file explicitly given store here 
 * else assume it to be index.html 
 * like www.gnu.org/gpl.html and www.gnu.org/  no file given so index.html */
extern char *dir_tok[25];

extern struct files *list_of_entries,*this_entry;

/* global pointer to the filesystem */
extern struct httpfs *httpfs;

#define HTTPFS_SERVER_NAME    PACKAGE
#define HTTPFS_SERVER_VERSION VERSION

#endif /* __GLOBALS_H__ */
