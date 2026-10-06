/*
    httpfs - Sitemap hierarchy and node generator implementation

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

#include "sitemap.h"

#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DJB2 string hashing algorithm */
hurd_ihash_key_t sitemap_hash_string(const char *str)
{
    unsigned long hash = 5381;
    int c;

    if (!str) {
        return (hurd_ihash_key_t)0;
    }

    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return (hurd_ihash_key_t)hash;
}

struct sitemap_node *sitemap_node_create(const char *name, const char *full_path, bool is_directory, struct sitemap_node *parent)
{
    if (!name || !full_path) {
        return NULL;
    }

    struct sitemap_node *node = malloc(sizeof(struct sitemap_node));
    if (!node) {
        return NULL;
    }

    node->name = strdup(name);
    if (!node->name) {
        free(node);
        return NULL;
    }

    node->full_path = strdup(full_path);
    if (!node->full_path) {
        free(node->name);
        free(node);
        return NULL;
    }

    node->url = NULL;
    node->is_directory = is_directory;
    node->state = NODE_STATE_PENDING;
    node->error_code = 0;
    node->last_modified = (time_t)0;
    node->size = (off_t)0;
    node->mime_type = NULL;
    node->parent = parent;

    node->read_buffer = NULL;
    node->buffer_len = 0;
    node->buffer_offset = 0;

    node->ref_count = 1;

    if (pthread_mutex_init(&node->lock, NULL) != 0) {
        free(node->full_path);
        free(node->name);
        free(node);
        return NULL;
    }

    if (pthread_cond_init(&node->cond, NULL) != 0) {
        pthread_mutex_destroy(&node->lock);
        free(node->full_path);
        free(node->name);
        free(node);
        return NULL;
    }

    hurd_ihash_init(&node->children_hash, HURD_IHASH_NO_LOCP);

    return node;
}

void sitemap_node_ref(struct sitemap_node *node)
{
    if (!node) {
        return;
    }

    pthread_mutex_lock(&node->lock);
    node->ref_count++;
    pthread_mutex_unlock(&node->lock);
}

void sitemap_node_unref(struct sitemap_node *node)
{
    if (!node) {
        return;
    }

    pthread_mutex_lock(&node->lock);
    if (--node->ref_count == 0) {
        pthread_mutex_unlock(&node->lock);
        sitemap_node_free(node);
    } else {
        pthread_mutex_unlock(&node->lock);
    }
}

void sitemap_node_free(struct sitemap_node *node)
{
    if (!node) {
        return;
    }

    /* Recursively free all child nodes in directory */
    if (node->is_directory) {
        HURD_IHASH_ITERATE(&node->children_hash, child_val) {
            struct sitemap_node *child = (struct sitemap_node *)child_val;
            sitemap_node_free(child);
        }
    }
    hurd_ihash_destroy(&node->children_hash);

    if (node->name) {
        free(node->name);
        node->name = NULL;
    }

    if (node->full_path) {
        free(node->full_path);
        node->full_path = NULL;
    }

    if (node->url) {
        free(node->url);
        node->url = NULL;
    }

    if (node->mime_type) {
        free(node->mime_type);
        node->mime_type = NULL;
    }

    if (node->read_buffer) {
        free(node->read_buffer);
        node->read_buffer = NULL;
    }

    pthread_cond_destroy(&node->cond);
    pthread_mutex_destroy(&node->lock);

    free(node);
}

bool sitemap_node_add_child(struct sitemap_node *parent, struct sitemap_node *child)
{
    if (!parent || !child || !parent->is_directory || !child->name) {
        return false;
    }

    hurd_ihash_key_t key = sitemap_hash_string(child->name);

    pthread_mutex_lock(&parent->lock);

    error_t err = hurd_ihash_add(&parent->children_hash, key, (hurd_ihash_value_t)child);
    if (err == 0) {
        child->parent = parent;
    }

    pthread_mutex_unlock(&parent->lock);

    return (err == 0);
}

struct sitemap_node *sitemap_node_find_child(struct sitemap_node *parent, const char *child_name)
{
    if (!parent || !child_name || !parent->is_directory) {
        return NULL;
    }

    hurd_ihash_key_t key = sitemap_hash_string(child_name);

    pthread_mutex_lock(&parent->lock);

    struct sitemap_node *found = (struct sitemap_node *)hurd_ihash_find(&parent->children_hash, key);
    if (found) {
        if (strcmp(found->name, child_name) == 0) {
            pthread_mutex_unlock(&parent->lock);
            return found;
        }

        /* Collision fallback: iterate entries to match child_name */
        found = NULL;
        HURD_IHASH_ITERATE(&parent->children_hash, val) {
            struct sitemap_node *cand = (struct sitemap_node *)val;
            if (strcmp(cand->name, child_name) == 0) {
                found = cand;
                break;
            }
        }
    }

    pthread_mutex_unlock(&parent->lock);

    return found;
}

struct sitemap_node *sitemap_insert_path(struct sitemap_node *root, const char *path, bool is_directory)
{
    if (!root || !path) {
        return NULL;
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        return NULL;
    }

    /* Trim trailing slashes if path ends in '/' */
    size_t len = strlen(path_copy);
    while (len > 1 && path_copy[len - 1] == '/') {
        path_copy[len - 1] = '\0';
        len--;
        is_directory = true;
    }

    char *saveptr = NULL;
    char *token = strtok_r(path_copy, "/", &saveptr);
    if (!token) {
        free(path_copy);
        return root;
    }

    struct sitemap_node *curr = root;
    char current_path[4096] = "";

    while (token != NULL) {
        char *next_token = strtok_r(NULL, "/", &saveptr);
        bool token_is_leaf = (next_token == NULL);
        bool node_is_dir = token_is_leaf ? is_directory : true;

        /* Build cumulative path */
        strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);
        strncat(current_path, token, sizeof(current_path) - strlen(current_path) - 1);

        struct sitemap_node *child = sitemap_node_find_child(curr, token);
        if (!child) {
            child = sitemap_node_create(token, current_path, node_is_dir, curr);
            if (!child) {
                free(path_copy);
                return NULL;
            }
            if (!sitemap_node_add_child(curr, child)) {
                sitemap_node_free(child);
                free(path_copy);
                return NULL;
            }
        }

        curr = child;
        token = next_token;
    }

    free(path_copy);
    return curr;
}

struct sitemap_node *sitemap_tree_lookup(struct sitemap_node *root, const char *path)
{
    if (!root || !path) {
        return NULL;
    }

    if (path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        return root;
    }

    char *path_dup = strdup(path);
    if (!path_dup) {
        return NULL;
    }

    struct sitemap_node *curr = root;
    char *saveptr = NULL;

    char *token = strtok_r(path_dup, "/", &saveptr);

    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            token = strtok_r(NULL, "/", &saveptr);
            continue;
        }

        if (strcmp(token, "..") == 0) {
            if (curr->parent != NULL) {
                curr = curr->parent;
            }
            token = strtok_r(NULL, "/", &saveptr);
            continue;
        }

        struct sitemap_node *next = sitemap_node_find_child(curr, token);
        if (!next) {
            free(path_dup);
            return NULL;
        }

        curr = next;
        token = strtok_r(NULL, "/", &saveptr);
    }

    free(path_dup);
    return curr;
}

struct sitemap_node *sitemap_fetch_and_build(const char *sitemap_url)
{
    (void)sitemap_url;
    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    return root;
}
