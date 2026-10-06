/*
    httpfs - Command-line argument and fsysopts parser implementation

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

#include "httpfs_argp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global runtime parameters instance */
struct httpfs_params httpfs_params;

static const struct argp_option options[] = {
    {"sitemap",    's', "PATH_OR_URL", 0, "Sitemap path or remote URL (default: /sitemap.xml)", 0},
    {"user-agent", 'u', "STRING",      0, "User-Agent header string for HTTP requests", 0},
    {"timeout",    't', "SECONDS",     0, "Network timeout in seconds (default: 30)", 0},
    {"insecure",   'k', 0,             0, "Allow insecure TLS/SSL connections without verification", 0},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct httpfs_params *params = (struct httpfs_params *)state->input;
    if (!params) {
        return EINVAL;
    }

    switch (key) {
    case 's': {
        if (!arg || *arg == '\0') {
            return EINVAL;
        }
        char *dup = strdup(arg);
        if (!dup) {
            return ENOMEM;
        }
        pthread_mutex_lock(&params->lock);
        free(params->sitemap);
        params->sitemap = dup;
        pthread_mutex_unlock(&params->lock);
        break;
    }
    case 'u': {
        if (!arg || *arg == '\0') {
            return EINVAL;
        }
        char *dup = strdup(arg);
        if (!dup) {
            return ENOMEM;
        }
        pthread_mutex_lock(&params->lock);
        free(params->user_agent);
        params->user_agent = dup;
        pthread_mutex_unlock(&params->lock);
        break;
    }
    case 't': {
        if (!arg) {
            return EINVAL;
        }
        char *endptr = NULL;
        long val = strtol(arg, &endptr, 10);
        if (*endptr != '\0' || val <= 0 || val > 86400) {
            return EINVAL;
        }
        pthread_mutex_lock(&params->lock);
        params->timeout_sec = (int)val;
        pthread_mutex_unlock(&params->lock);
        break;
    }
    case 'k': {
        pthread_mutex_lock(&params->lock);
        params->insecure = true;
        pthread_mutex_unlock(&params->lock);
        break;
    }
    case ARGP_KEY_ARG: {
        /* Only accept one positional argument: the target base URL */
        if (state->arg_num > 0) {
            return EINVAL;
        }
        /* Validate scheme */
        if (strncmp(arg, "http://", 7) != 0 && strncmp(arg, "https://", 8) != 0) {
            return EINVAL;
        }
        char *dup = strdup(arg);
        if (!dup) {
            return ENOMEM;
        }
        pthread_mutex_lock(&params->lock);
        free(params->url);
        params->url = dup;
        pthread_mutex_unlock(&params->lock);
        break;
    }
    case ARGP_KEY_END: {
        /* Missing mandatory URL */
        if (!params->url) {
            return EINVAL;
        }
        break;
    }
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

const struct argp httpfs_argp = {
    options,
    parse_opt,
    "URL",
    "Asynchronous HTTP/HTTPS sitemap filesystem translator for GNU/Hurd",
    NULL,
    NULL,
    NULL
};

/* Hook for libnetfs fsysopts integration */
struct argp *netfs_runtime_argp = (struct argp *)&httpfs_argp;

void httpfs_params_init(struct httpfs_params *params)
{
    if (!params) {
        return;
    }

    params->url = NULL;
    params->sitemap = strdup(HTTPFS_DEFAULT_SITEMAP_PATH);
    params->user_agent = strdup(HTTPFS_DEFAULT_USER_AGENT);
    params->timeout_sec = HTTPFS_DEFAULT_TIMEOUT_SEC;
    params->insecure = false;

    pthread_mutex_init(&params->lock, NULL);
}

void httpfs_params_cleanup(struct httpfs_params *params)
{
    if (!params) {
        return;
    }

    pthread_mutex_lock(&params->lock);

    if (params->url) {
        free(params->url);
        params->url = NULL;
    }

    if (params->sitemap) {
        free(params->sitemap);
        params->sitemap = NULL;
    }

    if (params->user_agent) {
        free(params->user_agent);
        params->user_agent = NULL;
    }

    pthread_mutex_unlock(&params->lock);
    pthread_mutex_destroy(&params->lock);
}

error_t httpfs_parse_args(int argc, char **argv, struct httpfs_params *params)
{
    if (!argv || !params) {
        return EINVAL;
    }

    return argp_parse(&httpfs_argp, argc, argv, ARGP_NO_EXIT | ARGP_SILENT, NULL, params);
}
