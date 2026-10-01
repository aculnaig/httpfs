/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 Gianluca Cannata <gcannata23@gmail.com> */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <hurd/netfs.h>

char *netfs_server_name = "httpfs";
char *netfs_server_version = "0.1.0";

int main(void)
{
    error_t err = 0;

    netfs_init();

    err = netfs_shutdown(0);
    if (err != 0) {
        error(1, err, "Error occurred shutting down netfs.");
    }
}
