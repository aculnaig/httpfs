/*
    httpfs - Sitemap hierarchy and node state machine unit tests

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

#include "sitemap.h"

/* Test DJB2 hash calculation */
static void test_sitemap_hash_djb2(void **state)
{
    (void) state;

    hurd_ihash_key_t k1 = sitemap_hash_string("index.html");
    hurd_ihash_key_t k2 = sitemap_hash_string("index.html");
    hurd_ihash_key_t k3 = sitemap_hash_string("about.html");
    hurd_ihash_key_t k4 = sitemap_hash_string("docs");

    assert_int_equal(k1, k2);
    assert_int_not_equal(k1, 0);
    assert_int_not_equal(k1, k3);
    assert_int_not_equal(k1, k4);
}

/* Test node creation and initial state defaults */
static void test_sitemap_node_create_defaults(void **state)
{
    (void) state;

    struct sitemap_node *node = sitemap_node_create("docs", "/docs", true, NULL);

    assert_non_null(node);
    assert_string_equal(node->name, "docs");
    assert_string_equal(node->full_path, "/docs");
    assert_true(node->is_directory);
    assert_null(node->parent);
    assert_int_equal(node->state, NODE_STATE_PENDING);
    assert_int_equal(node->error_code, 0);
    assert_int_equal(node->ref_count, 1);
    assert_int_equal(node->size, 0);
    assert_int_equal(node->last_modified, 0);
    assert_null(node->url);
    assert_null(node->mime_type);
    assert_null(node->read_buffer);
    assert_int_equal(node->buffer_len, 0);
    assert_int_equal(node->buffer_offset, 0);

    /* Verify mutex and cond can be operated */
    pthread_mutex_lock(&node->lock);
    node->state = NODE_STATE_READY;
    pthread_cond_broadcast(&node->cond);
    pthread_mutex_unlock(&node->lock);

    sitemap_node_unref(node);
}

/* Test reference counting and cleanup */
static void test_sitemap_node_ref_unref(void **state)
{
    (void) state;

    struct sitemap_node *node = sitemap_node_create("file.txt", "/file.txt", false, NULL);
    assert_non_null(node);
    assert_int_equal(node->ref_count, 1);

    sitemap_node_ref(node);
    assert_int_equal(node->ref_count, 2);

    sitemap_node_unref(node);
    assert_int_equal(node->ref_count, 1);

    /* Second unref triggers free */
    sitemap_node_unref(node);
}

/* Test adding and finding children in directory */
static void test_sitemap_add_and_find_child(void **state)
{
    (void) state;

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    struct sitemap_node *child1 = sitemap_node_create("index.html", "/index.html", false, root);
    struct sitemap_node *child2 = sitemap_node_create("docs", "/docs", true, root);

    assert_true(sitemap_node_add_child(root, child1));
    assert_true(sitemap_node_add_child(root, child2));

    /* Cannot add child to a regular file */
    struct sitemap_node *invalid_child = sitemap_node_create("sub", "/index.html/sub", false, child1);
    assert_false(sitemap_node_add_child(child1, invalid_child));
    sitemap_node_free(invalid_child);

    /* Lookup existing children */
    struct sitemap_node *found1 = sitemap_node_find_child(root, "index.html");
    assert_ptr_equal(found1, child1);

    struct sitemap_node *found2 = sitemap_node_find_child(root, "docs");
    assert_ptr_equal(found2, child2);

    /* Lookup non-existent child */
    assert_null(sitemap_node_find_child(root, "nonexistent"));

    sitemap_node_unref(root);
}

/* Test inserting nested URL paths */
static void test_sitemap_insert_path(void **state)
{
    (void) state;

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    /* Insert deep file path */
    struct sitemap_node *leaf1 = sitemap_insert_path(root, "/docs/api/v1/endpoints.json", false);
    assert_non_null(leaf1);
    assert_string_equal(leaf1->name, "endpoints.json");
    assert_string_equal(leaf1->full_path, "/docs/api/v1/endpoints.json");
    assert_false(leaf1->is_directory);

    /* Verify intermediate directory hierarchy */
    struct sitemap_node *docs = sitemap_node_find_child(root, "docs");
    assert_non_null(docs);
    assert_true(docs->is_directory);
    assert_ptr_equal(docs->parent, root);

    struct sitemap_node *api = sitemap_node_find_child(docs, "api");
    assert_non_null(api);
    assert_true(api->is_directory);
    assert_ptr_equal(api->parent, docs);

    struct sitemap_node *v1 = sitemap_node_find_child(api, "v1");
    assert_non_null(v1);
    assert_true(v1->is_directory);
    assert_ptr_equal(v1->parent, api);
    assert_ptr_equal(leaf1->parent, v1);

    /* Insert sibling in the same directory */
    struct sitemap_node *leaf2 = sitemap_insert_path(root, "/docs/api/v1/auth.json", false);
    assert_non_null(leaf2);
    assert_string_equal(leaf2->name, "auth.json");
    assert_ptr_equal(leaf2->parent, v1);

    /* Insert directory path with trailing slash */
    struct sitemap_node *dir_leaf = sitemap_insert_path(root, "/images/", true);
    assert_non_null(dir_leaf);
    assert_string_equal(dir_leaf->name, "images");
    assert_true(dir_leaf->is_directory);

    sitemap_node_unref(root);
}

/* Test tree lookup with relative components (., ..) */
static void test_sitemap_tree_lookup(void **state)
{
    (void) state;

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    struct sitemap_node *target = sitemap_insert_path(root, "/a/b/c/file.txt", false);
    assert_non_null(target);

    /* Lookup direct path */
    struct sitemap_node *found = sitemap_tree_lookup(root, "/a/b/c/file.txt");
    assert_ptr_equal(found, target);

    /* Lookup with relative path and dot navigation */
    struct sitemap_node *found_rel = sitemap_tree_lookup(root, "a/b/../b/./c/file.txt");
    assert_ptr_equal(found_rel, target);

    /* Root lookups */
    assert_ptr_equal(sitemap_tree_lookup(root, "/"), root);
    assert_ptr_equal(sitemap_tree_lookup(root, ""), root);

    /* Missing path lookup */
    assert_null(sitemap_tree_lookup(root, "/a/b/missing.txt"));

    sitemap_node_unref(root);
}

/* Test state machine lifecycle transitions */
static void test_sitemap_state_transitions(void **state)
{
    (void) state;

    struct sitemap_node *node = sitemap_node_create("page.html", "/page.html", false, NULL);
    assert_non_null(node);
    assert_int_equal(node->state, NODE_STATE_PENDING);

    /* Transition to LOADING */
    pthread_mutex_lock(&node->lock);
    node->state = NODE_STATE_LOADING;
    pthread_mutex_unlock(&node->lock);

    /* Transition to READY with metadata */
    pthread_mutex_lock(&node->lock);
    node->state = NODE_STATE_READY;
    node->size = 4096;
    node->last_modified = 1700000000;
    node->mime_type = strdup("text/html");
    pthread_cond_broadcast(&node->cond);
    pthread_mutex_unlock(&node->lock);

    assert_int_equal(node->state, NODE_STATE_READY);
    assert_int_equal(node->size, 4096);
    assert_string_equal(node->mime_type, "text/html");

    sitemap_node_unref(node);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sitemap_hash_djb2),
        cmocka_unit_test(test_sitemap_node_create_defaults),
        cmocka_unit_test(test_sitemap_node_ref_unref),
        cmocka_unit_test(test_sitemap_add_and_find_child),
        cmocka_unit_test(test_sitemap_insert_path),
        cmocka_unit_test(test_sitemap_tree_lookup),
        cmocka_unit_test(test_sitemap_state_transitions),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
