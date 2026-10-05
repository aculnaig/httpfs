/*
    Sitemap node generator functions

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

#include <error.h>
#include "sitemap.h"
#include <stdlib.h>
#include <string.h>

/* FNV-1a of a 32-bit string */
static inline hurd_ihash_key_t hash_string(const char *str)
{
    /* Offset basis */
    unsigned int hash = 2166136261u;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 16777619u; /* P = 2^24 + 2^8 + 0x93 = 16777619 */
    }
    return (hurd_ihash_key_t)hash;
}

void sitemap_node_free(struct sitemap_node *node)
{
    if (node) {
        if (node->name)
            free(node->name);

        if (node->full_path)
            free(node->full_path);

        free(node);
    }
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

    node->name = strndup(name, strlen(name) + 1);
    if (!node->name) {
        sitemap_node_free(node);
        return NULL;
    }

    node->full_path = strndup(full_path, strlen(full_path) + 1);
    if (!node->full_path) {
        sitemap_node_free(node);
        return NULL;
    }

    node->is_directory = is_directory;
    node->last_modified = (time_t)0;
    node->size = (off_t)0;
    node->parent = parent;

    if (pthread_mutex_init(&node->lock, NULL) != 0) {
        sitemap_node_free(node);
        return NULL;
    }

    hurd_ihash_init(&node->children_hash, HURD_IHASH_NO_LOCP);

    return node;
}

bool sitemap_node_add_child(struct sitemap_node *parent, struct sitemap_node *child)
{
    if (!parent || !child || !parent->is_directory || !child->name) {
        return false;
    }

    hurd_ihash_key_t key = hash_string(child->name);

    pthread_mutex_lock(&parent->lock);

    error_t err = hurd_ihash_add(&parent->children_hash, key, (hurd_ihash_value_t)child);

    pthread_mutex_unlock(&parent->lock);

    return (err == 0);
}

struct sitemap_node *sitemap_node_find_child(struct sitemap_node *parent, const char *child_name)
{
    if (!parent || !child_name || !parent->is_directory) {
        return NULL;
    }

    hurd_ihash_key_t key = hash_string(child_name);

    pthread_mutex_lock(&parent->lock);

    struct sitemap_node *found = (struct sitemap_node *) hurd_ihash_find(&parent->children_hash, key);
    if (found && strcmp(found->name, child_name) != 0) {
        found = NULL;
    }

    pthread_mutex_unlock(&parent->lock);

    return found;
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
        }{

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
    struct sitemap_node *root = sitemap_node_create("", "/", true, NULL);
    
}
