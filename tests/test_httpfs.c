/*
 * httpfs unit tests
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "httpfs.h"

#define TEST_PORT 8080

char *url = NULL;
char *ip_addr = "0.0.0.0";
int mode = 1;
unsigned short port = TEST_PORT;
int debug_flag = 1;
struct files *list_of_entries = NULL, *this_entry = NULL;
char *dir_tok[25] = {NULL};
int no_of_slashes = 0;
volatile struct mapped_time_value *httpfs_maptime = NULL;
char *netfs_server_name = HTTPFS_SERVER_NAME;
char *netfs_server_version = HTTPFS_SERVER_VERSION;
struct httpfs *httpfs = NULL;

int main(void) {
    const struct CMUnitTest tests[] = {};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
