/*
    httpfs - A simple HTTP filesystem for the Hurd.

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "httpfs.h"
#include <hurd/netfs.h>

char *netfs_server_name = "httpfs";
char *netfs_server_version = "0.1.0";

/* The root node of the filesystem. */
struct node *netfs_root_node = NULL;

int main(int argc, char **argv)
{
    mach_port_t bootstrap_port;
    error_t err = 0;

    /* Initialize parameters and parse CLI options */
    httpfs_params_init(&httpfs_params);
    err = httpfs_parse_args(argc, argv, &httpfs_params);
    if (err != 0) {
        httpfs_params_cleanup(&httpfs_params);
        error(1, err, "Failed to parse command-line arguments");
    }

    /* Initialize the netfs server */
    netfs_init();

    /* Create the root netnode */
    struct netnode *nn_root = malloc(sizeof(struct netnode));
    if (nn_root == NULL) {
        httpfs_params_cleanup(&httpfs_params);
        error(1, ENOMEM, "Failed to create root netnode");
    }
    err = httpfs_init(nn_root);
    if (err != 0) {
        free(nn_root);
        httpfs_params_cleanup(&httpfs_params);
        error(1, err, "Failed to initialize root netnode");
    }

    /* Create the root node */
    netfs_root_node = netfs_make_node(nn_root);
    if (netfs_root_node == NULL) {
        httpfs_destroy(nn_root);
        free(nn_root);
        httpfs_params_cleanup(&httpfs_params);
        error(1, ENOMEM, "Failed to create root node");
    }

    /* Start the netfs server */
    task_get_bootstrap_port(mach_task_self(), &bootstrap_port);
    netfs_startup(bootstrap_port, 0);

    /* Enter the main server loop */
    netfs_server_loop();

    /* Shutdown the netfs server */
    err = netfs_shutdown(0);
    if (err != 0) {
        error(0, err, "Error occurred shutting down netfs");
    }
    err = httpfs_destroy(nn_root);
    if (err != 0) {
        error(0, err, "Error occurred shutting down httpfs");
    }
    free(nn_root);
    httpfs_params_cleanup(&httpfs_params);

    return 0;
}
