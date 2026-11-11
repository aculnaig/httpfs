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
#include <errno.h>
#include <netdb.h>
#include <microhttpd.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "globals.h"

/* http.c integration tests */
static enum MHD_Result
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **req_cls)
{
    const char *page = "<html><body>Hello, browser!</body></html>";
    struct MHD_Response *response;
    enum MHD_Result ret;
    (void) cls;
    (void) url;
    (void) method;
    (void) version;
    (void) upload_data;
    (void) upload_data_size;
    (void) req_cls;

    response = MHD_create_response_from_buffer_static(strlen(page), page);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

static void test_open_connection_curl_easy_perform_success(void **state)
{
    (void) state;

    /* No proxy */
    ip_addr = "0.0.0.0";
    debug_flag = 1;
    port = 8080;

    CURLcode curl_err = curl_global_init(CURL_GLOBAL_ALL);
    if (curl_err != CURLE_OK) {
        fprintf(stderr, "curl_global_init(): %s\n", curl_easy_strerror(curl_err));
        exit(EXIT_FAILURE);
    }

    struct MHD_Daemon *daemon;

    struct sockaddr_in localhost;
    memset(&localhost, 0, sizeof(localhost));
    localhost.sin_family = AF_INET;
    localhost.sin_port = htons(port);
    localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
                              port, NULL, NULL,
                              &answer_to_connection, NULL,
                              MHD_OPTION_SOCK_ADDR, (struct sockaddr *) &localhost,
                              MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "MHD_start_daemon(): ENOMEM\n");
        exit(EXIT_FAILURE);
    }

    struct netnode *node = malloc(sizeof(struct netnode));
    if (!node) {
        fprintf(stderr, "malloc(): ENOMEM.\n");
        exit(EXIT_FAILURE);
    }

    memset(node, 0, sizeof(struct netnode));
    node->url = "http://localhost:8080/";
    node->conn_req = "http://localhost:8080/";

    char buffer[4096];
    off_t head_len;

    error_t err = open_connection(node, buffer, 0, &head_len);

    assert_int_equal(err, 0);
    assert_int_equal(head_len, 0);

    free(node);
    MHD_stop_daemon(daemon);
    curl_global_cleanup();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_open_connection_curl_easy_perform_success),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
