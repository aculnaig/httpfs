/*
    httpfs - Sitemap hierarchy and node state machine header

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

#ifndef HTTPFS_SITEMAP_H
#define HTTPFS_SITEMAP_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <hurd/ihash.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

/* Node lifecycle state */
enum sitemap_node_state {
    NODE_STATE_PENDING = 0, /* Discovered in sitemap; content/metadata not yet fetched */
    NODE_STATE_LOADING,     /* Asynchronous HTTP request in progress */
    NODE_STATE_READY,       /* Metadata and child hierarchy populated */
    NODE_STATE_ERROR        /* Fetch or parsing error occurred */
};

/*
 * struct sitemap_node represents an entity in the remote website hierarchy.
 */
struct sitemap_node {
    char *name;                     /* Single path component name (e.g. "index.html" or "docs") */
    char *full_path;                /* Absolute POSIX path within translator (e.g. "/docs/index.html") */
    char *url;                      /* Remote HTTP/HTTPS URL */
    bool is_directory;              /* True if directory, false if regular file */

    enum sitemap_node_state state;  /* Current lifecycle state */
    error_t error_code;             /* POSIX error code if state == NODE_STATE_ERROR */

    /* Metadata */
    time_t last_modified;           /* Parsed from <lastmod> or HTTP Last-Modified header */
    off_t size;                     /* Size in bytes (0 if unknown) */
    char *mime_type;                /* Content-Type header (e.g. "text/html") */

    /* Tree hierarchical navigation */
    struct sitemap_node *parent;    /* Pointer to parent node (NULL for root) */

    /* Children table for directories */
    struct hurd_ihash children_hash;

    /* Synchronization */
    pthread_mutex_t lock;           /* Mutex protecting node state and children table */
    pthread_cond_t cond;            /* Condition variable for async fetch completion */

    /* In-memory read-ahead cache buffer */
    char *read_buffer;              /* Cached byte window */
    size_t buffer_len;              /* Valid bytes in read_buffer */
    off_t buffer_offset;            /* File offset of the cached window */

    /* Reference count */
    unsigned int ref_count;
};

/*
 * DJB2 string hashing algorithm generating a hurd_ihash_key_t.
 */
hurd_ihash_key_t sitemap_hash_string(const char *str);

/*
 * Allocates and initializes a new sitemap node with ref_count = 1.
 */
struct sitemap_node *sitemap_node_create(const char *name, const char *full_path, bool is_directory, struct sitemap_node *parent);

/*
 * Increment reference count.
 */
void sitemap_node_ref(struct sitemap_node *node);

/*
 * Decrement reference count; when reaches 0, invokes sitemap_node_free().
 */
void sitemap_node_unref(struct sitemap_node *node);

/*
 * Recursively deallocates a sitemap node, its children, and internal buffers.
 */
void sitemap_node_free(struct sitemap_node *node);

/*
 * Insert child into parent directory's hash table.
 * Returns true on success, false on error.
 */
bool sitemap_node_add_child(struct sitemap_node *parent, struct sitemap_node *child);

/*
 * Find child by name in parent's children hash table in O(1).
 * Handles hash collisions via name comparison.
 */
struct sitemap_node *sitemap_node_find_child(struct sitemap_node *parent, const char *child_name);

/*
 * Tokenizes a path string and creates all missing intermediate directories
 * down to the leaf node. Returns the leaf node or NULL on error.
 */
struct sitemap_node *sitemap_insert_path(struct sitemap_node *root, const char *path, bool is_directory);

/*
 * Navigates the sitemap hierarchy using POSIX relative or absolute path,
 * supporting '.' and '..' components.
 */
struct sitemap_node *sitemap_tree_lookup(struct sitemap_node *root, const char *path);

#endif /* HTTPFS_SITEMAP_H */
