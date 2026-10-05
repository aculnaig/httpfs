#ifndef HTTPFS_SITEMAP_H
#define HTTPFS_SITEMAP_H

#include <hurd/ihash.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

/* A sitemap_node is the representation of the HTTP remote server. */
struct sitemap_node {

    char *name; /* name of the node. (e.g. docs/ or docs/home.html) */
    char *full_path; /* Relative POSIX path (e.g. /docs/home.html)*/
    bool is_directory;

    /* Optional metadata extracted from <lastmod> */
    time_t last_modified; /* <lastmod> timestamp (zero if absent) */
    off_t size; /* Size in bytes (0 if not known) */

    /* Tree hierarchical navigation */
    struct sitemap_node *parent; /* Reference to the parent sitemap_node (NULL for root "/") */

    /* A hash map of the children of the sitemap */
    struct hurd_ihash children_hash;

    pthread_mutex_t lock; /* Mutex for thread-safe access to the sitemap node and its hash map */
}

/*
 *  Allocates and initialized a new sitemap node.
 */
struct sitemap_node *sitemap_node_create(const char *name, const char *full_path, bool is_directory, struct sitemap_node *parent);
void sitemap_node_free(struct sitemap_node *node);

/*
 * Insert a child in the hash map of a parent node in O(1).
 */
bool sitemap_node_add_child(struct sitemap_node *parent, struct sitemap_node *child);

struct sitemap_node *sitemap_tree_lookup(struct sitemap_node *root, const char *path);

int sitemap_parse_xml_buffer(struct sitemap_node *root, const char *xml_data, size_t size);

struct sitemap_node *sitemap_fetch_and_build(const char *sitemap_url);

#endif /* HTTPFS_SITEMAP_H */
