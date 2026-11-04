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

#include "globals.h"

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
