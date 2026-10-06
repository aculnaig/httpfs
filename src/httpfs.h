/*
    httpfs - HTTP filesystem for the Hurd
    Header definitions

    Copyright (C) 2026 Free Software Foundation, Inc.
    Written by Gianluca Cannata <gcannata23@gmail.com>
    This file is part of the GNU Hurd.

    The GNU Hurd is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The GNU Hurd is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef HTTPFS_H
#define HTTPFS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <curl/curl.h>
#include <hurd/netfs.h>
#include <hurd/ihash.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "httpfs_argp.h"
#include "sitemap.h"

/*
 * struct netnode represents the translator-specific context attached
 * to each Hurd 'struct node' (via netfs_node_netnode(np)).
 */
struct netnode {
    char *name;                 /* Node component name */
    char *url;                  /* Remote URL associated with this node */
    struct sitemap_node *snode; /* Pointer to sitemap hierarchy node */
    CURL *curl_handle;          /* Persistent libcurl handle for transfers */
    pthread_mutex_t curl_lock;  /* Mutex synchronizing access to curl_handle */
};

/* Root node of the filesystem */
extern struct node *netfs_root_node;

/* Lifecycle initialization and cleanup functions */
error_t httpfs_init(struct netnode *root);
error_t httpfs_destroy(struct netnode *root);

#endif /* HTTPFS_H */
