/*
    httpfs - Sitemap streaming XML parser unit tests

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
#include <time.h>
#include <cmocka.h>

#include "sitemap.h"
#include "sitemap_xml.h"

/* Test ISO 8601 date parsing */
static void test_iso8601_parsing(void **state)
{
    (void) state;

    /* Date only: 2026-10-06 */
    time_t t1 = sitemap_parse_iso8601("2026-10-06");
    assert_int_not_equal(t1, 0);

    struct tm tm1;
    gmtime_r(&t1, &tm1);
    assert_int_equal(tm1.tm_year + 1900, 2026);
    assert_int_equal(tm1.tm_mon + 1, 10);
    assert_int_equal(tm1.tm_mday, 6);

    /* Date-time with UTC Z: 2026-10-06T12:30:45Z */
    time_t t2 = sitemap_parse_iso8601("2026-10-06T12:30:45Z");
    assert_int_not_equal(t2, 0);

    struct tm tm2;
    gmtime_r(&t2, &tm2);
    assert_int_equal(tm2.tm_hour, 12);
    assert_int_equal(tm2.tm_min, 30);
    assert_int_equal(tm2.tm_sec, 45);

    /* Date-time with +02:00 timezone offset: 14:30:45+02:00 == 12:30:45 UTC */
    time_t t3 = sitemap_parse_iso8601("2026-10-06T14:30:45+02:00");
    assert_int_equal(t2, t3);

    /* Invalid date strings */
    assert_int_equal(sitemap_parse_iso8601("not-a-date"), 0);
    assert_int_equal(sitemap_parse_iso8601(""), 0);
    assert_int_equal(sitemap_parse_iso8601(NULL), 0);
}

/* Test URL path extraction */
static void test_extract_path_from_url(void **state)
{
    (void) state;

    char *p1 = sitemap_extract_path_from_url("http://example.com/docs/api/index.html");
    assert_string_equal(p1, "/docs/api/index.html");
    free(p1);

    char *p2 = sitemap_extract_path_from_url("https://example.org:8080/images/logo.png");
    assert_string_equal(p2, "/images/logo.png");
    free(p2);

    char *p3 = sitemap_extract_path_from_url("http://example.com/");
    assert_string_equal(p3, "/");
    free(p3);

    char *p4 = sitemap_extract_path_from_url("http://example.com");
    assert_string_equal(p4, "/");
    free(p4);

    char *p5 = sitemap_extract_path_from_url("/already/relative/path.txt");
    assert_string_equal(p5, "/already/relative/path.txt");
    free(p5);
}

/* Test parsing standard <urlset> sitemap */
static void test_parse_simple_urlset(void **state)
{
    (void) state;

    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n"
        "  <url>\n"
        "    <loc>http://example.com/index.html</loc>\n"
        "    <lastmod>2026-10-01</lastmod>\n"
        "  </url>\n"
        "  <url>\n"
        "    <loc>http://example.com/docs/api.html</loc>\n"
        "    <lastmod>2026-10-02T10:00:00Z</lastmod>\n"
        "  </url>\n"
        "</urlset>\n";

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    int rc = sitemap_parse_xml_buffer(root, xml, strlen(xml), NULL);
    assert_int_equal(rc, 0);

    /* Verify /index.html */
    struct sitemap_node *index_node = sitemap_node_find_child(root, "index.html");
    assert_non_null(index_node);
    assert_false(index_node->is_directory);
    assert_non_null(index_node->url);
    assert_string_equal(index_node->url, "http://example.com/index.html");
    assert_int_not_equal(index_node->last_modified, 0);

    /* Verify /docs directory and /docs/api.html */
    struct sitemap_node *docs_dir = sitemap_node_find_child(root, "docs");
    assert_non_null(docs_dir);
    assert_true(docs_dir->is_directory);

    struct sitemap_node *api_node = sitemap_node_find_child(docs_dir, "api.html");
    assert_non_null(api_node);
    assert_false(api_node->is_directory);
    assert_string_equal(api_node->url, "http://example.com/docs/api.html");
    assert_int_not_equal(api_node->last_modified, 0);

    sitemap_node_unref(root);
}

/* Test parsing <sitemapindex> containing nested sitemaps */
static void test_parse_sitemapindex(void **state)
{
    (void) state;

    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<sitemapindex xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n"
        "  <sitemap>\n"
        "    <loc>http://example.com/sitemap_pages.xml</loc>\n"
        "    <lastmod>2026-10-01</lastmod>\n"
        "  </sitemap>\n"
        "  <sitemap>\n"
        "    <loc>http://example.com/sitemap_images.xml</loc>\n"
        "    <lastmod>2026-10-02</lastmod>\n"
        "  </sitemap>\n"
        "</sitemapindex>\n";

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    struct sitemap_index_entry *sub_list = NULL;
    int rc = sitemap_parse_xml_buffer(root, xml, strlen(xml), &sub_list);
    assert_int_equal(rc, 0);

    assert_non_null(sub_list);
    assert_string_equal(sub_list->loc, "http://example.com/sitemap_pages.xml");
    assert_non_null(sub_list->next);
    assert_string_equal(sub_list->next->loc, "http://example.com/sitemap_images.xml");
    assert_null(sub_list->next->next);

    sitemap_index_list_free(sub_list);
    sitemap_node_unref(root);
}

/* Test handling malformed XML buffers */
static void test_parse_malformed_xml(void **state)
{
    (void) state;

    const char *broken_xml = "<urlset><url><loc>http://broken.com</url>";
    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    int rc = sitemap_parse_xml_buffer(root, broken_xml, strlen(broken_xml), NULL);
    assert_int_not_equal(rc, 0);

    sitemap_node_unref(root);
}

/* Test NULL / empty buffer handling */
static void test_parse_empty_buffer(void **state)
{
    (void) state;

    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    assert_non_null(root);

    assert_int_equal(sitemap_parse_xml_buffer(root, NULL, 0, NULL), EINVAL);
    assert_int_equal(sitemap_parse_xml_buffer(root, "", 0, NULL), EINVAL);

    sitemap_node_unref(root);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iso8601_parsing),
        cmocka_unit_test(test_extract_path_from_url),
        cmocka_unit_test(test_parse_simple_urlset),
        cmocka_unit_test(test_parse_sitemapindex),
        cmocka_unit_test(test_parse_malformed_xml),
        cmocka_unit_test(test_parse_empty_buffer),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
