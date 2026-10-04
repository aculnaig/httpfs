/*
    Network filesystem operations for the HTTP filesystem

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

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <hurd/netfs.h>

#include "httpfs.h"

error_t httpfs_init(struct netnode *root)
{
    error_t err = 0;
    // Initialize the libcurl library
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        return EIO; // Return an I/O error if libcurl initialization fails
    }

    root = malloc(sizeof(struct netnode));
    if (root == NULL) {
        curl_global_cleanup(); // Clean up libcurl if memory allocation fails
        return ENOMEM; // Return an out-of-memory error
    }

    root->name = strdup("httpfs");

    return 0; // Return success
}

error_t httpfs_destroy(struct netnode *root)
{
    // Destroy the node cache
    hurd_ihash_destroy(root->ihash_table);

    // Clean up libcurl
    curl_global_cleanup();

    // Destroy the mutex for synchronizing access to the node cache
    pthread_mutex_destroy(&root->ihash_lock);

    return 0; // Return success
}

/* Lookup NAME in DIR for USER; set *NODE to the found name upon return.
 * If the name was not found, then return ENOENT. On any error, clear *NODE.
 * (*NODE, if found, should be locked, this call should unlock DIR no matter what.)
 */
error_t netfs_attempt_lookup(struct iouser *user, struct node *dir, const char *name, struct node **node)
{
    error_t err = 0;

    if (dir == NULL) {
        err = ENOENT;
        return err;
    }

    if (*name == '\0' || strcmp(name, ".") == 0) {
        *node = dir;
        pthread_mutex_lock(&dir->lock);
        netfs_nref(dir);
        pthread_mutex_unlock(&dir->lock);
        return err;
    }

    if (strcmp(name, "..") == 0) {
        *node = dir->parent;
        pthread_mutex_lock(&dir->parent->lock);
        netfs_nref(dir->parent);
        pthread_mutex_unlock(&dir->parent->lock);
        return err;
    }

    /* Get the parent node from *DIR */
    struct netnode *parent_nn = netfs_node_netnode(dir);
    if (parent_nn == NULL) {
        err = ENOENT;
        return err;
    }

    /* Make the URL of the child node*/
    size_t child_url_len = strlen(parent_nn->url) + strlen(name) + 2;
    char *child_url = malloc(child_url_len);
    if (child_url == NULL) {
        err = ENOMEM;
        return err;
    }
    snprintf(child_url, child_url_len, "%s/%s", parent_nn->url, name);

    /* Execute the HTTP request to verify the existence of the child node */
    pthread_mutex_lock(&parent_nn->curl_lock);
    CURL *curl = parent_nn->curl_handle;
    curl_easy_setopt(curl, CURLOPT_URL, child_url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // Perform a HEAD request to check for existence
    CURLcode res = curl_easy_perform(curl);
    if (res != CURL_OK) {
        err = EIO; // Return an I/O error if the HTTP request fails
        free(child_url);
        pthread_mutex_unlock(&parent_nn->curl_lock);
        return err;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200) {
        err = ENOENT;
        free(child_url);
        pthread_mutex_unlock(&parent_nn->curl_lock);
        return err;
    }

    pthread_mutex_unlock(&parent_nn->curl_lock);

    /* Create a new netnode for the child node */
    struct netnode *child_nn = malloc(sizeof(struct netnode));
    if (child_nn == NULL) {
        err = ENOMEM;
        free(child_url);
        return err;
    }
    child_nn->name = strndup(name, sizeof(name));
    child_nn->url = strndup(child_url, child_url_len);
    child_nn->curl_handle = curl_easy_init();
    pthread_mutex_init(&child_nn->curl_lock, NULL);

    /* Instantiate the child node */
    struct node *child_node = netfs_make_node(child_nn);
    if (child_node == NULL) {
        err = ENOMEM;
        curl_easy_cleanup(child_nn->curl_handle);
        pthread_mutex_destroy(&child_nn->curl_lock);
        free(child_nn->url);
        free(child_nn->name);
        free(child_nn);
        return err;
    }

    return err;
}

error_t netfs_validate_stat(struct node *np, struct iouser *cred)
{
    // Validate the stat structure for the given node and user credentials
    // This function can be used to check permissions, ownership, etc.
    // For simplicity, we will just return 0 (success) in this example.
    return 0;
}

error_t netfs_attempt_rmdir(struct iouser *user, struct node *dir, const char *name)
{
    // Attempt to remove a directory with the given name from the specified parent directory
    // This function can be used to check if the directory can be removed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation not supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_mksymlink(struct iouser *cred, struct node *np, const char *name)
{
    // Attempt to create a symbolic link with the given name in the specified directory
    // This function can be used to check if the symlink can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_get_dirents(struct iouser *cred, struct node *dir, int entry, int nentries, char **data, mach_msg_type_number_t *datacnt, vm_size_t bufsize, int *amt)
{
    // Get the directory entries for the specified directory
    // This function can be used to retrieve the list of files and directories in the specified directory
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_utimes(struct iouser *cred, struct node *np, struct timespec *atime, struct timespec *mtime)
{
    // Attempt to update the access and modification times of the specified node
    // This function can be used to check if the times can be updated based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_readlink(struct iouser *user, struct node *np, char *buf) {
    // Attempt to read the target of a symbolic link for the specified node
    // This function can be used to check if the symlink can be read based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_sync(struct iouser *cred, struct node *np, int wait) {
    // Attempt to synchronize the specified node with the underlying storage
    // This function can be used to check if the node can be synchronized based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_check_open_permissions(struct iouser *user, struct node *np, int flags, int newnode) {
    // Check if the specified user has permission to open the specified node with the given flags
    // This function can be used to check if the node can be opened based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_mkdir(struct iouser *user, struct node *dir, const char *name, mode_t mode) {
    // Attempt to create a new directory with the given name and mode in the specified parent directory
    // This function can be used to check if the directory can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_chmod(struct iouser *cred, struct node *np, mode_t mode) {
    // Attempt to change the permissions of the specified node to the given mode
    // This function can be used to check if the permissions can be changed based on user credentials, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_rename(struct iouser *user, struct node *fromdir, const char *fromname, struct node *todir, const char *toname, int excl) {
    // Attempt to rename a file or directory from the specified source directory and name to the specified target directory and name
    // This function can be used to check if the rename operation can be performed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_chauthor(struct iouser *cred, struct node *np, uid_t author) {
    // Attempt to change the author of the specified node to the given author ID
    // This function can be used to check if the author can be changed based on user credentials, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_chflags(struct iouser *cred, struct node *np, int flags) {
    // Attempt to change the flags of the specified node to the given flags
    // This function can be used to check if the flags can be changed based on user credentials, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_access(struct iouser *cred, struct node *np, int *types) {
    // Attempt to check the access permissions of the specified node for the given user credentials
    // This function can be used to check if the user has the required access permissions for the
    // specified node based on the requested access types (read, write, execute, etc.)
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

int netfs_maxsymlinks = 0; // Maximum number of symbolic links allowed in the filesystem (0 means no limit)

error_t netfs_attempt_link(struct iouser *user, struct node *dir, struct node *file, const char *name, int excl) {
    // Attempt to create a hard link to the specified file in the given directory with the specified name
    // This function can be used to check if the hard link can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_create_file(struct iouser *user, struct node *dir, const char *name, mode_t mode, struct node **np) {
    // Attempt to create a new file with the given name and mode in the specified parent directory
    // This function can be used to check if the file can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_statfs(struct iouser *cred, struct node *np, fsys_statfsbuf_t *st) {
    // Attempt to retrieve filesystem statistics for the specified node
    // This function can be used to check if the filesystem statistics can be retrieved based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_mkdev(struct iouser *cred, struct node *np, mode_t type, dev_t indexes) {
    // Attempt to create a new device node with the given type and indexes in the specified directory
    // This function can be used to check if the device node can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_chown(struct iouser *cred, struct node *np, uid_t owner, gid_t group) {
    // Attempt to change the ownership of the specified node to the given owner and group
    // This function can be used to check if the ownership can be changed based on user credentials, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_mkfile(struct iouser *user, struct node *dir, mode_t mode, struct node **np) {
    // Attempt to create a new regular file with the given mode in the specified parent directory
    // This function can be used to check if the file can be created based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_write(struct iouser *cred, struct node *np, loff_t offset, size_t *len, const void *data) {
    // Attempt to write data to the specified node at the given offset
    // This function can be used to check if the write operation can be performed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_read(struct iouser *cred, struct node *np, loff_t offset, size_t *len, void *data) {
    // Attempt to read data from the specified node at the given offset
    // This function can be used to check if the read operation can be performed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_syncfs(struct iouser *cred, int wait) {
    // Attempt to synchronize the entire filesystem with the underlying storage
    // This function can be used to check if the filesystem can be synchronized based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_set_size(struct iouser *cred, struct node *np, loff_t size) {
    // Attempt to set the size of the specified node to the given size
    // This function can be used to check if the size can be changed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_attempt_unlink(struct iouser *user, struct node *dir, const char *name) {
    // Attempt to remove a file or directory with the given name from the specified parent directory
    // This function can be used to check if the unlink operation can be performed based on permissions, etc.
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

error_t netfs_report_access(struct iouser *cred, struct node *np, int *types) {
    // Report the access permissions of the specified node for the given user credentials
    // This function can be used to check if the user has the required access permissions for the
    // specified node based on the requested access types (read, write, execute, etc.)
    // For simplicity, we will just return EOPNOTSUPP (operation non supported) in this example.
    return EOPNOTSUPP;
}

void netfs_node_norefs(struct node *np) {
    // Clean up a node when it has no more references
    // This function can be used to free any resources associated with the node
    struct netnode *nn = netfs_node_netnode(np);
    if (nn != NULL) {
        curl_easy_cleanup(nn->curl_handle); // Clean up the libcurl handle associated with the node
        pthread_mutex_destroy(&nn->curl_lock); // Destroy the mutex associated with the libcurl handle
        hurd_ihash_destroy(nn->ihash_table); // Destroy the ihash table associated with the node
        pthread_mutex_destroy(&nn->ihash_lock); // Destroy the mutex associated with the ihash table
        free(nn->url); // Free the url string associated with the node
        free(nn->name); // Free the name string associated with the node
        free(nn);
    }

    netfs_drop_node(np); // Drop the reference to the node in the netfs
}
