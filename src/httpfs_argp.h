/*
    httpfs - Command-line argument and fsysopts parser header

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

#ifndef HTTPFS_HTTPFS_ARGP_H
#define HTTPFS_HTTPFS_ARGP_H

#include <argp.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#define HTTPFS_DEFAULT_SITEMAP_PATH "/sitemap.xml"
#define HTTPFS_DEFAULT_TIMEOUT_SEC  30
#define HTTPFS_DEFAULT_USER_AGENT   "httpfs/0.1.0"

/* Runtime configuration parameters for httpfs */
struct httpfs_params {
    char *url;          /* Remote base URL (e.g. "http://example.com") */
    char *sitemap;      /* Path or URL to sitemap XML (default: "/sitemap.xml") */
    char *user_agent;   /* HTTP User-Agent header string */
    int timeout_sec;    /* Request timeout in seconds */
    bool insecure;      /* Disables TLS certificate validation if true */

    pthread_mutex_t lock; /* Mutex protecting dynamic updates via fsysopts */
};

/* Global runtime parameters instance */
extern struct httpfs_params httpfs_params;

/* Exported GNU argp parser structure */
extern const struct argp httpfs_argp;

/*
 * Initialize httpfs_params with default values.
 */
void httpfs_params_init(struct httpfs_params *params);

/*
 * Free dynamically allocated strings inside httpfs_params.
 */
void httpfs_params_cleanup(struct httpfs_params *params);

/*
 * Parse command line or fsysopts arguments into params.
 * Returns 0 on success, or POSIX errno (e.g. EINVAL, ENOMEM) on failure.
 */
error_t httpfs_parse_args(int argc, char **argv, struct httpfs_params *params);

#endif /* HTTPFS_HTTPFS_ARGP_H */
