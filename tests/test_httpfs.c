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

#include "globals.h"

/* http.c unit tests */
void __wrap_curl_easy_cleanup(CURL *curl) {
    (void) curl;
}

CURLcode __wrap_curl_easy_setopt(CURL *curl, CURLoption option, ...) {
    (void) curl;
    (void) option;

    return CURLE_OK;
}

CURL *__wrap_curl_easy_init() {
    return (CURL *)mock();
}

CURLcode __wrap_curl_easy_perform(CURL *curl) {
    return (CURLcode) mock();
}

static void test_open_connection_curl_easy_init_fails(void **state)
{
    (void) state;

    will_return(__wrap_curl_easy_init, NULL);

    error_t err = open_connection(NULL, NULL, 0, NULL);

    assert_int_equal(err, ENOMEM);
}

static void test_open_connection_curl_easy_perform_fails(void **state)
{
    (void) state;

    CURLcode curl_err = curl_global_init(CURL_GLOBAL_ALL);
    if (curl_err != CURLE_OK) {
        fprintf(stderr, "curl_global_init(): %s\n", curl_easy_strerror(curl_err));
        assert_true(false);
    }

    CURL *mock_curl = (CURL *) 0x123456789;
    will_return(__wrap_curl_easy_init, mock_curl);
    will_return_count(__wrap_curl_easy_setopt, CURLE_OK, -2);
    will_return(__wrap_curl_easy_perform, CURLE_ABORTED_BY_CALLBACK);

    /* No proxy */
    ip_addr = "0.0.0.0";
    debug_flag = 1;
    port = 8080;

    struct netnode *node = malloc(sizeof(struct netnode));
    if (!node) {
        fprintf(stderr, "malloc(): ENOMEM.\n");
        assert_true(false);
    }

    memset(node, 0, sizeof(struct netnode));
    node->url = "http://localhost:8080/";
    node->conn_req = "http://localhost:8080/";

    error_t err = open_connection(node, NULL, 0, NULL);

    assert_int_equal(err, EAGAIN);

    free(node);
    curl_global_cleanup();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_open_connection_curl_easy_init_fails),
        cmocka_unit_test(test_open_connection_curl_easy_perform_fails),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
