/*
    Sitemap test suite

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

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sitemap.h"

/* A test case that does nothing and succeeds. */
static void test_null_success(void **state)
{
    (void) state; /* Unused */
}

/* Check if a sitemap node is created correctly. */
static void test_sitemap_node_create(void **state)
{
    (void) state; /* Unused */

    struct sitemap_node *node = sitemap_node_create("docs", "/docs", true, NULL);

    assert_non_null(node);
    assert_string_equal(node->name, "docs");
    assert_string_equal(node->full_path, "/docs");
    assert_true(node->is_directory);
    assert_null(node->parent);

    sitemap_node_free(node);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_null_success),
        cmocka_unit_test(test_sitemap_node_create),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
