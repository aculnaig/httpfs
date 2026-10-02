#ifndef HTTPFS_H
#define HTTPFS_H

#define _GNU_SOURCE

#include <stddef.h>
#include <sys/types.h>
#include <pthread.h>
#include <curl/curl.h>

#include <hurd/netfs.h>
#include <hurd/ihash.h>

/*
 * HTTPFS is a simple file system that allows you to mount a remote HTTP server as a local file system.
 * It uses the libcurl library to handle HTTP requests and responses.
 *
 * struct netnode is the main structure that represents a node in the file system.
 * It contains information about the node's type (file or directory), its name, and its parent node.
 * The netnode structure also contains a pointer to the next node in the linked list of nodes,
 * as well as a pointer to the data associated with the node (for files) or a pointer to the list of child nodes (for directories).
 */
 struct netnode {
     int type; // 0 for file, 1 for directory
     char *name;
     struct netnode *parent;
     struct netnode *next;
     void *data; // For files, this points to the file data. For directories, this points to the list of child nodes.

     CURL *curl_handle; // libcurl handle for HTTP requests

     void *cache; // Pointer to a cache structure for caching file data
     size_t cache_size; // Size of the cache

     hurd_ihash_t ihash_table; // Pointer to an ihash table for managing child nodes in a directory
     pthread_mutex_t ihash_lock; // Mutex for synchronizing access to the ihash table
 };

 // Function prototypes for HTTPFS operations
 int httpfs_init(struct netnode *root);
 int httpfs_destroy(struct netnode *root);

 #endif /* HTTPFS_H */
