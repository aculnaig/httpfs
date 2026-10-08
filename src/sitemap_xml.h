/*
    httpfs - Sitemap streaming XML parser header

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

#ifndef HTTPFS_SITEMAP_XML_H
#define HTTPFS_SITEMAP_XML_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "sitemap.h"
#include <stddef.h>
#include <time.h>

/* Discovered sub-sitemap entry inside <sitemapindex> */
struct sitemap_index_entry {
    char *loc;                          /* URL of sub-sitemap */
    time_t last_modified;               /* Optional timestamp */
    struct sitemap_index_entry *next;
};

/*
 * Parse an ISO 8601 date string (e.g. "2026-10-06" or "2026-10-06T12:00:00Z")
 * into a POSIX time_t. Returns (time_t)0 on parsing error.
 */
time_t sitemap_parse_iso8601(const char *date_str);

/*
 * Extracts relative POSIX path from a full URL or relative path string.
 * Returns newly allocated string that caller must free().
 */
char *sitemap_extract_path_from_url(const char *url_or_path);

/*
 * Stream-parses sitemap XML buffer using libxml2 xmlReader.
 * Directly inserts <url> elements into root sitemap hierarchy.
 * If <sitemapindex> is encountered, appends sub-sitemaps to out_sub_sitemaps.
 * Returns 0 on success, or POSIX error code (e.g. EINVAL, ENOMEM).
 */
int sitemap_parse_xml_buffer(struct sitemap_node *root,
                            const char *xml_data,
                            size_t size,
                            struct sitemap_index_entry **out_sub_sitemaps);

/*
 * Frees a linked list of sitemap_index_entry structs.
 */
void sitemap_index_list_free(struct sitemap_index_entry *list);

#endif /* HTTPFS_SITEMAP_XML_H */
