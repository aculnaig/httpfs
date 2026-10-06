/*
    httpfs - Command-line argument and fsysopts parser unit tests

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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include "httpfs_argp.h"

/* Test default options when only a valid URL is supplied */
static void test_argp_defaults(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {"httpfs", "http://example.com", NULL};
    int argc = 2;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_equal(err, 0);

    assert_non_null(params.url);
    assert_string_equal(params.url, "http://example.com");

    assert_non_null(params.sitemap);
    assert_string_equal(params.sitemap, HTTPFS_DEFAULT_SITEMAP_PATH);

    assert_non_null(params.user_agent);
    assert_string_equal(params.user_agent, HTTPFS_DEFAULT_USER_AGENT);

    assert_int_equal(params.timeout_sec, HTTPFS_DEFAULT_TIMEOUT_SEC);
    assert_false(params.insecure);

    httpfs_params_cleanup(&params);
}

/* Test overriding all options via short and long flags */
static void test_argp_all_options(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {
        "httpfs",
        "--sitemap=/custom_sitemap.xml",
        "--user-agent=TestAgent/2.0",
        "--timeout=45",
        "--insecure",
        "https://secure.example.com",
        NULL
    };
    int argc = 6;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_equal(err, 0);

    assert_non_null(params.url);
    assert_string_equal(params.url, "https://secure.example.com");

    assert_non_null(params.sitemap);
    assert_string_equal(params.sitemap, "/custom_sitemap.xml");

    assert_non_null(params.user_agent);
    assert_string_equal(params.user_agent, "TestAgent/2.0");

    assert_int_equal(params.timeout_sec, 45);
    assert_true(params.insecure);

    httpfs_params_cleanup(&params);
}

/* Test short flag variants (-s, -u, -t, -k) */
static void test_argp_short_options(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {
        "httpfs",
        "-s", "https://example.com/sitemap_index.xml",
        "-u", "ShortAgent/1.0",
        "-t", "10",
        "-k",
        "https://example.com",
        NULL
    };
    int argc = sizeof(argv) / sizeof(argv[0]) - 1;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_equal(err, 0);

    assert_string_equal(params.url, "https://example.com");
    assert_string_equal(params.sitemap, "https://example.com/sitemap_index.xml");
    assert_string_equal(params.user_agent, "ShortAgent/1.0");
    assert_int_equal(params.timeout_sec, 10);
    assert_true(params.insecure);

    httpfs_params_cleanup(&params);
}

/* Test error when positional URL is missing */
static void test_argp_missing_url(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {"httpfs", NULL};
    int argc = 1;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_not_equal(err, 0);

    httpfs_params_cleanup(&params);
}

/* Test error when invalid URL scheme is provided */
static void test_argp_invalid_url_scheme(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {"httpfs", "ftp://ftp.example.com", NULL};
    int argc = 2;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_equal(err, EINVAL);

    httpfs_params_cleanup(&params);
}

/* Test error when non-positive timeout is passed */
static void test_argp_invalid_timeout(void **state)
{
    (void) state;
    struct httpfs_params params;
    httpfs_params_init(&params);

    char *argv[] = {"httpfs", "--timeout=0", "http://example.com", NULL};
    int argc = 3;

    error_t err = httpfs_parse_args(argc, argv, &params);
    assert_int_equal(err, EINVAL);

    httpfs_params_cleanup(&params);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_argp_defaults),
        cmocka_unit_test(test_argp_all_options),
        cmocka_unit_test(test_argp_short_options),
        cmocka_unit_test(test_argp_missing_url),
        cmocka_unit_test(test_argp_invalid_url_scheme),
        cmocka_unit_test(test_argp_invalid_timeout),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
