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
 */
 struct netnode {
     char *url; // The URL associated with this node (for files and directories)
     CURL *curl_handle; // libcurl handle for HTTP requests
     pthread_mutex_t curl_lock; // Mutex for synchronizing access to the libcurl handle
 };

 // Function prototypes for HTTPFS operations
 int httpfs_init(struct netnode *root);
 int httpfs_destroy(struct netnode *root);
 int httpfs_parse_args(int argc, char **argv, struct netnode *root);

 #endif /* HTTPFS_H */
