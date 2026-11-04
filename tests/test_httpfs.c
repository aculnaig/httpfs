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

struct hostent *__wrap_gethostbyname(const char *name) {
    assert_non_null(name);

    if (strcmp(name, "www.example.com") == 0) {
        return (struct hostent *)mock();
    }

    return (struct hostent *)mock();
}

/* http.c unit tests */
static void test_lookup_host_1(void **state) {
    (void) state;

    struct hostent mock_hostent;
    memset(&mock_hostent, 0, sizeof(mock_hostent));
    mock_hostent.h_name = "www.example.com";

    will_return(__wrap_gethostbyname, &mock_hostent);

    struct hostent *result = NULL;
    int ret = lookup_host("www.example.com", &result);

    assert_int_equal(ret, 0);
    assert_non_null(result);
    assert_string_equal(result->h_name, "www.example.com");
}

static void test_lookup_host_2(void **state) {
    (void) state;

    will_return(__wrap_gethostbyname, NULL);

    struct hostent *result = NULL;
    int ret = lookup_host("invalid.url", &result);

    assert_int_equal(ret, EINVAL);
    assert_null(result);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lookup_host_1),
        cmocka_unit_test(test_lookup_host_2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
